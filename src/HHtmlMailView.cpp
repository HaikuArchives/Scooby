#include "HHtmlMailView.h"
#include "CTextView.h"
#include "HHtmlView.h"
#include "HTabView.h"
#include "BetterScrollView.h"
#include "HAttachmentList.h"
#include "HAttachmentItem.h"
#include "Encoding.h"
#include "HMailList.h"
#include "HApp.h"
#include "TrackerUtils.h"
#include "HPrefs.h"
#include "Utilities.h"

#include <TabView.h>
#include <Debug.h>
#include <ClassInfo.h>
#include <File.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Window.h>
#include <E-mail.h>
#include <Beep.h>
#include <NodeInfo.h>
#include <ScrollBar.h>
#include <ctype.h>
#include <Roster.h>

#define TMP_FILE_NAME "Scooby.tmp"

/***********************************************************
 * Constructor
 ***********************************************************/
HHtmlMailView::HHtmlMailView(BRect frame,const char* name,
							uint32 resize,uint32 flags)
		:BView(frame,name,resize,flags|B_WILL_DRAW|B_FRAME_EVENTS)
		,fFile(NULL)
		,fFilePanel(NULL)
{
	BRect rect = Bounds();

	const char* kTabNames[3] = {_("Body"),_("Headers"),_("Attachments")};
	HTabView *tabview = new HTabView(rect,"tabview",kTabNames,3,B_FOLLOW_ALL);
	tabview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BView* TabbedViews[3];
	BRect ContentRect = tabview->GetContentArea();
	fHtmlView = new HHtmlView(ContentRect,_("Body"),false,B_FOLLOW_ALL);
	TabbedViews[0] = fHtmlView;
	
	BRect textRect = ContentRect;
	textRect.right -= B_V_SCROLL_BAR_WIDTH;
	fHeaderView = new CTextView(textRect,"headerview",B_FOLLOW_ALL,B_WILL_DRAW);
	fHeaderView->MakeEditable(false);
	fHeaderView->SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_1_TINT));
	BScrollView *scroll = new BScrollView(_("Headers"),fHeaderView,B_FOLLOW_ALL,0,true,true);
	TabbedViews[1] = scroll;
	
	BetterScrollView *bscroll;
	BRect attachlist = ContentRect;
	attachlist.right -= B_V_SCROLL_BAR_WIDTH;
	attachlist.bottom -= B_H_SCROLL_BAR_HEIGHT;

	fAttachmentList = new HAttachmentList(attachlist,&bscroll,"attachmentlist");
	TabbedViews[2] = bscroll;
	
	tabview->AddViews(TabbedViews);
	tabview->SetTabEnabled(2,false);
	AddChild(tabview);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HHtmlMailView::~HHtmlMailView()
{
	ClearList();
	delete fFile;
	delete fFilePanel;
	// Remove tmp file
	BPath path;
	::find_directory(B_COMMON_TEMP_DIRECTORY,&path);
	path.Append( TMP_FILE_NAME );
	
	BEntry entry(path.Path());
	if(entry.InitCheck() == B_OK)
		entry.Remove();
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HHtmlMailView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_OPEN_ATTACHMENT:
	{
		int32 sel = fAttachmentList->CurrentSelection();
		if(sel >= 0)
			OpenAttachment(sel);
		break;
	}
	case M_SAVE_ATTACHMENT:
	{
		int32 sel = fAttachmentList->CurrentSelection();
		HAttachmentItem *item = cast_as(fAttachmentList->ItemAt(sel),HAttachmentItem);
		if(!item)
			break;
		if(!fFilePanel)
		{
			fFilePanel = new MySavePanel(this);
		}
		fFilePanel->SetSaveText(item->Name());
		fFilePanel->Show();
		break;
	}
	case B_REFS_RECEIVED:
	{
		int32 sel;
		int32 index = 0;
		
		while((sel = fAttachmentList->CurrentSelection(index++)) >= 0)
		{
			const char* name;
			entry_ref ref;
			if((message->FindRef("directory",&ref) == B_OK)&&
				(message->FindString("name",&name) == B_OK))
			{
				PRINT(("%s\n",name));
				SaveAttachment(sel,ref,name,false);	
			}else if(message->FindRef("directory",&ref) == B_OK){
				
				HAttachmentItem* item = cast_as(fAttachmentList->ItemAt(sel),HAttachmentItem);
				if(!item)
					continue;
				name = item->Name();
				PRINT(("%s\n",name));
				SaveAttachment(sel,ref,(!name)?"Unknown":name);
			}
		}
		break;
	}
	case M_SET_CONTENT:
	{
		BFile *file;
		if(message->FindPointer("pointer",(void**)&file) == B_OK)
			SetContent(file);
		else{
			if(fFile)
			{
				app_info info;
    				be_app->GetAppInfo(&info); 
    				BPath path(&info.ref);
    				path.GetParent(&path);
    				path.Append("html/startup.html");
    				char *buf = new char[::strlen(path.Path())+8];
    				::sprintf(buf,"file://%s",path.Path());
    				fHtmlView->SetEncoding((EncodingMessages)fHtmlView->fDefaultEncoding);
    				fHtmlView->ShowURL(buf);
				delete[] buf;
				
				ResetAttachmentList();
				ResetScrollBar();
				delete fFile;
				fFile = NULL;
				
				fHeaderView->SetText(NULL);
			}
		}
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * OpenAttachment
 ***********************************************************/
void
HHtmlMailView::OpenAttachment(int32 sel )
{
	HAttachmentItem *item = cast_as(fAttachmentList->ItemAt(sel),HAttachmentItem);
	if(!item)
	{
		beep();
		return;
	}
	int32 file_offset = item->Offset();
	int32 data_len = item->DataLength();
	
	off_t size;
	fFile->Seek(SEEK_SET,0);
	fFile->GetSize(&size);
	char *buf = new char[size+1];
	size = fFile->Read(buf,size);
	buf[size] = '\0';
	char *data = new char[data_len+1];
	::memcpy(data,&buf[file_offset],data_len);
	data[data_len] = '\0';
	const char* name = item->Name();
	
	// dump to tmp directory
	BPath path;
	entry_ref ref;
	::find_directory(B_COMMON_TEMP_DIRECTORY,&path);
	path.Append((name)?name:"Unknown");
	::get_ref_for_path(path.Path(),&ref);
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
	
	const char* encoding = item->ContentEncoding();
	if(encoding && ::strcasecmp(encoding,"base64") == 0)
	{
		bool is_text = false;
		if(::strncmp(item->ContentType(),"text",4) == 0)
			is_text = true;
		data_len = decode_base64(data, data, data_len, is_text);
	}else if(encoding && ::strcasecmp(encoding,"quoted-printable") == 0){
		data_len = Encoding().decode_quoted_printable(data,data,data_len,false);
	}else{
		BString label("Unsupported attachment format: ");
		label += encoding;
		(new BAlert("",label.String(),"OK"))->Go();
		return;
	}
	file.Write(data,data_len);
	file.SetSize(data_len);
	::update_mime_info(path.Path(),false,true,true);
	// Open with tracker
	BMessage	open(B_REFS_RECEIVED);
	BMessenger tracker("application/x-vnd.Be-TRAK", -1, NULL);
	if (tracker.IsValid())
	{
		open.AddRef("refs", &ref);
		tracker.SendMessage(&open);
	}
	// Set item state
	item->SetExtracted(true);
	item->SetFileRef(ref);
	delete[] data;
	delete[] buf;
}

/***********************************************************
 * SetContent
 ***********************************************************/
void
HHtmlMailView::SetContent(BFile *file)
{
	LoadMessage(file);	
}

/***********************************************************
 * SaveAttachment
 ***********************************************************/
void
HHtmlMailView::SaveAttachment(int32 sel,entry_ref ref,const char* name,bool rename)
{
	HAttachmentItem *item = cast_as(fAttachmentList->ItemAt(sel),HAttachmentItem);
	if(!item)
	{
		beep();
		return;
	}
	BDirectory destDir(&ref);
	BPath path(&ref);
	path.Append(name);
	
	int32 file_offset = item->Offset();
	int32 data_len = item->DataLength();
	
	off_t size;
	fFile->Seek(SEEK_SET,0);
	fFile->GetSize(&size);
	char *buf = new char[size+1];
	size = fFile->Read(buf,size);
	buf[size] = '\0';
	char *data = new char[data_len+1];
	::memcpy(data,&buf[file_offset],data_len);
	data[data_len] = '\0';
	
	BFile file;
	if(rename)
		TrackerUtils().SmartCreateFile(&file,&destDir,path.Leaf(),"_");
	else{
		if(file.SetTo(path.Path(),B_WRITE_ONLY|B_CREATE_FILE) != B_OK)
			return;
	}	
	const char* encoding = item->ContentEncoding();
	if(encoding && ::strcasecmp(encoding,"base64") == 0)
	{
		bool is_text = false;
		if(::strncmp(item->ContentType(),"text",4) == 0)
			is_text = true;
		data_len = decode_base64(data, data, data_len, is_text);
	}else if(encoding && ::strcasecmp(encoding,"base64") == 0){
		data_len = Encoding().decode_quoted_printable(data,data,data_len,false);
	}else{
		BString label("Unsupported attachment format: ");
		label += encoding;
		(new BAlert("",label.String(),"OK"))->Go();
		return;
	}
	file.Write(data,data_len);
	file.SetSize(data_len);
	if(item->ContentType())
	{
		BNodeInfo ninfo(&file);
		ninfo.SetType(item->ContentType());
	}
	delete[] buf;
	delete[] data;
}

/***********************************************************
 * ResetScrollBar
 ***********************************************************/
void
HHtmlMailView::ResetScrollBar()
{
	// Reset scrollbar
	BScrollBar *bar(NULL);
	BView* view = this->FindView("NetPositive");
	if(view && (view = view->ChildAt(0))) 
	{
		view = view->ChildAt(1);
		bar = cast_as(view,BScrollBar);
		if(bar)
			bar->SetValue(0);
	}
}

/***********************************************************
 * ResetAttachmentList
 ***********************************************************/
void
HHtmlMailView::ResetAttachmentList()
{
	ClearList();
	// Reset attachment tab
	HTabView *tabview = cast_as(FindView("tabview"),HTabView);
	tabview->SetTabLabel(2,_("Attachments"));
	tabview->SetTabEnabled(2,false);
}

/***********************************************************
 * LoadMessage
 ***********************************************************/
void
HHtmlMailView::LoadMessage(BFile *file)
{
	if(!file)
		return;
	delete fFile;
	fFile = NULL;
	ResetAttachmentList();
	ResetScrollBar();
	fHeaderView->SetText(NULL);
	bool html = false;
	bool openNewWindow;
	((HApp*)be_app)->Prefs()->GetData("open_link_as_new_window",&openNewWindow);

	// Get content and header size
	Encoding encode;
	bool multipart = false;
	bool bfile = false;
	char *parameter = NULL;
	char *charset = NULL;
	int32 header_len,content_len;
	ssize_t attrsize;
	off_t filesize;
	file->GetSize(&filesize);
	attrsize = file->ReadAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&header_len,sizeof(int32));
	if(attrsize == B_ENTRY_NOT_FOUND || header_len< 0)
	{
		// if could not get header length from attr, get fron mail content.
		PRINT(("ATTRSIZE:%d  HEADER:%d\n",attrsize,header_len));
		char *filecontent = new char[filesize+1];
		filesize = file->Read(filecontent,filesize);
		filecontent[filesize] = '\0';
		header_len = 0;
		for(int32 i = 0;i < filesize;i++)
		{
			if(strncmp(&filecontent[i],"\r\n\r\n",4)==0)
			{
				header_len = i+4;
				break;
			}
		}
		delete[] filecontent;
	}
	attrsize = file->ReadAttr(B_MAIL_ATTR_CONTENT,B_INT32_TYPE,0,&content_len,sizeof(int32));
	if( attrsize == B_ENTRY_NOT_FOUND || content_len < 0)
	{
		PRINT(("ATTRSIZE:%d  HEADER:%d\n",attrsize,content_len));
		content_len = filesize-header_len;
	}
	// Load message from file
	off_t size;
	file->GetSize(&size);
	char *buf = new char[size+1];
	BString content;
	file->Seek(SEEK_SET,0);
	size= file->ReadAt(0,buf,size);
	buf[size] = '\0';
	char *header = new char[header_len+1];
	::strncpy(header,buf,header_len);
	header[header_len] = '\0';
	content = &buf[header_len];
	delete[] buf;
	// Find Content-Type
	BString content_type("");
	if(GetParameter(header,"Content-Type:",&parameter))
	{
		if(::strncmp(parameter,"multipart/",10) == 0)
			multipart = true;
	}
	content_type = parameter;
	delete[] parameter;
	parameter = NULL;
	// Find Transfer encoding
	char* transfer_encoding;
	if(GetParameter(header,"Content-Transfer-Encoding:",&parameter))
	{
		if(::strncmp(parameter,"multipart/",10) == 0)
			multipart = true;
		if(::strcmp(parameter,"multipart/x-bfile") == 0)
			bfile = true;
	}
	transfer_encoding = ::strdup(parameter);
	delete[] parameter;
	parameter = NULL;
	// Find boudary
	BString boundary;
	if(multipart)
	{
		GetParameter(header,"boundary=",&parameter);
		boundary = parameter;
		delete[] parameter;
		parameter = NULL;
	}
	// Parse all parts
	if(multipart)
		ParseAllParts(content.String(),boundary.String(),header_len);
	// Convert plain text to html
	if(multipart)
	{
		int32 part_index = fAttachmentList->FindPart("text/html");
		if(part_index < 0)
		{
			part_index = fAttachmentList->FindPart("text/plain");
			html = false;
		}else
			html = true;;
		HAttachmentItem *item = cast_as(fAttachmentList->ItemAt(part_index),HAttachmentItem);
		if(!item)
		{
			GetParameter(header,"charset=",&parameter);
			Plain2Html(content,parameter,transfer_encoding);
			delete[] parameter;
			parameter = NULL;
		}else{
			int32 data_len = item->DataLength();
			int32 offset = item->Offset();
			BString part;
			content.CopyInto(part,offset-header_len,data_len);
			// Extract
			const char* encoding = item->ContentEncoding();
					
			if(encoding)
			{
				bool mime_q = false;
				bool mime = false;
				if(::strcasecmp(encoding,"quoted-printable") == 0)
				{
					mime_q = true;
					mime = true;
				}else if(::strcasecmp(encoding,"base64") == 0){
					mime = true;
					mime_q = false;
				}
				if(mime)
					encode.MimeDecode(part,mime_q);
			}
			charset = ::strdup(item->Charset());
			if(!html && charset)
			{
				Plain2Html(part,charset,transfer_encoding);
			}
			content = part;
			// Set attachment tab label
			if(fAttachmentList->CountItems() > 0)
			{
				BString label(_("Attachments"));
				label <<" [ " <<fAttachmentList->CountItems() << " ]";
				HTabView *tabview = cast_as(FindView("tabview"),HTabView);
				
				tabview->SetTabLabel(2,label.String() );
				tabview->SetTabEnabled(2,true);
			}
			// Convert link target to blank window
			if(html && openNewWindow)
				ConvertLinkToBlankWindow(content);
		}
	}else{
		if(content_type.Compare("text/html") != 0)
		{
			GetParameter(header,"charset=",&parameter);
			charset = ::strdup(parameter);
			Plain2Html(content,parameter,transfer_encoding);
			delete[] parameter;
			parameter = NULL;
		}else{
			if(transfer_encoding)
			{
				bool mime_q = false;
				bool mime = false;
				if(::strcasecmp(transfer_encoding,"quoted-printable") == 0)
				{
					mime_q = true;
					mime = true;
				}else if(::strcasecmp(transfer_encoding,"base64") == 0){
					mime = true;
					mime_q = false;
				}
				if(mime)
					encode.MimeDecode(content,mime_q);
			}
			// Convert link target to blank window
			if(html && openNewWindow)
				ConvertLinkToBlankWindow(content);
		}
	}
	encode.ConvertReturnsToLF(content);
	encode.ConvertReturnsToLF(header);
	// Dump to tmp directory
	BPath path;
	::find_directory(B_COMMON_TEMP_DIRECTORY,&path);
	path.Append( TMP_FILE_NAME );
	BFile tmpFile(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
	tmpFile.Write(content.String(),content.Length());
	tmpFile.SetSize(content.Length());
	// Get tmp file path
	char *tmpPath = new char[::strlen(path.Path())+7+1];
	::sprintf(tmpPath,"file://%s",path.Path());
	// Set content and header
	if(charset)
		SetEncoding(charset);
	else
		fHtmlView->SetEncoding((EncodingMessages)fHtmlView->fDefaultEncoding);
	fHtmlView->ShowURL(tmpPath);
	fHeaderView->SetText( header );
	delete[] tmpPath;
	delete[] header;
	free( charset );
	free(transfer_encoding);
	fFile = file;
}

/***********************************************************
 * SetEncoding
 ***********************************************************/
void
HHtmlMailView::SetEncoding(const char* encoding)
{
	PRINT(("%s\n",encoding));
	int32 what = fHtmlView->fDefaultEncoding;
	if(::strcasecmp(encoding,"ISO-8859-1") == 0)
		what = NETPOSITIVE_ISO1;
	else if(::strcasecmp(encoding,"ISO-8859-2") == 0)
		what = NETPOSITIVE_ISO2;
	else if(::strcasecmp(encoding,"ISO-8859-5") == 0)
		what = NETPOSITIVE_ISO5;
	else if(::strcasecmp(encoding,"ISO-8859-7") == 0)
		what = NETPOSITIVE_ISO7;
	else if(::strcasecmp(encoding,"ISO-2022-JP") == 0)
		what = NETPOSITIVE_JAPANESE_AUTO;
	else if(::strcasecmp(encoding,"euc-jp") == 0)
		what = NETPOSITIVE_JAPANESE_AUTO;
	else if(::strcasecmp(encoding,"utf-8") == 0)
		what = NETPOSITIVE_UTF8;
	else if(::strcasecmp(encoding,"koi8-r") == 0)
		what = NETPOSITIVE_KOI8R;
	else if(::strcasecmp(encoding,"windows-1251") == 0)
		what = NETPOSITIVE_KOI8R;
	fHtmlView->SetEncoding((EncodingMessages)what);
}

/***********************************************************
 * ConvertLinkToBlankWindow
 ***********************************************************/
void
HHtmlMailView::ConvertLinkToBlankWindow(BString &str)
{
	const char* text = str.String();
	int32 len = str.Length();
	BString out;
	
	for(int32 i = 0;i < len;i++)
	{
		if(strncasecmp(&text[i],"href ",4) == 0)
		{
			while(text[i] && text[i] != '>')
				out += text[i++];
			if(text[i] == '>')
			{
				out += " target=\"_blank\"";
				out += text[i];
			}
		}else
			out += text[i];
	}
	str = out;
}

/***********************************************************
 * Plain2Html
 ***********************************************************/
void
HHtmlMailView::Plain2Html(BString &content,const char* encoding,const char* transfer_encoding)
{
	bool openNewWindow;
	((HApp*)be_app)->Prefs()->GetData("open_link_as_new_window",&openNewWindow);
	
	char buf[20];
	Encoding encode;
	bool translate_space = false;
	BString out("");
	out += "<html>\n";
	out += "<body bgcolor=white>\n";
	// Convert to UTF8 
	// We need to convert to UTF8 for multibyte charactor support
	if(transfer_encoding && ::strcasecmp(transfer_encoding,"quoted-printable") == 0)
		encode.MimeDecode(content,true);
	else if(transfer_encoding&&::strcasecmp(transfer_encoding,"base64") == 0)
		encode.MimeDecode(content,false);
	
	int32 default_encoding = 0;
	if(encoding)
		encode.ConvertToUTF8(content,encoding);
	else{
		default_encoding = encode.DefaultEncoding();
		encode.ConvertToUTF8(content,default_encoding);
	}
	if(encoding)
	{
		out += "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=";
		out += encoding;
		out += "\">";
	}
	// We must add higlight quote and replace line feed
	const char* text = content.String();
	
	int32 len = content.Length();
	BString tmp;
	for(int32 i = 0;i < len;i++)
	{
		switch(text[i])
		{
		// Hight quoted text
		case '>':
		{
			if(text[i-1] == '\n' || i == 0)
			{
				BString bullet, bulletb, blank;
				bullet  += "<td width=4 bgcolor=silver></td>";
				bulletb += "<td width=8 bgcolor=white></td>";
				blank   += "<td width=4 bgcolor=white></td>";
				tmp += "<table cellspacing=0 cellpadding=0><tr>";
				if((text[i+1] == ' ')
				&& (text[i+2] == '>'))
				{
					char* textw = const_cast<char *>(text);
					textw[i]   = ' ';
					textw[i+2] = ' ';
					tmp << bullet << bulletb << bullet << blank;
				}
				else if((text[i+1] == '>')
				     && (text[i+2] != '>'))
				{
					char* textw = const_cast<char *>(text);
				    textw[i]   = ' ';
				    textw[i+1] = ' ';
					tmp << bullet << bulletb << bullet << blank;
				}
				else if((text[i+1] == '>')
				     && (text[i+2] == '>')
				     && (text[i+3] == '>'))
				{
					char* textw = const_cast<char *>(text);
					textw[i]   = ' ';
					textw[i+1] = ' ';
					textw[i+2] = ' ';
					textw[i+3] = ' ';
					tmp << bullet << bulletb << bullet << bulletb;
					tmp << bullet << bulletb << bullet << blank;				
				}
				else if((text[i+1] == '>')
				     && (text[i+2] == '>'))
				{
					char* textw = const_cast<char *>(text);
					textw[i]   = ' ';
					textw[i+1] = ' ';
					textw[i+2] = ' ';
					tmp << bullet << bulletb << bullet << bulletb;
					tmp << bullet << blank;
				}
				else
				{
					char* textw = const_cast<char *>(text);
					textw[i] = ' ';
					tmp << bullet << blank;
				}
				tmp += "<td><i>";
				while(text[i] != '\0' && text[i] != '\n')
				{
					// Normal charactors
					ConvertToHtmlCharactor(text[i++],buf,&translate_space);
					tmp += buf;
				}
				tmp += "</i></td></tr></table>";
				ConvertToHtmlCharactor(text[i],buf,&translate_space);
				tmp += buf;
			}else{
				ConvertToHtmlCharactor(text[i],buf,&translate_space);
				tmp += buf;
			}
			break;
		}
		// http,ftp,mailto
		case 'f':
		case 'h':
		case 'm':
			if(strncmp(&text[i],"http://",7) == 0
				|| strncmp(&text[i],"https://",8) == 0
				|| strncmp(&text[i],"ftp://",6) == 0
				|| strncmp(&text[i],"mailto:",7) == 0)
			{
				BString uri("");
				while(text[i] != '\0'&& IsURI(text[i]))
					uri += text[i++];
				tmp += "<a href=\"";
				tmp += uri;
				tmp += "\"";
				if(openNewWindow)
					tmp += " target=\"_blank\"";
				tmp += ">";
				tmp += uri;
				tmp += "</a>";
				ConvertToHtmlCharactor(text[i],buf,&translate_space);
				tmp += buf;
			}else{
				ConvertToHtmlCharactor(text[i],buf,&translate_space);
				tmp += buf;
			}
			break;
		default:
				ConvertToHtmlCharactor(text[i],buf,&translate_space);
				tmp += buf;
		}
	}
	// Convert from UTF8
	if(encoding)
		encode.ConvertFromUTF8(tmp,encoding);
	else
		encode.ConvertFromUTF8(tmp,default_encoding);
	//
	content = tmp;
	//
	out += content;
	out += "\n</body>\n";
	out += "</html>\n";
	content = out;
}

/***********************************************************
 * ConvertToHtmlCharactor
 ***********************************************************/
void
HHtmlMailView::ConvertToHtmlCharactor(char c,char *out,bool *translate_space)
{
	::memset(out,0,20);
	// Special charactors
	switch(c)
	{
	case '>':
		::strcpy(out,"&gt;");
		break;
	case '<':
		::strcpy(out,"&lt;");
		break;
	case '&':
		::strcpy(out,"&amp;");
		break;
	case '"':
		::strcpy(out,"&quot;");
		break;
	case '!':
		::strcpy(out,"&#33;");
		break;
	case '#':
		::strcpy(out,"&#35;");
		break;
	case '$':
		::strcpy(out,"&#36;");
		break;
	case '%':
		::strcpy(out,"&#37;");
		break;
	case '\n':
		::sprintf(out,"<br>%c",c);
		break;
	case '\t':
		::strcpy(out,"&nbsp;&nbsp;&nbsp;");
		break;
	case ' ':
		if(*translate_space)
			::strcpy(out,"&nbsp;");		
		else{
			out[0] = c;
			*translate_space = true;
		}
		return;
	default:
		out[0] = c;
	}
	*translate_space = false;
}

/***********************************************************
 * IsURI
 ***********************************************************/
bool
HHtmlMailView::IsURI(char c)
{
	if(c != ' '&&c != '\t'&&c != '>'&&c != '<'&&c != ')'&&c != '('&&
		c != '"'&&c != '\''&&c != '\n'&&c != '\r'&&c != '['&&c != ']'&&!(c >> 7))
		return true;
	
	return false;
}

/***********************************************************
 * ParseAllParts
 ***********************************************************/
void
HHtmlMailView::ParseAllParts(const char* str,const char* boundary,int32 header_len)
{
	char *new_boundary = new char[::strlen(boundary)+3];
	::sprintf(new_boundary,"--%s",boundary);
	int32 boundary_len = ::strlen(new_boundary);
	char *end_boundary = new char[boundary_len +3];
	::sprintf(end_boundary,"%s--",new_boundary);
	
	BString content(str);
	int32 start=0;
	int32 part_offset = 0;
	BString part("");
	char *subBoundary = NULL;
	
	while((start=content.FindFirst(new_boundary,start)) != B_ERROR)
	{
		start+= boundary_len;
		part_offset = start;
		part = "";
		while(::strncmp(&content[start],new_boundary,boundary_len) != 0)
			part += content[start++];
		
		if( GetParameter(part.String(),"boundary=",&subBoundary) )
		{
			ParseAllParts(part.String(),subBoundary,header_len+part_offset);
		}else
			AddPart(part.String(),header_len+part_offset);
		delete[] subBoundary;
		subBoundary = NULL;
		if(::strncmp(&content[start],end_boundary,boundary_len+2) == 0)
			break;
	}
	
	delete[] new_boundary;
	delete[] end_boundary;
}

/***********************************************************
 * AddPart
 ***********************************************************/
void
HHtmlMailView::AddPart(const char* part,int32 file_offset)
{
	if(::strlen(part) == 0)
		return;
	BString line("");
	
	char* content_type = NULL;
	char* name = NULL;
	char* encoding = NULL;
	char* charset = NULL;

	GetParameter(part,"Content-Type:",&content_type);
	GetParameter(part,"name=",&name);
	GetParameter(part,"Content-Transfer-Encoding:",&encoding);
	GetParameter(part,"charset=",&charset);

	//PRINT(("Type:%s\nName:%s\nEncoding:%s\ncharset:%s\n\n",content_type,name,encoding,charset));
	
	char *p = strstr(part,"\r\n\r\n");
	int32 offset = 0;
	int32 data_len = 0;
	if(p)
	{
		data_len = ::strlen(p)-4;
		offset = ::strlen(part) - ::strlen(p)+4;
	}
	offset += file_offset;
	//PRINT(("Offset:%d FileOffset:%d DataLen:%d\n\n",offset,file_offset,data_len));
	HAttachmentItem *item;
	fAttachmentList->AddItem(item = new HAttachmentItem(name,
												offset,
												data_len,
												content_type,
												encoding,
												charset));
	
	delete[] content_type;
	delete[] name;
	delete[] encoding;	
	delete[] charset;
}

/***********************************************************
 * ClearList
 ***********************************************************/
void
HHtmlMailView::ClearList()
{
	HAttachmentItem *item;
	int32 count = fAttachmentList->CountItems();
	while(count>0)
	{
		item = cast_as(fAttachmentList->RemoveItem(--count),HAttachmentItem);
		
		if(item && item->IsExtracted())
		{
			entry_ref ref = item->FileRef();
			BEntry entry(&ref);
			if(entry.InitCheck() == B_OK)
				entry.Remove();
		}
		delete item;
	}
}

/***********************************************************
 * Constructor
 ***********************************************************/
MySavePanel::MySavePanel(BView *target)
	:BFilePanel(B_SAVE_PANEL)
	,fTargetView(target)
{
	
}

/***********************************************************
 * Destructor
 ***********************************************************/
MySavePanel::~MySavePanel()
{
}

/***********************************************************
 * SendMessage
 ***********************************************************/
void
MySavePanel::SendMessage(const BMessenger*, BMessage* msg)
{
	const char	*name = NULL;
	BMessage	save(B_REFS_RECEIVED);
	entry_ref	ref;

	if ((msg->FindRef("directory", &ref) == B_OK) && 
		(msg->FindString("name", &name) == B_OK)) {
		save.AddString("name", name);
		save.AddRef("directory", &ref);
		fTargetView->Window()->PostMessage(&save, fTargetView);
	}
}
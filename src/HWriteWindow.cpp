#include "HWriteWindow.h"
#include "HAddressView.h"
#include "MenuUtils.h"
#include "ResourceUtils.h"
#include "HToolbar.h"
#include "HApp.h"
#include "HPrefs.h"
#include "Encoding.h"
#include "HSmtpClientView.h"
#include "SmtpClient.h"
#include "HEnclosureView.h"
#include "ArrowButton.h"
#include "HEnclosureItem.h"
#include "HMailItem.h"
#include "HMailView.h"
#include "TrackerUtils.h"
#include "HWindow.h"


#include <MenuBar.h>
#include <ClassInfo.h>
#include <Debug.h>
#include <ScrollView.h>
#include <E-mail.h>
#include <Alert.h>
#include <Window.h>
#include <NodeInfo.h>
#include <stdio.h>
#include <File.h>
#include <Entry.h>
#include <Autolock.h>
#include <MenuField.h>
#include <Beep.h>
#include <Clipboard.h>

#define DRAFT_FOLDER "Drafts"
#define TEMPLATE_FOLDER "Templates"

const char *kBoundary="----=_NextPart_";

/***********************************************************
 * Constructor
 ***********************************************************/
HWriteWindow::HWriteWindow(BRect rect
							,const char* name
							,const char* subject
							,const char* to
							,HMailItem *replyItem
							,bool reply
							,bool forward)
	:BWindow(rect,name,B_DOCUMENT_WINDOW,0)
	,fFilePanel(NULL)
	,fReply(reply)
	,fForward(forward)
	,fReplyItem(replyItem)
	,fReplyFile(NULL)
	,fDraftEntry(NULL)
	,fSkipComplete(false)
	,fSent(false)
{
	InitMenu();
	InitGUI();
	BTextControl *ctrl;
	ctrl = cast_as(FindView("to"),BTextControl);
	ctrl->MakeFocus(true);
	if(to)
		ctrl->SetText(to);
	ctrl = cast_as(FindView("subject"),BTextControl);
	if(subject)
		ctrl->SetText(subject);
	if(fReplyItem)
	{
		entry_ref ref = fReplyItem->Ref();
		// this file pointer will be deleted with HMailView
		fReplyFile = new BFile(&ref,B_READ_ONLY);
		fTextView->LoadMessage(fReplyFile,reply,false,NULL);
	}
	
	// SetWindowSizeLimit
	float min_width,min_height,max_width,max_height;
	GetSizeLimits(&min_width,&max_width,&min_height,&max_height);
	min_width = 400;
	min_height = 300;
	SetSizeLimits(min_width,max_width,min_height,max_height);
	AddShortcut('U',0,new BMessage(M_SELECT),fTextView);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HWriteWindow::~HWriteWindow()
{
	delete fFilePanel;
	delete fDraftEntry;
	if(fReplyItem && fReplyItem->fDeleteMe)
		delete fReplyItem;
}

/***********************************************************
 * InitMenu
 ***********************************************************/
void
HWriteWindow::InitMenu()
{
	BMenuBar *menubar = new BMenuBar(Bounds(),"");
    BMenu		*aMenu,*subMenu;
    BMenuItem 	*item;
	MenuUtils utils;
	BPath path;
	ResourceUtils rsrc_utils;
//// ------------------------ File Menu ----------------------    
	aMenu = new BMenu(_("File"));
	
	subMenu = new BMenu(_("Open draft"));
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append(DRAFT_FOLDER);
	
	AddChildItem(subMenu,path.Path(),M_OPEN_DRAFT);
	aMenu->AddItem(subMenu);
	utils.AddMenuItem(aMenu,_("Save as draft"),M_SAVE_DRAFT,this,this,'S',B_SHIFT_KEY);
	
	aMenu->AddSeparatorItem();
	
	subMenu = new BMenu(_("Open template"));
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append(TEMPLATE_FOLDER);
	
	AddChildItem(subMenu,path.Path(),M_OPEN_TEMPLATE);
	aMenu->AddItem(subMenu);
	utils.AddMenuItem(aMenu,_("Save as template"),M_SAVE_TEMPLATE,this,this,0,0);
	
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Print Message"),M_PRINT_MESSAGE,this,this,'P',0,
							rsrc_utils.GetBitmapResource('BBMP',"Printer"));
	utils.AddMenuItem(aMenu,_("Page Setup…"),M_PAGE_SETUP_MESSAGE,be_app,be_app,0,0,
							rsrc_utils.GetBitmapResource('BBMP',"PageSetup"));
	aMenu->AddSeparatorItem();					
	utils.AddMenuItem(aMenu,_("Close"),B_QUIT_REQUESTED,this,this,'W',0);
	utils.AddMenuItem(aMenu,_("Quit"),B_QUIT_REQUESTED,be_app,be_app,'Q',0);
	
    menubar->AddItem( aMenu ); 
    //  Edit
   	aMenu = new BMenu(_("Edit"));
   	utils.AddMenuItem(aMenu,_("Undo"),B_UNDO,this,this,'Z',0);
   	aMenu->AddSeparatorItem();
   	utils.AddMenuItem(aMenu,_("Cut"),B_CUT,this,this,'X',0);
   	utils.AddMenuItem(aMenu,_("Copy"),B_COPY,this,this,'C',0);
   	utils.AddMenuItem(aMenu,_("Paste"),B_PASTE,this,this,'V',0);
   	aMenu->AddSeparatorItem();
   	utils.AddMenuItem(aMenu,_("Select All"),B_SELECT_ALL,this,this,'A',0);
   	menubar->AddItem(aMenu);
   	// Mail
	aMenu = new BMenu(_("Mail"));
	utils.AddMenuItem(aMenu,_("Send Now"),M_SEND_NOW,this,this,'M',0,
					rsrc_utils.GetBitmapResource('BBMP',"Send"));
	utils.AddMenuItem(aMenu,_("Send Later"),M_SEND_LATER,this,this,'L',0,
					rsrc_utils.GetBitmapResource('BBMP',"Send Later"));
	menubar->AddItem( aMenu );
	
	aMenu = new BMenu(_("Message"));
	subMenu = new BMenu(_("Enclosure"));
	utils.AddMenuItem(subMenu,_("Add Enclosure…"),M_ADD_ENCLOSURE,this,this,'A',B_SHIFT_KEY);
	utils.AddMenuItem(subMenu,_("Remove Enclosure"),M_DEL_ENCLOSURE,this,this,0,0);
	aMenu->AddItem(subMenu);
	subMenu = new BMenu(_("Priority"));
	subMenu->SetRadioMode(true);
	utils.AddMenuItem(subMenu,_("Highest"),(uint32)0,this,this);
	utils.AddMenuItem(subMenu,_("High"),(uint32)0,this,this);
	utils.AddMenuItem(subMenu,_("Normal"),(uint32)0,this,this);
	utils.AddMenuItem(subMenu,_("Low"),(uint32)0,this,this);
	utils.AddMenuItem(subMenu,_("Lowest"),(uint32)0,this,this);
	item = subMenu->FindItem(_("Normal"));
	if(item)
		item->SetMarked(true);
	aMenu->AddItem(subMenu);
	aMenu->AddSeparatorItem();
	subMenu = new BMenu(_("Word Wrap"));
	subMenu->SetRadioMode(true);
	utils.AddMenuItem(subMenu,_("Soft Wrap"),(uint32)0,this,this);
	utils.AddMenuItem(subMenu,_("Hard Wrap"),(uint32)0,this,this);
	aMenu->AddItem(subMenu);
	item = subMenu->FindItem(_("Hard Wrap"));
	if(item)
		item->SetMarked(true);
	
	menubar->AddItem( aMenu );
	
	
	aMenu = new BMenu(_("Signature"));
	
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Signatures");

	AddChildItem(aMenu,path.Path(),M_ADD_SIGNATURE,true);
	menubar->AddItem( aMenu );
	
    AddChild(menubar);
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HWriteWindow::InitGUI()
{
	int16 mode;
	((HApp*)be_app)->Prefs()->GetData("toolbar_mode",&mode);
	const int32 kToolbarHeight = (mode == 0)?50:30;
	
	HPrefs *prefs = ((HApp*)be_app)->Prefs();
	bool bValue;
	BRect rect = Bounds();
	rect.top += KeyMenuBar()->Bounds().Height() + kToolbarHeight;
	prefs->GetData("expand_addr",&bValue);
	
	rect.bottom = rect.top;
	rect.bottom += (bValue)?130:50;
	
	AddChild((fTopView = new HAddressView(rect)));
	fTopView->InitGUI();
	ArrowButton *button = cast_as(fTopView->FindView("addr_arrow"),ArrowButton);
	button->SetState((int32)bValue);
	fTopView->EnableJump(!bValue);
	
	rect.OffsetBy(0,rect.Height()+1);
	prefs->GetData("expand_enclosure",&bValue);

	rect.bottom = rect.top;
	rect.bottom += (bValue)?80:20;
	
	AddChild((fEnclosureView = new HEnclosureView(rect)));
	fEnclosureView->InitGUI();
	button = cast_as(fEnclosureView->FindView("enc_arrow"),
										ArrowButton);
	button->SetState((int32)bValue);

	
	rect.OffsetBy(0,rect.Height()+1);
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = Bounds().bottom - B_H_SCROLL_BAR_HEIGHT;
	fTextView = new HMailView(rect,false,NULL);
	
	BScrollView *scroll = new BScrollView("scroll",fTextView,B_FOLLOW_ALL,
										B_WILL_DRAW,true,true);
	AddChild(scroll);
	fTextView->SetDoesUndo(true);
	
	/********** Toolbarの追加 ***********/
	BRect toolrect = Bounds();
	toolrect.top += (KeyMenuBar()->Bounds()).Height();
	toolrect.bottom = toolrect.top + kToolbarHeight;
	toolrect.right += 2;
	toolrect.left -= 1;
	ResourceUtils utils;
	HToolbar *toolbox = new HToolbar(toolrect,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	if(kToolbarHeight == 50)
		toolbox->UseLabel(true);
	toolbox->AddButton("Send",utils.GetBitmapResource('BBMP',"Send"),
					new BMessage(M_SEND_NOW),"Send");
	toolbox->AddButton("Later",utils.GetBitmapResource('BBMP',"Send Later"),
					new BMessage(M_SEND_LATER),"Send Later");
	toolbox->AddSpace();
	toolbox->AddButton("Attach",utils.GetBitmapResource('BBMP',"Enclosure"),
					new BMessage(M_ADD_ENCLOSURE),"Add Enclosure");
	
	
	AddChild(toolbox);
	
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HWriteWindow::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	// Send Now
	case M_SEND_NOW:
	{
		entry_ref ref;
		BListView *list= cast_as(fEnclosureView->FindView("listview"),BListView);
		int32 count = list->CountItems();
		bool multipart = (count == 0)?false:true;
		fSent = true;
		if( SaveMail(false,ref,multipart) == B_OK)
		{
			WriteReplyStatus();
			
			BMessage msg(M_CREATE_MAIL);
			HMailItem *item = new HMailItem(ref);
			msg.AddPointer("pointer",item);
			msg.AddBool("send",true);
			((HApp*)be_app)->MainWindow()->PostMessage(&msg);
			PostMessage(B_QUIT_REQUESTED);
			RemoveDraft();
		}
		break;
	}
	// Send Later
	case M_SEND_LATER:
	{	
		fSent = true;
		entry_ref ref;
		BListView *list= cast_as(fEnclosureView->FindView("listview"),BListView);
		int32 count = list->CountItems();
		bool multipart = (count == 0)?false:true;
		
		if( SaveMail(false,ref,multipart) == B_OK)
		{
			WriteReplyStatus();
			
			BMessage msg(M_CREATE_MAIL);
			HMailItem *item = new HMailItem(ref);
			msg.AddPointer("pointer",item);
			msg.AddBool("send",false);
			((HApp*)be_app)->MainWindow()->PostMessage(&msg);	
			PostMessage(B_QUIT_REQUESTED);
			RemoveDraft();
		}
		break;
	}
	// Open draft
	case M_OPEN_DRAFT:
	{
		entry_ref ref;
		if(message->FindRef("refs",&ref) == B_OK)
			OpenDraft(ref);
		break;
	}
	// Save as draft
	case M_SAVE_DRAFT:
		SaveAsDraft();
		break;
	// Open template
	case M_OPEN_TEMPLATE:
	{
		entry_ref ref;
		if(message->FindRef("refs",&ref) == B_OK)
			OpenTemplate(ref);
		break;
	}
	// Save as template
	case M_SAVE_TEMPLATE:
		SaveAsTemplate();
		break;
	// Modified
	case M_MODIFIED:
	{
		BTextControl *control;
		if(message->FindPointer("pointer",(void**)&control) == B_OK)
		{
			// Subject
			if( ::strcmp(control->Label(),_("Subject:")) == 0)
			{
				const char *text = control->Text();
				if( ::strlen(text) > 0)
					SetTitle(text);
				else
					SetTitle(_("New Message"));
			}else if(!fSkipComplete&&::strcmp(control->Name(),"from") != 0){
				BTextView *view = control->TextView();
				const char* text = view->Text();
				int32 start,end,len;
				len = strlen(text);
				view->GetSelection(&start,&end);
				if(start != end)
					break;
				BString addr;
				for(int32 i = len;i >= 0;i--)
				{
					if(text[i] == ',')
						break;
					addr.Insert(text[i],1,0);	
				}
				int32 addr_len =addr.Length()-1;
				if(addr_len < 1)
					break;
				
				BMessage addr_list;
				//PRINT(("Address:%s %d\n",addr.String(),addr_len ));
				
				fTopView->FindAddress(addr.String(),addr_list);
				
				int32 count;
				type_code type;
				addr_list.GetInfo("address",&type,&count);
				if(count > 0)
				{
					const char* complete;
					addr_list.FindString("address",0,&complete);
					view->Insert(complete+addr_len);
					view->Select(start,start+strlen(complete));
					//PRINT(("Find:%s\n",complete ));
					fSkipComplete = true;
				}
			}else{
				fSkipComplete = false;
			}
		}
		break;
	}
	case M_ADDR_MSG:
	{
		BTextControl *control;
		if(message->FindPointer("pointer",(void**)&control) == B_OK)
		{
			const char* addr;
			message->FindString("email",&addr);
			
			if(!control)
				break;
			int32 start,end;
		
			control->TextView()->GetSelection(&start,&end);
			if(start != 0)
				control->TextView()->Insert(",");
			control->TextView()->Insert(addr);
			
		}
		break;
	}
	// Add Signature
	case M_ADD_SIGNATURE:
	{
		entry_ref ref;
		if(message->FindRef("refs",&ref) != B_OK)
			return;
		
		BFile file(&ref,B_READ_ONLY);
		if(file.InitCheck() != B_OK)
			return;
		off_t size;
		file.GetSize(&size);
		char *buf = new char[size+1];
		size = file.Read(buf,size);
		buf[size] = '\0';
		int32 length = strlen(buf);
		int32 offset = fTextView->TextLength();
		fTextView->Insert(offset,buf,length);
		delete[] buf;
		PRINT(("ADD SIGNATURE\n"));		
		break;
	}
	// Add enclosure
	case M_ADD_ENCLOSURE:
	{
		if(!fFilePanel)
		{
			fFilePanel = new BFilePanel();
			fFilePanel->SetTarget(this);
		}
		fFilePanel->Show();
		break;
	}
	// RefsReceived
	case B_REFS_RECEIVED:
	{
		int32 count;
		type_code type;
		message->GetInfo("refs",&type,&count);
		for(int32 i = 0;i < count;i++)
		{
			entry_ref ref;
			if(message->FindRef("refs",i,&ref) == B_OK)
				fEnclosureView->AddEnclosure(ref);
		}
		break;
	}
	// Del enclosure
	case M_DEL_ENCLOSURE:
	{
		BListView *list= cast_as(fEnclosureView->FindView("listview"),BListView);
		int32 sel = list->CurrentSelection();
		if( sel < 0)
			break;
		fEnclosureView->RemoveEnclosure(sel);
		break;
	}
	// Expand enclosure view
	case M_EXPAND_ENCLOSURE:
	{	
		BRect rect = fEnclosureView->Bounds();
		BScrollView *view = cast_as(FindView("scroll"),BScrollView);
		bool expand = (rect.Height() == 20)?true:false;
		const int32 kEnclosureHeight = (expand)?60:-60;
		
		
		fEnclosureView->ResizeBy(0,kEnclosureHeight);
		view->ResizeBy(0,-kEnclosureHeight);
		view->MoveBy(0,kEnclosureHeight);
		//fTextView->ResizeBy(0,-kEnclosureHeight);
		break;
	}
	// Expand address view
	case M_EXPAND_ADDRESS:
	{
		BScrollView *view = cast_as(FindView("scroll"),BScrollView);
		BRect rect = fTopView->Bounds();
		
		bool expand = (rect.Height() == 50)?true:false;
		const int32 kAddrHeight = (expand)?80:-80;
		
		fTopView->ResizeBy(0,kAddrHeight);
		fEnclosureView->MoveBy(0,kAddrHeight);
		view->ResizeBy(0,-kAddrHeight);
		view->MoveBy(0,kAddrHeight);
		//fTextView->ResizeBy(0,-kAddrHeight);
		fTopView->EnableJump(!expand);
		break;
	}
	// Print message
	case M_PRINT_MESSAGE:
	{
		BMessage msg(*message);
		msg.AddString("job_name",Title());
		msg.AddPointer("view",fTextView);
		be_app->PostMessage(&msg);
		break;
	}
	// Edit menus
	case B_CUT:
	case B_PASTE:
	case B_COPY:
	case B_SELECT_ALL:
	case B_UNDO:
	{
		BView *view = CurrentFocus();
		if(view)
			PostMessage(message,view);
		break;
	}
	default:
		BWindow::MessageReceived(message);
	}
}


/***********************************************************
 * MenusBeginning
 ***********************************************************/
void
HWriteWindow::MenusBeginning()
{
	BMenuItem *item;
	item = KeyMenuBar()->FindItem(M_DEL_ENCLOSURE);
	BListView *list= cast_as(fEnclosureView->FindView("listview"),BListView);
	if(list)
	{
		item->SetEnabled((list->CurrentSelection() <0 )?false:true);
	}
	int32 start,end;
	
	// Cut & Copy
	BTextControl *ctrl(NULL);
	BView *focus = CurrentFocus();
	if(focus == fTextView)
	{
		fTextView->GetSelection(&start,&end);
		if(start != end)
		{
			KeyMenuBar()->FindItem(B_CUT)->SetEnabled(true);
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(true);
		} else {
			KeyMenuBar()->FindItem(B_CUT)->SetEnabled(false);
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(false);
		}
	}else if((ctrl = fTopView->FocusedView()))
	{
		BTextView *textview = ctrl->TextView();
		textview->GetSelection(&start,&end);
		
		if(start != end)
		{
			KeyMenuBar()->FindItem(B_CUT)->SetEnabled(true);
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(true);
		} else {
			KeyMenuBar()->FindItem(B_CUT)->SetEnabled(false);
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(false);
		}
	}else{
		KeyMenuBar()->FindItem(B_CUT)->SetEnabled(false);
		KeyMenuBar()->FindItem(B_COPY )->SetEnabled(false);
	}
	// Undo
	item = KeyMenuBar()->FindItem(B_UNDO);
	if(fTextView->DoesUndo())
	{
		item->SetEnabled(true);
		bool redo;
		switch(fTextView->UndoState(&redo))
		{
		case B_UNDO_UNAVAILABLE:
			item->SetLabel(_("Can't Undo"));
			item->SetEnabled(false);
			break;
		case B_UNDO_TYPING:
			item->SetLabel(redo?_("Redo Typing"):_("Undo Typing"));
			break;
		case B_UNDO_CUT:
			item->SetLabel(redo?_("Redo Cut"):_("Undo Cut"));
			break;
		case B_UNDO_PASTE:
			item->SetLabel(redo?_("Redo Paste"):_("Undo Paste"));
			break;
		case B_UNDO_CLEAR:
			item->SetLabel(redo?_("Redo Clear"):_("Undo Clear"));
			break;
		case B_UNDO_DROP:
			item->SetLabel(redo?_("Redo Drop"):_("Undo Drop"));
			break;
		}
	}else{
		item->SetLabel(_("Can't Undo"));
		item->SetEnabled(false);
	}
	// Paste
	BMessage *clip = NULL;
   	if(be_clipboard->Lock())
   	{
   		clip = be_clipboard->Data();
   	
   		if(clip == NULL)
   			KeyMenuBar()->FindItem(B_PASTE)->SetEnabled(false);
   		else{
   			type_code type;
   			int32 count;
   			clip->GetInfo("text/plain",&type,&count);
   			if(count != 0)
   				KeyMenuBar()->FindItem(B_PASTE)->SetEnabled(true);
   			else
   				KeyMenuBar()->FindItem(B_PASTE)->SetEnabled(false);
   		}
   		be_clipboard->Unlock();
   	}
}

/***********************************************************
 * WriteReplyStatus
 ***********************************************************/
void
HWriteWindow::WriteReplyStatus()
{
	if(!fReplyFile)
		return;
	BString status;
	fReplyFile->ReadAttrString(B_MAIL_ATTR_STATUS,&status);
	if(fReply)
		status = "Replied";
	else if(fForward)
		status = "Forwarded";
	fReplyFile->WriteAttrString(B_MAIL_ATTR_STATUS,&status);
	
	fReplyItem->RefreshStatus();
}
/***********************************************************
 * SaveMail
 ***********************************************************/
status_t
HWriteWindow::SaveMail(bool send_now,entry_ref &ref,bool is_multipart)
{
	BString to(fTopView->To());
	if(to.Length() == 0)
	{
		(new BAlert("",_("To field is empty"),_("OK"),NULL,NULL,
								B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return B_ERROR;
	}

	BString subject(fTopView->Subject());
	if(subject.Length() == 0)
		subject = "Untitled";
	BString encoded_subject(subject);
	
	Encoding encode;
	int32 encoding;
	((HApp*)be_app)->Prefs()->GetData("encoding",&encoding);
	encode.UTF82Mime(encoded_subject,encoding);
	
	BString cc(fTopView->Cc());
	BString bcc(fTopView->Bcc());
	BString from("");
	BString content;
	if(!IsHardWrap())
		content = fTextView->Text();	
	else
		fTextView->GetHardWrapedText(content);
	// Get out box
	BPath path;
	::find_directory(B_USER_DIRECTORY,&path);
	path.Append("mail");
	path.Append(OUTBOX);
	
	BString filename = subject;
	filename.ReplaceAll("/","_");
	filename.ReplaceAll(":","_");
	
	path.Append( filename.String() );
	
	BFile file;
	status_t err = B_ERROR;
	int32 i = 1;
	BString tmpsubject("");
	while(err != B_OK)
	{
		err = file.SetTo(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_FAIL_IF_EXISTS);
		if(err == B_OK)
			break;
		path.GetParent(&path);
		tmpsubject.SetTo( filename );
		BString index("_");
		index <<i++;
		tmpsubject << index;
		if(i > 50)
		{
			tmpsubject = "";
			tmpsubject	<<time(NULL);		
		}
		path.Append(tmpsubject.String());
	}
	::get_ref_for_path(path.Path(),&ref);
	// make smtp server
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	path.Append(fTopView->AccountName());
	
	BFile AccountFile(path.Path(),B_READ_ONLY);
	BString smtp_host(""),reply;
	if(AccountFile.InitCheck() == B_OK)
	{
		BMessage msg;
		msg.Unflatten(&AccountFile);
	
		if(msg.FindString("smtp_host",&smtp_host) != B_OK)
			smtp_host = "";
		if(msg.FindString("reply_to",&reply) != B_OK)
			reply = "";
	}
	// make date 
	time_t now = time(NULL);
	char *timeBuf = new char[64];
	struct tm *lt = localtime(&now);
	int dd, hh, mm, ss, yyyy;
	char day[4], mon[4];
	::sscanf(asctime(lt), "%3s %3s %d %d:%d:%d %d\n",
	       day, mon, &dd, &hh, &mm, &ss, &yyyy);
#if __INTEL__
	::snprintf(timeBuf, 64,"%s, %d %s %d %02d:%02d:%02d %s",
		   day, dd, mon, yyyy, hh, mm, ss, TimeZoneOffset(&now));
#else
	::sprintf(timeBuf,"%s, %d %s %d %02d:%02d:%02d %s",
		   day, dd, mon, yyyy, hh, mm, ss, TimeZoneOffset(&now));
#endif
	// make header
	BString header("");
	from = fTopView->From();
	BString encoded_from(from),encoded_to(to),encoded_cc(cc),encoded_bcc(bcc);
	encode.UTF82Mime(encoded_from,encoding);
	encode.UTF82Mime(encoded_to,encoding);
	encode.UTF82Mime(encoded_cc,encoding);
	encode.UTF82Mime(encoded_bcc,encoding);
	
	header << "To: " << encoded_to << "\n";
	if(cc.Length() !=0 )
		header << "Cc: " << encoded_cc << "\n";
	if(bcc.Length() !=0 )
		header << "BCc: " << encoded_bcc << "\n";
	header << "Subject: " << encoded_subject << "\n";
	header << "From: " << from << "\n";
	if(reply.Length() != 0)
		header << "Reply-To: " << reply << "\n";
	header << "Date: " << timeBuf << "\n";
	header << "MIME-Version: 1.0" << "\n";
	header << "X-Mailer: " << XMAILER << "\n";
	// Get priority
	int32 priority = 3;
	BMenu *subMenu = KeyMenuBar()->SubmenuAt(3);
	subMenu = subMenu->SubmenuAt(1);
	BMenuItem *item = subMenu->FindMarked();
	if(::strcmp(item->Label(),"Highest") == 0)
		priority = 1;
	else if(::strcmp(item->Label(),"High") == 0)
		priority = 2;
	else if(::strcmp(item->Label(),"Low") == 0)
		priority = 4;
	else if(::strcmp(item->Label(),"Lowest") == 0)
		priority = 5;
	header << "X-Priority: " << priority << "\n";
	BString boundary(kBoundary);
	boundary <<(int32)time(NULL) << "_=----";
	if(!is_multipart)
	{
		header << "Content-Type: text/plain; charset=";
		BString charset;
		FindCharset(encoding,charset);
		header << charset << "\n";
	}else{
		header << "Content-Type: multipart/mixed;" << "\n";
		header << "\t" << "boundary=\"" << boundary << "\"\n";
	}
	header << "\n";
	delete[] timeBuf;
	// write all to the file
	BString mime("1.0");
	// for japanese support
	content << " ";
	//
	encode.ConvertFromUTF8(content,encoding);
	if(is_multipart)
	{
		BString part("");
		boundary.Insert("--",0);
		part << "This is a multi-part message in MIME format.\n\n";
		part << boundary <<"\n";
		part << "Content-Type: text/plain; charset=";
		BString charset;
		FindCharset(encoding,charset);
		part << charset << "\n";
		
		part << "\n" << content << "\n";
		content = "";
		WriteAllPart(part,boundary.String());
		part << boundary << "--\n";
		content = part;
	}
		
	encode.ConvertReturnCode(header,M_CRLF);
	encode.ConvertReturnCode(content,M_CRLF);
	
	BString all_content = header;
	all_content << content;
	
	file.Write(all_content.String(),all_content.Length());
	file.SetSize( all_content.Length() );
	
	file.WriteAttr(B_MAIL_ATTR_STATUS,B_STRING_TYPE,0,"Pending",8);
	//file.WriteAttrString(B_MAIL_ATTR_PRIORITY,&priority);
	file.WriteAttrString(B_MAIL_ATTR_TO,&to);
	file.WriteAttrString(B_MAIL_ATTR_CC,&cc);
	file.WriteAttrString(B_MAIL_ATTR_BCC,&bcc);
	file.WriteAttrString(B_MAIL_ATTR_FROM,&from);
	file.WriteAttrString(B_MAIL_ATTR_SUBJECT,&subject);
	file.WriteAttrString(B_MAIL_ATTR_MIME,&mime);
	file.WriteAttrString(B_MAIL_ATTR_REPLY,&reply);
	file.WriteAttr(B_MAIL_ATTR_ATTACHMENT,B_BOOL_TYPE,0,&is_multipart,sizeof(bool));
	BString attrPriority;
	attrPriority << priority;
	switch(priority)
	{
	case 1:
		attrPriority << " (Highest)";
		break;
	case 2:
		attrPriority << " (High)";
		break;
	case 3:
		attrPriority << " (Normal)";
		break;
	case 4:
		attrPriority << " (Low)";
		break;
	case 5:
		attrPriority << " (Lowest)";
		break;
	}
	file.WriteAttrString(B_MAIL_ATTR_PRIORITY,&attrPriority);
	int32 header_len = header.Length();
	int32 content_len = content.Length();
	//PRINT(("header:%d, content%d\n",header_len,content_len));
	file.WriteAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&header_len,sizeof(int32));
	file.WriteAttr(B_MAIL_ATTR_CONTENT,B_INT32_TYPE,0,&content_len,sizeof(int32));	
	file.WriteAttr(B_MAIL_ATTR_WHEN,B_TIME_TYPE,0,&now,sizeof(time_t));
	file.WriteAttrString(B_MAIL_ATTR_SMTP_SERVER,&smtp_host);
	BNodeInfo ninfo(&file);
	ninfo.SetType("text/x-email");
	
	return B_OK;
}

/***********************************************************
 * WriteAllPart
 ***********************************************************/
void
HWriteWindow::WriteAllPart(BString &out,const char* boundary)
{
	BString str("");
	BListView *list = cast_as(fEnclosureView->FindView("listview"),BListView);
	int32 count = list->CountItems();
	
	for(int32 i = 0;i < count;i++)
	{
		HEnclosureItem *item = cast_as(list->ItemAt(i),HEnclosureItem);
		if(!item)
		{
			PRINT(("The item is invalid\n"));
			continue;
		}
		entry_ref ref = item->Ref();
		
		BFile file(&ref,B_READ_ONLY);
		if(file.InitCheck() != B_OK)
		{
			PRINT(("File not found\n"));
			continue;
		}
		off_t size;
		file.GetSize(&size);
		char *buf = new char[size+1];
		char *outBuf = new char[size*2];
		size = file.Read(buf,size);
		buf[size] = '\0';
		size = encode_base64(outBuf,buf,size);
		outBuf[size] = '\0';
		char name[B_FILE_NAME_LENGTH+1];
		BEntry entry(&ref);
		entry.GetName(name);
		BNodeInfo info(&file);
		char type[B_MIME_TYPE_LENGTH+1];
		type[0] = '\0';
		if( info.GetType(type) != B_OK)
		{
			BPath path(&ref);
			BString cmd("/bin/mimeset \"");
			cmd << path.Path() << "\"";
			::system(cmd.String());
			if(info.GetType(type) != B_OK)
				type[0] = '\0';
		}
		if(!type)
			strcpy(type,"application/octet-stream");
		str << boundary << "\n";
		str << "Content-Type: " << type << "; ";
		str << "name=\"" << name << "\"\n";
		str << "Content-Disposition: attachment\n";
		str << "Content-Transfer-Encoding: base64\n\n";
		str << outBuf << "\n";
		delete[] buf;
		delete[] outBuf;
	} 
	out << str;
}

/***********************************************************
 * IsHardWrap
 ***********************************************************/
bool
HWriteWindow::IsHardWrap()
{
	BMenu *subMenu = KeyMenuBar()->SubmenuAt(3);
	subMenu = subMenu->SubmenuAt(3);
	
	BMenuItem *item = subMenu->FindMarked();
	if(item)
		return true;
	if(::strcmp(item->Label(),_("Hard Wrap")) == 0)
		return true;
	return false;
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HWriteWindow::QuitRequested()
{
	if(fTextView->TextLength() > 0 && !fSent)
	{
		// **GR changed to standard Be-Mail-Alert
		BAlert *alert= new BAlert("Save_Alert",_("Save as draft before closing?"),
						_("Don't Save"),
						_("Cancel"),
						_("Save"),
						B_WIDTH_AS_USUAL,
						B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->SetShortcut(0, 'd');
        alert->SetShortcut(1, 'c');
        alert->SetShortcut(2, B_ENTER);
        
        int32 button = alert->Go();
		switch(button)
		{
		case 2:	
			PostMessage(M_SAVE_DRAFT);
			return false;
		case 1:
			return false;
		default:
			break;
		}
	}
	
	Unlock();
	fTextView->StopLoad();
	Lock();
	
	HPrefs *prefs = ((HApp*)be_app)->Prefs();
	// Save view's enpansion
	bool expand = (fTopView->Bounds().Height() == 50)?false:true;
	prefs->SetData("expand_addr",expand);
	
	expand = (fEnclosureView->Bounds().Height() == 20)?false:true;
	prefs->SetData("expand_enclosure",expand);
	// Save Window Rect
	prefs->SetData("write_window_rect",Frame());
	return BWindow::QuitRequested();
}

/***********************************************************
 * TimeZoneOffset
 ***********************************************************/
char *
HWriteWindow::TimeZoneOffset(time_t *now)
{
	static char offset_string[6];
	struct tm gmt, *lt;
	int off;
	char sign = '+';

	gmt = *gmtime(now);
	lt = localtime(now);
	off = (lt->tm_hour - gmt.tm_hour) * 60 + lt->tm_min - gmt.tm_min;
	if (lt->tm_year < gmt.tm_year)
		off -= 24 * 60;
	else if (lt->tm_year > gmt.tm_year)
		off += 24 * 60;
	else if (lt->tm_yday < gmt.tm_yday)
		off -= 24 * 60;
	else if (lt->tm_yday > gmt.tm_yday)
		off += 24 * 60;
	if (off < 0) {
		sign = '-';
		off = -off;
	}
	if (off >= 24 * 60)	
		off = 23 * 60 + 59;	
	sprintf(offset_string, "%c%02d%02d", sign, off / 60, off % 60);

	return (offset_string);
}

/***********************************************************
 * OpenTemplate
 ***********************************************************/
void
HWriteWindow::OpenTemplate(entry_ref ref)
{
	BFile file(&ref,B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	off_t size;
	file.GetSize(&size);
	char *buf = new char[size+1];
	size = file.Read(buf,size);
	buf[size] = '\0';
	
	fTextView->SetText(buf);
}

/***********************************************************
 * SaveAsTemplate
 ***********************************************************/
void
HWriteWindow::SaveAsTemplate()
{
	BString subject(fTopView->Subject());
	if(subject.Length() == 0)
	{
		beep();
		(new BAlert("",_("Please enter subject"),_("OK"),NULL,NULL,
					B_WIDTH_AS_USUAL,B_IDEA_ALERT))->Go();
		return;
	}
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append(TEMPLATE_FOLDER);
	::create_directory(path.Path(),0777);
	path.Append(subject.String());
	
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_FAIL_IF_EXISTS);
	if(file.InitCheck() != B_OK)
	{
		beep();
		(new BAlert("",_("Same file exists"),_("OK"),NULL,NULL,
					B_WIDTH_AS_USUAL,B_IDEA_ALERT))->Go();
		return;
	}
	
	file.Write(fTextView->Text(),fTextView->TextLength());
	
	// add to template menu
	BMenu *menu = KeyMenuBar()->SubmenuAt(0)->SubmenuAt(3);
	if(menu)
	{
		entry_ref ref;
		::get_ref_for_path(path.Path(),&ref);
		BMessage *msg = new BMessage(M_OPEN_TEMPLATE);
		msg->AddRef("refs",&ref);
		menu->AddItem(new BMenuItem(path.Leaf(),msg));
	}
}

/***********************************************************
 * OpenDraft
 ***********************************************************/
void
HWriteWindow::OpenDraft(entry_ref ref)
{	
	BFile *file = new BFile(&ref,B_READ_ONLY);
	if(file->InitCheck() != B_OK)
	{
		delete file;
		return;
	}
	BString attr("");
	const char* kAttr_Name[] = {"cc","subject","to","bcc"};
	status_t err;
	// address and so on
	BTextControl *control;
	for(int32 i = 0;i < 4;i++)
	{
		PRINT(("%s\n",kAttr_Name[i]));
		err = file->ReadAttrString(kAttr_Name[i],&attr);
		control = cast_as(fTopView->FindView(kAttr_Name[i]),BTextControl);
		if(control && err == B_OK && attr.Length() >0)
			control->SetText(attr.String());
	}
	
	int32 index;
	file->ReadAttr("from",B_INT32_TYPE,0,&index,sizeof(int32));
	BMenuField *field = cast_as(fTopView->FindView("FromMenu"),BMenuField);
	if(field)
	{
		BMenuItem *item =field->Menu()->ItemAt(index);
		if(item)
		{
			BMessage msg(M_ACCOUNT_CHANGE);
			msg.AddString("name",item->Label());
			PostMessage(&msg,fTopView);
		}
	}
	
	// enclosures
	entry_ref attach_ref;
	BMessage msg;
	msg.Unflatten(file);
	const char *content;
	msg.FindString("content",&content);
	fTextView->SetText(content);
	
	int32 count;
	type_code type;
	msg.GetInfo("enclosure",&type,&count);
	for(int32 i = 0;i < count;i++)
	{
		if(msg.FindRef("enclosure",i,&attach_ref) == B_OK)
			fEnclosureView->AddEnclosure(attach_ref);
	}
	
	delete fDraftEntry;
	fDraftEntry = new BEntry(&ref);
	delete file;
	//fTextView->LoadMessage(file,false,false,NULL);
}

/***********************************************************
 * SaveAsDraft
 ***********************************************************/
void
HWriteWindow::SaveAsDraft()
{
	BString subject(fTopView->Subject());
	if(subject.Length() == 0)
	{
		beep();
		(new BAlert("",_("Please enter subject"),_("OK"),NULL,NULL,
					B_WIDTH_AS_USUAL,B_IDEA_ALERT))->Go();
		return;
	}
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append(DRAFT_FOLDER);
	::create_directory(path.Path(),0777);
	path.Append(subject.String());
	
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
	if(file.InitCheck() != B_OK)
	{
		beep();
		(new BAlert("",_("Could not create file"),_("OK"),NULL,NULL,
					B_WIDTH_AS_USUAL,B_IDEA_ALERT))->Go();
		return;
	}
	
	
	
	file.WriteAttrString("subject",&subject);
	BString str = fTopView->To();
	file.WriteAttrString("to",&str);
	str = fTopView->Cc();
	file.WriteAttrString("cc",&str);
	str= fTopView->Bcc();
	file.WriteAttrString("bcc",&str);
	
	BMenuField *field = cast_as(fTopView->FindView("FromMenu"),BMenuField);
	BMenuItem *item = field->Menu()->FindMarked();
	if(item)
	{
		int32 account = field->Menu()->IndexOf(item);
		file.WriteAttr("from",B_INT32_TYPE,0,&account,sizeof(int32));
	}

	BMessage msg;
	msg.AddString("content",fTextView->Text());
	
	BListView *listview = cast_as(fEnclosureView->FindView("listview")
									,BListView);
	int32 count = listview->CountItems();
	for(int32 i = 0;i < count;i++)
	{
		HEnclosureItem *item =cast_as(listview->ItemAt(i),HEnclosureItem);
		if(!item)
			continue;
		entry_ref ref = item->Ref();
		
		msg.AddRef("enclosure",&ref);
	}
	msg.Flatten(&file);
	PostMessage(B_QUIT_REQUESTED);
	fSent = true;
}

/***********************************************************
 * RemoveDraft
 ***********************************************************/
void
HWriteWindow::RemoveDraft()
{
	// remove draft
	if(fDraftEntry)
	{
		entry_ref ref;
		fDraftEntry->GetRef(&ref);
	
		TrackerUtils().MoveToTrash(ref);
		delete fDraftEntry;
		fDraftEntry = NULL;
	}
}

/***********************************************************
 * AddChildItem
 ***********************************************************/
void
HWriteWindow::AddChildItem(BMenu *menu,const char* path,int32 what,bool mod)
{
	BDirectory dir(path);
	BEntry entry;
	status_t err = B_OK;
	char name[B_FILE_NAME_LENGTH+1];
	char c = '1';
	entry_ref ref;
	while(err == B_OK)
	{
		err = dir.GetNextEntry(&entry);
		if(err != B_OK)
			break;
		entry.GetRef(&ref);
		entry.GetName(name);
		BMessage *msg = new BMessage(what);
		msg->AddRef("refs",&ref);
		menu->AddItem(new BMenuItem(name,msg,(mod)?c++:0,0));
	}
}

/***********************************************************
 * FindCharset
 ***********************************************************/
void
HWriteWindow::FindCharset(int32 conversion,BString &charset)
{
	charset += "\"";
	charset += Encoding().FindCharset(conversion);
	charset += "\"";
}

/***********************************************************
 * DispatchMessage
 ***********************************************************/
void
HWriteWindow::DispatchMessage(BMessage *message,BHandler *handler)
{
	BView *child = cast_as(handler,BView);
	
	BView *parent(NULL);
	if(child)
		parent = child->Parent();
	
	if(message->what == B_KEY_DOWN 
		&& parent
		&& is_kind_of(parent,BTextControl))
	{
		BView *parent = child->Parent();
		if(!parent
			&&::strcmp(parent->Name(),"subject") == 0 
			&&::strcmp(parent->Name(),"from") == 0)
			goto end;
		const char* bytes;
		int32 modifiers;
		
		message->FindInt32("modifiers",&modifiers);
		message->FindString("bytes",&bytes);
		
		if(bytes[0] == B_BACKSPACE || bytes[0] == B_DELETE)
		{
			fSkipComplete = true;
		}
	}
end:
	BWindow::DispatchMessage(message,handler);
}
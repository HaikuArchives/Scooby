#include "HPopClientView.h"
#include "PopLooper.h"
#include "HApp.h"
#include "Encoding.h"
#include "HWindow.h"
#include "ExtraMailAttr.h"
#include "TrackerUtils.h"
#include "TrackerString.h"
#include "Utilities.h"

#include <Window.h>
#include <stdio.h>
#include <stdlib.h>
#include <Debug.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <Directory.h>
#include <E-mail.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <Beep.h>
#include <ClassInfo.h>
#include <ctype.h>
#include <PopUpMenu.h>
#include <MenuItem.h>

#define MAIL_FOLDER "mail"

/***********************************************************
 * Constructor
 ***********************************************************/
HPopClientView::HPopClientView(BRect frame,
						const char* name)
	:HProgressBarView(frame,name)
	,fPopLooper(NULL)
	,fStartPos(0)
	,fStartSize(-1)
	,fRetrieve(0)
	,fIsRunning(false)
	,fPopServers(NULL)
	,fServerIndex(0)
	,fAccountName("")
	,fLastSize(0)
	,fMailCurrentIndex(0)
	,fMailMaxIndex(0)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
HPopClientView::~HPopClientView()
{
	if(fPopLooper)
		fPopLooper->PostMessage(B_QUIT_REQUESTED);
	if(fPopServers)
		delete fPopServers;
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HPopClientView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_POP_ABORT:
		Cancel();
		break;
	case H_ERROR_MESSAGE:
	{
		PRINT(("POP ERROR\n"));
		BString err_str(_("POP3 ERROR"));
		err_str += "\n";
		if(message->FindString("log"))
			err_str += message->FindString("log");
		beep();
		(new BAlert("",err_str.String(),"OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		if(fPopLooper)
			fPopLooper->PostMessage(B_QUIT_REQUESTED);
		fPopLooper = NULL;
		break;
	}
	case M_QUIT_FINISHED:
		StopBarberPole();
		StopProgress();
		if(!fPopServers)
			break;
		int32 count;
		type_code type;
		fPopServers->GetInfo("address",&type,&count);
		SetText("");
	
		if(fServerIndex < count)
		{
			// Connect next server
			Window()->PostMessage(M_POP_CONNECT,this);
		}else{
			// Quit pop session
			if(fPopLooper)
				fPopLooper->PostMessage(B_QUIT_REQUESTED);
			SetText("");
			fIsRunning = false;
			delete fPopServers;
			fPopServers = NULL;
			if(fGotMails)
				PlayNotifySound();
		}
		break;
	case M_POP_CONNECT:
	{
		fIsRunning = true;
		fCanUseUIDL = true;
		if(!fPopServers)
		{
			fGotMails = false;
			fPopServers = new BMessage(*message);
			fServerIndex = 0;
		}
		
		const char* address,*login,*password,*name;
		
		int16 port;
		
		if(fPopServers->FindString("address",fServerIndex,&address) != B_OK)
			break;
		if(fPopServers->FindInt16("port",fServerIndex,&port) != B_OK)
			port = 110;
		if(fPopServers->FindString("login",fServerIndex,&login) != B_OK)
			break;
		if(fPopServers->FindString("name",fServerIndex,&name) != B_OK)
			break;
		if(fPopServers->FindString("password",fServerIndex,&password) != B_OK)
			password= "";
		if(fPopServers->FindString("uidl",fServerIndex,&fUidl) != B_OK)
			fUidl = "";
		int16 proto;
		if(fPopServers->FindInt16("protocol_type",fServerIndex,&proto) != B_OK)
			proto= 0;
		fUseAPOP = (proto == 0)?false:true;
		if(fPopServers->FindInt32("delete_day",fServerIndex,&fDeleteDays) != B_OK)
			fDeleteDays = 0;
		if(fPopServers->FindInt16("retrieve",fServerIndex,&fRetrieve) != B_OK)
			fRetrieve = 0;
		PRINT(("Retrieve:%d\n",fRetrieve));
		fServerIndex++;
		fDeleteMails.MakeEmpty();
		fDeleteMails.what = H_DELETE_MESSAGE;
		PRINT(("Connect To %s\n",address));
		PopConnect(name,address,port,login,password);
		break;
	}
	// conect success
	case H_CONNECT_MESSAGE:
	{	
		SetText("Login" B_UTF8_ELLIPSIS);
		BMessage msg(H_LOGIN_MESSAGE);
		msg.AddString("login",fLogin);
		msg.AddString("password",fPassword);
		msg.AddBool("apop",fUseAPOP);
		if(fPopLooper)
			fPopLooper->PostMessage(&msg);
		break;
	}
	// login success
	case H_LOGIN_MESSAGE:
		SetText("UIDL" B_UTF8_ELLIPSIS);
		if(fPopLooper)
			fPopLooper->PostMessage(H_UIDL_MESSAGE);
		break;
	// list success
	case H_UIDL_MESSAGE:
	{
		BString list;
		BMessage msg(H_RETR_MESSAGE);
		
		if(message->FindString("list",&list) == B_OK)
		{	
			int32 startpos = 1;
			if(fUidl.Length() > 1)
			{
				if(list.FindFirst(fUidl) != B_ERROR)
					startpos = atoi(fUidl.String())+1;
				else
					startpos = 1;
			}
			
			if(list.Length() == 0)
			{
				if(fPopLooper)
					fPopLooper->PostMessage(B_QUIT_REQUESTED);
				fPopLooper = NULL;
				SetText("");
				break;
			}
			// make list
			char *buf = const_cast<char*>(list.String());
			char *p = strtok(buf,"\n");
			int size,index,count = 0;
			char uidl[1024];
			while(p)
			{
				if(strlen(p) <= 1)
					continue;
				if(fCanUseUIDL)
					::sscanf(p,"%d%s",&index,uidl);
				else
					::sscanf(p,"%d%d",&index,&size);
				if(index >= startpos)
				{
					msg.AddInt32("index",index);
					fUidl = "";
					if(fCanUseUIDL)
					{
						fUidl << index; 
						fUidl += " ";
						fUidl += uidl;
					}else{
						fUidl << index;
						fUidl += " ";
						fUidl << size;
						fUidl += "\r\n";
					}
					count++;
				}
				p = strtok('\0',"\n");
			}
			
			if(count==0)
			{
				if(fPopLooper)
					fPopLooper->PostMessage(B_QUIT_REQUESTED);
				fPopLooper = NULL;
				SetText("");
				break;
			}
			BString label("RETR [ ");
			
			fMailCurrentIndex = (startpos == 0)?1:startpos;
			fMailMaxIndex = count + fMailCurrentIndex - 1;
			label << fMailCurrentIndex << " / " << fMailMaxIndex << " ]";
			SetText(label.String());
			StopBarberPole();
			StartProgress();
			if(fPopLooper)
				fPopLooper->PostMessage(&msg);
		}else{
			// POP3 server is not support UIDL command
			fCanUseUIDL = false;
			PRINT(("UIDL not supported\n"));
			SetText("LIST" B_UTF8_ELLIPSIS);
			if(fPopLooper)
				fPopLooper->PostMessage(H_LIST_MESSAGE);
		}
		break;
	}
	// receive list
	case H_LIST_MESSAGE:
	{
		message->what = H_UIDL_MESSAGE;
		Window()->PostMessage(message,this);
		break;
	}
	// receive content
	case H_RETR_MESSAGE:
	{
		int32 index;
		const char* content;
		if(message->FindInt32("index",&index) == B_OK &&
			message->FindString("content",&content) == B_OK)
		{
			fMailCurrentIndex = index+1;
			entry_ref folder_ref;
			bool is_delete;
			SaveMail(content,&folder_ref,&is_delete);
	
			if(is_delete)
				fDeleteMails.AddInt32("index",index);
			
			if(!is_delete && message->FindBool("end"))
			{
				SetNextRecvPos(fUidl.String());
				if(fPopLooper)
					fPopLooper->PostMessage(B_QUIT_REQUESTED);
				fPopLooper = NULL;
				SetText("");
				break;
			}else if(is_delete && message->FindBool("end")){
				int32 count;
				type_code type;
				
				fDeleteMails.GetInfo("index",&type,&count);
				
				if(count > 0)
				{
					SetValue(fStartPos+1);
					fMailCurrentIndex = fMailMaxIndex;
					SetMaxValue(fMailMaxIndex);
					BString label("DEL [ ");
					label << fMailCurrentIndex << " / " << fMailMaxIndex << " ]";
					SetText(label.String() );
					if(fPopLooper)
						fPopLooper->PostMessage(&fDeleteMails);
					
				}else{
					if(fPopLooper)
						fPopLooper->PostMessage(B_QUIT_REQUESTED);
					fPopLooper = NULL;
					SetText("");
				}
				break;
			}
			
			SetValue(0);
			BString label("RETR [ ");
			label << fMailCurrentIndex << " / " << fMailMaxIndex << " ]";
			//PRINT(("%s\n",label.String() ));
			SetText(label.String() );
		}
		break;
	}
	// delete success
	case H_DELETE_MESSAGE:
	{
		int32 index;
		if(message->FindInt32("index",&index) == B_OK)
		{
			Update(1);
			fMailCurrentIndex = index-1;
			BString label("DEL [ ");
			label << fMailCurrentIndex << "/" << fMailMaxIndex << " ]";
			SetText(label.String());
			if(message->FindBool("end"))
			{
				if(fPopLooper)
					fPopLooper->PostMessage(B_QUIT_REQUESTED);
				fPopLooper = NULL;
				SetText("");
				SetNextRecvPos("");
			}
		}
		break;
	}
	// last success
	case H_LAST_MESSAGE:
		SetText("");
		if(fPopLooper)
			fPopLooper->PostMessage(B_QUIT_REQUESTED);
		break;
	// Set max size of progress bar
	case H_SET_MAX_SIZE:
	{
		int32 size;
		if(message->FindInt32("max_size",&size) != B_OK)
			break;
		SetMaxValue(size);
		SetValue(0);
		break;
	}
	// Receive received data size
	case H_RECEIVING_MESSAGE:
	{
		int32 size;
		if(message->FindInt32("size",&size) != B_OK)
			break;
		Update(size);
		break;
	}
	default:
		_inherited::MessageReceived(message);	
	}
	return;
}

/***********************************************************
 * PopConnect
 ***********************************************************/
void
HPopClientView::PopConnect(const char* name,
					const char* address,int16 port,
					const char* login,const char* pass)
{
	fAccountName = name;
	fLogin= login;
	fPassword = pass;
	
	StartBarberPole();
	if(fPopLooper)
		fPopLooper->PostMessage(B_QUIT_REQUESTED);
	fPopLooper = new PopLooper(this,Window());
	if(fPopLooper->Lock())
	{
		fPopLooper->InitBlackList();
		fPopLooper->Unlock();
	}
	BString label(_("Connecting to"));
	label += " ";
	label += address;
	label += B_UTF8_ELLIPSIS;
	SetText(label.String());
	BMessage msg(H_CONNECT_MESSAGE);
	msg.AddString("address",address);
	msg.AddInt16("port",port);
	fPopLooper->PostMessage(&msg);
}


/***********************************************************
 * GetHeaderParam
 ***********************************************************/
int32
HPopClientView::GetHeaderParam(BString &out,const char* content,int32 offset)
{
	int32 i = offset;
	if(out.Length() > 0)
		return i;
	if(content[i] == ' ')
		i++;
	while(1)
	{
		if(content[i] =='\n')
		{
			if(isalpha(content[i+1])|| content[i+1] == '\r'|| content[i+1] == '\n' )
				break;
			// skip first space character at the new line. 
			else if(content[i+1] == ' ' ||content[i+1] == '\t')
				i+=2;
		}
		if(content[i] != '\r' && content[i] != '\n' )
		 	out += content[i++];
		else
			i++;
	}
	return i;
}

/***********************************************************
 * SaveMail
 ***********************************************************/
void
HPopClientView::SaveMail(const char* all_content,
						entry_ref* folder_ref,
						bool *is_delete)
{
	fGotMails = true;

	BString header(""),subject(""),to(""),date(""),cc(""),from("")
			,priority(""),reply(""),mime("");
	Encoding encode;
	
	bool is_multipart = false;
	int32 org_len = ::strlen(all_content);
	// Probably deleted with Spam filter
	if(org_len == 0)
	{
		*is_delete = false;
		return;
	}
	//
	int32 header_len = 0;
	for(int32 i = 0;i < org_len;i++)
	{
		if(strncasecmp(&all_content[i],"Subject:",8) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(subject,all_content,i+8);
		else if(strncasecmp(&all_content[i],"Date:",5) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(date,all_content,i+5);
		else if(strncasecmp(&all_content[i],"Cc:",3) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(cc,all_content,i+3);
		else if(strncasecmp(&all_content[i],"To:",3) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(to,all_content,i+3);
		else if(strncasecmp(&all_content[i],"From:",5) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(from,all_content,i+5);
		else if(strncasecmp(&all_content[i],"X-Priority:",11) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(priority,all_content,i+11);
		else if(strncasecmp(&all_content[i],"Mime-Version:",13) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(mime,all_content,i+13);
		else if(strncasecmp(&all_content[i],"Reply-To:",9) == 0 && all_content[i-1] == '\n')
			i = GetHeaderParam(reply,all_content,i+9);
		else if(all_content[i] == '\r'||all_content[i] == '\n')
		{
			if(all_content[i-2] == '\r'||all_content[i-1] == '\n')
			{
				header_len = i+2;
				break;
			}
		}
	}
	
	header.Append(all_content,header_len);
	
	if(subject.Length() == 0)
		subject = "Untitled";
	if(strstr(header.String(),"Content-Type: multipart"))
		is_multipart = true;
	
	//PRINT(("From:%s\n",from.String()));
	encode.Mime2UTF8(from);
	//PRINT(("Decoded From:%s\n",from.String()));
	encode.Mime2UTF8(to);
	encode.Mime2UTF8(cc);
	encode.Mime2UTF8(reply);
	// convert mime subject to UTF8
	encode.Mime2UTF8(subject);
	
	// Filter mails
	BString folder_path;
	FilterMail(subject.String(),
				from.String(),
				to.String(),
				cc.String(),
				fAccountName.String(),
				reply.String(),
				folder_path);
	//PRINT(("path:%s\n",folder_path.String() ));
	
	// Save to disk
	BPath path = folder_path.String();
	::create_directory(path.Path(),0777);
	BDirectory destDir(path.Path());
	PRINT(("path:%s\n",path.Path() ));
	// create the e-mail file
	BFile file;
	
	if(TrackerUtils().SmartCreateFile(&file,&destDir,subject.String(),"_") != B_OK)
		PRINT(("Failed to save mail:%s",path.Leaf()));

	// file type first (or queries won't work)
	BNodeInfo ninfo(&file);
	ninfo.SetType("text/x-email");

	// write e-mail attributes
	file.Write(all_content,strlen(all_content));
	file.SetSize(strlen(all_content));
	
	file.WriteAttr(B_MAIL_ATTR_STATUS,B_STRING_TYPE,0,"New",4);
	file.WriteAttrString(B_MAIL_ATTR_PRIORITY,&priority);
	file.WriteAttrString(B_MAIL_ATTR_TO,&to);
	file.WriteAttrString(B_MAIL_ATTR_CC,&cc);
	file.WriteAttrString(B_MAIL_ATTR_FROM,&from);
	file.WriteAttrString(B_MAIL_ATTR_SUBJECT,&subject);
	file.WriteAttrString(B_MAIL_ATTR_REPLY,&reply);
	file.WriteAttrString(B_MAIL_ATTR_MIME,&mime);
	file.WriteAttrString(B_MAIL_ATTR_ACCOUNT,&fAccountName);
	file.WriteAttr(B_MAIL_ATTR_ATTACHMENT,B_BOOL_TYPE,0,&is_multipart,sizeof(bool));
	int32 content_len = strlen(all_content)-header_len;
	//PRINT(("header:%d, content%d\n",header_len,content_len));
	file.WriteAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&header_len,sizeof(int32));
	file.WriteAttr(B_MAIL_ATTR_CONTENT,B_INT32_TYPE,0,&content_len,sizeof(int32));	
	time_t when = MakeTime_t(date.String());
	time_t now = time(NULL);
	float diff = difftime(now,when);
	switch(fRetrieve )
	{
	case 0:
		*is_delete = false;
		break;
	case 1:
		*is_delete = true;
		break;
	case 2:
		*is_delete = ( diff/3600 > fDeleteDays*24)?true:false;
		break;
	}
	file.WriteAttr(B_MAIL_ATTR_WHEN,B_TIME_TYPE,0,&when,sizeof(time_t));
	
	entry_ref ref;
	::get_ref_for_path(path.Path(),&ref);
	*folder_ref =ref;
	return;
}

/***********************************************************
 * FilterMail
 ***********************************************************/
void
HPopClientView::FilterMail(const char* subject,
							const char* from,
							const char* to,
							const char* cc,
							const char* reply,
							const char* account,
							BString &outpath)
{
	BPath	path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Filters");
	::create_directory(path.Path(),0777);
	
	entry_ref ref;
	::get_ref_for_path(path.Path(),&ref);
	BDirectory dir( &ref );
   	//load all email
   	status_t err = B_NO_ERROR;
	dir.Rewind();
	BFile file;
	BMessage filter;
	int32 attr,op,action,op2;
	BString attr_value,action_value;
	char* key(NULL);
	bool hit = false,skip = false;
	type_code type;
	int32 count;
	
	while( err == B_OK )
	{
		if( (err = dir.GetNextRef( &ref )) == B_OK)
		{
			if( file.SetTo(&ref,B_READ_ONLY) != B_OK)
				continue;
			filter.Unflatten(&file);
			filter.GetInfo("attribute",&type,&count);
			filter.FindString("action_value",&action_value);
			filter.FindInt32("action",&action);	
			for(int32 i = 0;i < count;i++)
			{
				filter.FindInt32("attribute",i,&attr);
				filter.FindInt32("operation1",i,&op);
				filter.FindInt32("operation2",i,&op2);
				
				filter.FindString("attr_value",i,&attr_value);
				
				switch(attr)
				{
				case 0:
					key = ::strdup(subject);
					break;
				case 1:
					key = ::strdup(to);
					break;
				case 2:
					key = ::strdup(from);
					break;
				case 3:
					key = ::strdup(cc);
					break;
				case 4:
					key = ::strdup(reply);
					break;
				case 5:
					key = ::strdup(account);
					break;
				}
				
				if(!skip)
					hit = Filter(key,op,attr_value.String());
				else
					skip = false;
			
				free( key );
				key = NULL;
				// And condition
				if(op2 == 0 && !hit )
					break;
				// Or condition
				if(op2 == 1 && hit)
					skip=true;	
			}
			if(hit)
			{
				PRINT(("MATCH\n"));
				// action move
				//if(action == 0)
				//{	
					::find_directory(B_USER_DIRECTORY,&path);
					path.Append( MAIL_FOLDER );
					path.Append(action_value.String());
					PRINT(("%s\n",action_value.String()));
					if(!path.Path())
						goto no_hit;
					outpath = path.Path();
					return;
				//}
			}
		}
	}
no_hit:
	// not hit
	::find_directory(B_USER_DIRECTORY,&path);
	path.Append( MAIL_FOLDER );
				
	path.Append("in");
	outpath = path.Path();
}							

/***********************************************************
 * Filter
 ***********************************************************/
bool
HPopClientView::Filter(const char* in_key,int32 operation,const char *attr_value)
{
	using namespace BPrivate;
	
	TrackerString key(in_key);
	BString value(attr_value);
	TrackerStringExpressionType exp = kContains;
	
	switch(operation)
	{
	// contain
	case 0:
		exp = kContains;
		break;
	// is
	case 1:
		exp = kNone;
		break;
	// begin with
	case 2:
		exp = kStartsWith;
		break;
	// end with
	case 3:
		exp = kEndsWith;
		break;
	}
	return (exp == kNone)?(key == value):(key.Matches(value.String(),false,exp));
}

/***********************************************************
 * SetNextRecvPos
 ***********************************************************/
void
HPopClientView::SetNextRecvPos(const char *uidl)
{
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	path.Append(fAccountName.String());
	
	BFile file(path.Path(),B_READ_WRITE);
	if(file.InitCheck() != B_OK)
		return;
	BMessage msg;
	msg.MakeEmpty();
	msg.Unflatten(&file);
	
	msg.RemoveData("uidl");
	if(fRetrieve==0 || fRetrieve==2)
		msg.AddString("uidl",(uidl)?uidl:"");
	else
		msg.AddString("uidl","");
	file.Seek(0,SEEK_SET);
	ssize_t numBytes;
	msg.Flatten(&file,&numBytes);
	file.SetSize(numBytes);
	PRINT(("UIDL:%s\n",uidl));
}

/***********************************************************
 * PlayNotifySound
 ***********************************************************/
void
HPopClientView::PlayNotifySound()
{
	// Play notification sound
	::system_beep("New E-mail");
	// Change deskbar icon and Play LED animation
	HWindow *window = cast_as(Window(),HWindow);
	if(window)
	{
		window->ChangeDeskbarIcon(DESKBAR_NEW_ICON);
		window->PlayLEDAnimaiton();	
	}
}

/***********************************************************
 * Cancel
 ***********************************************************/
void
HPopClientView::Cancel()
{
	fPopLooper->ForceQuit();
	StopBarberPole();
	StopProgress();
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HPopClientView::MouseDown(BPoint pos)
{
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons); 
	if(fIsRunning)
    {	 
    	BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
    	BFont font(be_plain_font);
    	font.SetSize(10);
    	theMenu->SetFont(&font);
    	
    	theMenu->AddItem(new BMenuItem(_("Abort"),new BMessage(M_POP_ABORT)));
    	
    	
    	BRect r;
        ConvertToScreen(&pos);
        r.top = pos.y - 5;
        r.bottom = pos.y + 5;
        r.left = pos.x - 5;
        r.right = pos.x + 5;
    	
    	BMenuItem *theItem = theMenu->Go(pos, false,true,r);  
    	if(theItem)
    	{
    	 	BMessage*	aMessage = theItem->Message();
			if(aMessage)
				this->Window()->PostMessage(aMessage,this);
	 	} 
	 	delete theMenu;
    }else
    	_inherited::MouseDown(pos);
}

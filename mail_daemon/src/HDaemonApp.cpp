#include "HDaemonApp.h"
#include "Encoding.h"
#include "ExtraMailAttr.h"
#include "TrackerUtils.h"

#include <Debug.h>
#include <Entry.h>
#include <File.h>
#include <E-mail.h>
#include <ctype.h>
#include <stdlib.h>
#include <NodeInfo.h>
#include <Beep.h>
#include <Deskbar.h>
#include <Roster.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HDaemonApp::HDaemonApp():BApplication(APP_SIG)
	,fRunner(NULL)
	,fPopClient(NULL)
	,fAccountDirectory(NULL)
	,fHaveNewMails(false)
{
	// Load scooby preferences
	BPath path;
	::find_directory(B_USER_DIRECTORY,&path);
	path.Append("mail");
	path.Append( "Trash" );
	::create_directory(path.Path(),0777);
	char *pref_name = new char[::strlen("Scooby") + 12];
	::sprintf(pref_name,"%s %s","Scooby","preference");
	fPrefs = new HPrefs(pref_name,"Scooby");
	delete[] pref_name;
	fPrefs->LoadPrefs();
	// Get Account setting path
	::find_directory(B_USER_SETTINGS_DIRECTORY,&fSettingPath);
	fSettingPath.Append("Scooby");
	InstallDeskbarIcon();
	RunTimer();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HDaemonApp::~HDaemonApp()
{
	delete fAccountDirectory;
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HDaemonApp::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_CHECK_NOW:
	case M_TIMER:
		CheckMails();
		break;
	case M_RELOAD_SETTING:
		fPrefs->LoadPrefs();
		if(fRunner)
		{
			int32 interval;
			fPrefs->GetData("check_minutes",&interval);
			PRINT(("NEW INTERVAL:%d\n",interval));
			fRunner->SetInterval(interval);
		}
		break;
	case M_LAUNCH_SCOOBY:
	{
		be_roster->Launch("application/x-vnd.takamatsu-scooby");
		break;
	}
	// Error
	case H_ERROR_MESSAGE:
	{
		PRINT(("POP ERROR\n"));
		BString err_str("POP3 ERROR");
		err_str += "\n";
		if(message->FindString("log"))
			err_str += message->FindString("log");
		PRINT(("%s\n",err_str.String()));
		fPopClient->PostMessage(B_QUIT_REQUESTED);
		fPopClient = NULL;
		
		entry_ref ref;
		if(GetNextAccount(ref) == B_OK)
			// Connect next server
			CheckFromServer(ref);
		else{
			if(fGotMails)
				PlayNotifySound();
			fChecking = false;
		}
		break;
	}
	// conect success
	case H_CONNECT_MESSAGE:
	{	
		BMessage msg(H_LOGIN_MESSAGE);
		msg.AddString("login",fLogin);
		msg.AddString("password",fPassword);
		msg.AddBool("apop",(fProtocolType == 0)?false:true);
		fPopClient->PostMessage(&msg);
		break;
	}
	// login success
	case H_LOGIN_MESSAGE:
		fPopClient->PostMessage(H_UIDL_MESSAGE);
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
				fPopClient->PostMessage(B_QUIT_REQUESTED);
				fPopClient = NULL;
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
				fPopClient->PostMessage(B_QUIT_REQUESTED);
				fPopClient = NULL;
				break;
			}
			
			fPopClient->PostMessage(&msg);
		}else{
			// POP3 server is not support UIDL command
			fCanUseUIDL = false;
			PRINT(("UIDL not supported\n"));
			fPopClient->PostMessage(H_LIST_MESSAGE);
		}
		break;
	}
	// receive list
	case H_LIST_MESSAGE:
	{
		message->what = H_UIDL_MESSAGE;
		this->PostMessage(message);
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
			entry_ref folder_ref,file_ref;
			bool is_delete;
			SaveMail(content,&folder_ref,&file_ref,&is_delete);
	
			if(is_delete)
				fDeleteMails.AddInt32("index",index);
			int32 count;
			type_code type;
				
			fDeleteMails.GetInfo("index",&type,&count);
			if(count == 0 && message->FindBool("end"))
			{
				SetNextRecvPos(fUidl.String());
				fPopClient->PostMessage(B_QUIT_REQUESTED);
				fPopClient = NULL;
				break;
			}else if(count > 0 && message->FindBool("end")){
				fPopClient->PostMessage(&fDeleteMails);					
				break;
			}
		}
		break;
	}
	// delete success
	case H_DELETE_MESSAGE:
	{
		int32 index;
		if(message->FindInt32("index",&index) == B_OK)
		{
			if(message->FindBool("end"))
			{
				fPopClient->PostMessage(B_QUIT_REQUESTED);
				fPopClient = NULL;
				SetNextRecvPos("");
			}
		}
		break;
	}
	// Session ended
	case M_QUIT_FINISHED:
	{
		entry_ref ref;
		status_t err = GetNextAccount(ref);
		if(err == B_OK)
		{
			// Connect next server
			CheckFromServer(ref);
		}else{
			// Quit pop session
			if(fGotMails)
				PlayNotifySound();
			fChecking = false;
			PRINT(("All check have been finished\n"));
			RefreshAllCaches();
			EmptyNewMailList();
		}
		break;
	}
	// Check whether new mails have received from Deskbar replicant
	case M_CHECK_SCOOBY_STATE:
	{
		int32 icon;
		if(message->FindInt32("icon",&icon) != B_OK)
			break;
		BMessage msg(M_CHECK_SCOOBY_STATE);
		msg.AddInt32("icon",(fHaveNewMails)?DESKBAR_NEW_ICON:DESKBAR_NORMAL_ICON);
		message->SendReply(&msg,(BHandler*)NULL,1000000);
		break;
	}
	// reset deskbar icon
	case M_RESET_ICON:
	{
		fHaveNewMails = false;
		break;
	}
	case M_NEW_MESSAGE:
	{
		entry_ref ref;
		if(be_roster->FindApp("text/x-email",&ref) != B_OK)
			return;
		int32 argc = 0;
		char *argv[3];
		argv[argc] = new char[strlen("mailto:")+1];
		::strcpy(argv[argc++],"mailto:" );
		argv[argc++] = NULL;
		
		be_roster->Launch(&ref,argc-1,argv);
			
		for(int32 k = 0;k < argc;k++)
			delete[] argv[k];
		break;
	}
	default:
		BApplication::MessageReceived(message);
	}
}

/***********************************************************
 * RunTimer
 ***********************************************************/
void
HDaemonApp::RunTimer()
{
	int32 interval;
	fPrefs->GetData("check_minutes",&interval);
	PRINT(("INTERVAL:%d\n",interval));
	if(interval > 0)
		fRunner = new BMessageRunner(be_app_messenger,new BMessage(M_TIMER),interval*60000000);
}

/***********************************************************
 * StopTimer
 ***********************************************************/
void
HDaemonApp::StopTimer()
{
	delete fRunner;
	fRunner = NULL;	
}

/***********************************************************
 * CheckMails
 ***********************************************************/
void
HDaemonApp::CheckMails()
{
	if(fChecking)
		return;
	fChecking = true;
	PRINT(("CHECK\n"));
	entry_ref ref;

	if(!fAccountDirectory)
	{
		BPath path(fSettingPath);
		path.Append("Accounts");
		fAccountDirectory = new BDirectory(path.Path());
		PRINT(("%s\n",path.Path()));
	}
	fAccountDirectory->Rewind();
	
	if(GetNextAccount(ref) != B_OK || CheckFromServer(ref) != B_OK)
		fChecking = false;
}

/***********************************************************
 * GetNextAccount
 ***********************************************************/
status_t
HDaemonApp::GetNextAccount(entry_ref &ref)
{
	status_t err = B_OK;
	BEntry entry;
	while( err == B_OK )
	{
		err = fAccountDirectory->GetNextEntry(&entry,false);	
		if( entry.InitCheck() != B_NO_ERROR )
			break;
		else if(entry.IsDirectory())
			continue;
		else{
			if(entry.GetRef(&ref) == B_OK)
				return B_OK;
		}
	}
	return err;
}

/***********************************************************
 * CheckFromServer
 ***********************************************************/
status_t
HDaemonApp::CheckFromServer(entry_ref &ref)
{
	// Load account setting
	BFile file;
	BMessage msg;
	BEntry entry(&ref);
	
	char name[B_FILE_NAME_LENGTH+1];
	entry.GetName(name);
	PRINT(("Account Name:%s\n",name));
	if(file.SetTo(&entry,B_READ_ONLY) != B_OK)
		return B_ERROR;
	
	fAccountName = name;	
	
	msg.Unflatten(&file);
	const char* password,*port;
	
	if(msg.FindString("pop_host",&fHost) != B_OK)
		return B_ERROR;
			
	if(msg.FindString("pop_port",&port) != B_OK)
		return B_ERROR;
	fPort = atoi(port);
	if(msg.FindString("pop_user",&fLogin) != B_OK)
		return B_ERROR;
			
	if(msg.FindInt16("protocol_type",&fProtocolType) != B_OK)
		fProtocolType = 0;
	
	if(msg.FindInt16("retrieve",&fRetrievingType) != B_OK)
		fRetrievingType = 0;
			
	if(fRetrievingType == 2)
		msg.FindInt32("delete_day",&fDeleteDays);
	else
		fDeleteDays = 0;

	if(msg.FindString("uidl",&fUidl) != B_OK)	
		fUidl= "";
			
	if(msg.FindString("pop_password",&password) != B_OK)
		return B_ERROR;

	int32 len = strlen(password);
	fPassword = "";
	for(int32 k = 0;k < len;k++)
		fPassword += (char)(255-password[k]);
		
	if(fPopClient)
		fPopClient->PostMessage(B_QUIT_REQUESTED);
	fPopClient = new PopClient(NULL,this);
	if(fPopClient->Lock())
	{
		fPopClient->InitBlackList();
		fPopClient->Unlock();
	}
	
	fDeleteMails.MakeEmpty();
	fDeleteMails.what = H_DELETE_MESSAGE;
	fGotMails = false;
	
	BMessage sndMsg(H_CONNECT_MESSAGE);
	sndMsg.AddString("address",fHost);
	sndMsg.AddInt16("port",fPort);
	fPopClient->PostMessage(&sndMsg);
	return B_OK;
}

/***********************************************************
 * InstallDeskbarIcon
 ***********************************************************/
void
HDaemonApp::InstallDeskbarIcon()
{
	BDeskbar deskbar;

	if(deskbar.HasItem( "scooby_daemon" ) == false)
	{
		BRoster roster;
		entry_ref ref;
		roster.FindApp( APP_SIG , &ref);
		int32 id;
		deskbar.AddItem(&ref, &id);
	}
}

/***********************************************************
 * RemoveDeskbarIcon
 ***********************************************************/
void
HDaemonApp::RemoveDeskbarIcon()
{
	BDeskbar deskbar;
	if( deskbar.HasItem( "scooby_daemon" ))
		deskbar.RemoveItem( "scooby_daemon" );
}

/***********************************************************
 * AddNewMail
 ***********************************************************/
void
HDaemonApp::AddNewMail(BEntry *entry)
{
	fNewMailList.AddItem(entry);
}

/***********************************************************
 * EmptyNewMailList
 ***********************************************************/
void
HDaemonApp::EmptyNewMailList()
{
	int32 count = fNewMailList.CountItems();
	
	while(count>0)
		delete fNewMailList.RemoveItem(--count);
}

/***********************************************************
 * PlayNotifySound
 ***********************************************************/
void
HDaemonApp::PlayNotifySound()
{
	// Play notification sound
	system_beep("New E-mail");
	// Change deskbar icon
	//HWindow *window = cast_as(Window(),HWindow);
	//if(window)
	//	window->ChangeDeskbarIcon(DESKBAR_NEW_ICON);
}

/***********************************************************
 * SaveMails
 ***********************************************************/
void
HDaemonApp::SaveMail(const char* all_content,
						entry_ref* folder_ref,
						entry_ref *file_ref,
						bool *is_delete)
{
	fGotMails = true;
	if(!fHaveNewMails)
		fHaveNewMails = true;
	BString header(""),subject(""),to(""),date(""),cc(""),from("")
			,priority(""),reply(""),mime("");
	Encoding encode;
	
	bool is_multipart = false;
	int32 org_len = strlen(all_content);
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
				reply.String(),
				folder_path);
	//PRINT(("path:%s\n",folder_path.String() ));
	
	// Save to disk
	BPath path = folder_path.String();
	::create_directory(path.Path(),0777);
	BDirectory destDir(path.Path());
	path.Append(subject.String());
	//PRINT(("path:%s\n",path.Path() ));
	// create the e-mail file
	BFile file;

	TrackerUtils().SmartCreateFile(&file,&destDir,path.Leaf(),"_");
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
	file.WriteAttr(B_MAIL_ATTR_ATTACHMENT,B_BOOL_TYPE,0,&is_multipart,sizeof(bool));
	int32 content_len = strlen(all_content)-header_len;
	//PRINT(("header:%d, content%d\n",header_len,content_len));
	file.WriteAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&header_len,sizeof(int32));
	file.WriteAttr(B_MAIL_ATTR_CONTENT,B_INT32_TYPE,0,&content_len,sizeof(int32));	
	time_t when = MakeTime_t(date.String());
	time_t now = time(NULL);
	float diff = difftime(now,when);
	switch(fRetrievingType )
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
	
	BNodeInfo ninfo(&file);
	ninfo.SetType("text/x-email");
	entry_ref ref;
	::get_ref_for_path(path.Path(),&ref);
	*file_ref = ref;
	
	AddNewMail(new BEntry(file_ref));
	
	path.GetParent(&path);
	::get_ref_for_path(path.Path(),&ref);
	*folder_ref =ref;
	return;
}

/***********************************************************
 * GetHeaderParam
 ***********************************************************/
int32
HDaemonApp::GetHeaderParam(BString &out,const char* content,int32 offset)
{
	int32 i = offset;
	if(content[i] == ' ')
		i++;
	while(1)
	{
		if(content[i] =='\n')
		{
			if(isalpha(content[i+1])|| content[i+1] == '\r'|| content[i+1] == '\n' )
				break; 
		}
		if(content[i] != '\r' && content[i] != '\n' )
		 	out += content[i++];
		else
			i++;
	}
	return i;
}

/***********************************************************
 * FilterMail
 ***********************************************************/
void
HDaemonApp::FilterMail(const char* subject,
							const char* from,
							const char* to,
							const char* cc,
							const char* reply,
							BString &outpath)
{
	BPath	path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append("Scooby");
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
					path.Append( "mail" );
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
	path.Append( "mail" );
				
	path.Append("in");
	outpath = path.Path();
}							

/***********************************************************
 * Filter
 ***********************************************************/
bool
HDaemonApp::Filter(const char* in_key,int32 operation,const char *attr_value)
{
	BString key(in_key);	
	bool hit = false;
	
	switch(operation)
	{
	// contain
	case 0:
		if(key.FindFirst(attr_value) != B_ERROR)
			hit = true;
		break;
	// is
	case 1:
		if(key.Compare(attr_value) == 0)
			hit = true;
		break;
	// begin with
	case 2:
		if(key.FindFirst(attr_value) == 0)
			hit = true;
		break;
	// end with
	case 3:
		if(key.FindFirst(attr_value) == key.Length()- (int32)::strlen(attr_value) )
			hit = true;
		break;
	}
	return hit;
}

/***********************************************************
 * MakeTime_t
 ***********************************************************/
time_t
HDaemonApp::MakeTime_t(const char* date)
{
	const char* mons[] = {"Jan","Feb","Mar","Apr","May"
						,"Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	const char* wdays[] = {"Sun","Mon","Tue","Wed","Thu"
						,"Fri","Sat"};

	char swday[5];
	int wday=0;
	int day;
	char smon[4];
	int mon;
	//char stime[9];
	int year;
	int hour;
	int min;
	int sec;
	int gmt_off = 0;
	char offset[5];
	// Mon, 16 Oct 2000 00:50:04 +0900 or Mon, 16 Oct 00 00:50:04 +0900
	int num_scan  = ::sscanf(date,"%3s,%d%3s%d %2d:%2d:%2d %5s"
				,swday,&day,smon,&year,&hour,&min,&sec,offset);
	if(num_scan != 8)
	{
		// 9 Nov 2000 11:30:23 -0000 or  29 Jan 01 9:16:08 PM
		num_scan = ::sscanf(date, "%d%3s%d %2d:%2d:%2d %5s"
					,&day,smon,&year,&hour,&min,&sec,offset);
		if(strcasecmp(offset,"PM") == 0)
        		hour += 12;
        ::strcpy(offset,"-0000");	
        
		if(num_scan != 7)
		{
			// 01 Feb 01 12:22:42 PM
			num_scan = sscanf(date, "%d %3s %3d %d %2d:%2d %5s",
                                        &day, smon, &year, &hour, &min,&sec,offset);
        	if(strcasecmp(offset,"PM") == 0)
        		hour += 12;
        	if (num_scan != 7) 
        	{
        		// Return current time if this can not parse date
        	 	PRINT(("Unknown date format\n"));
				return time(NULL);
			}
		}
	}
	// month
	for(mon = 0;mon < 12;mon++)
	{
		if(strncmp(mons[mon],smon,3) == 0)
			break;
	}
	// week of day
	if(num_scan == 8)
	{
		for(wday = 0;wday < 12;wday++)
		{
			if(strncmp(wdays[wday],swday,3) == 0)
				break;
		}
	}
	// offset 
	char op = offset[0];
	float off = atof(offset+1);
	if(op == '+')
		gmt_off  = static_cast<int>((off/100.0)*60*60);
	else if(op == '-')
		gmt_off  = static_cast<int>(-(off/100.0)*60*60);
	else
		gmt_off = 0;
	struct tm btime;
	btime.tm_sec = sec;
	btime.tm_min = min;
	btime.tm_hour = hour;
	btime.tm_mday = day;
	btime.tm_mon = mon;
	
	if(year < 100)
	{
		if(year < 70)
			year+=2000;
		else
			year+=1900;
	}
	btime.tm_year = year - 1900;
	
	if(num_scan == 8)
		btime.tm_wday = wday;
	btime.tm_gmtoff = gmt_off;
	return mktime(&btime);
}

/***********************************************************
 * SetNextRecvPos
 ***********************************************************/
void
HDaemonApp::SetNextRecvPos(const char *uidl)
{
	BPath path(fSettingPath);
	path.Append("Accounts");
	path.Append(fAccountName.String());
	
	BFile file(path.Path(),B_READ_WRITE);
	if(file.InitCheck() != B_OK)
		return;
	BMessage msg;
	msg.MakeEmpty();
	msg.Unflatten(&file);
	
	msg.RemoveData("uidl");
	if(fRetrievingType==0 || fRetrievingType==2)
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
 * RefreshAllCaches
 ***********************************************************/
void
HDaemonApp::RefreshAllCaches()
{
	// Refresh folders structure cache
	BPath path;
	entry_ref foldersCacheRef;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append("Scooby");
	path.Append("Folders.cache");
	::get_ref_for_path(path.Path(),&foldersCacheRef);
	
	BFile file(&foldersCacheRef,B_WRITE_ONLY|B_CREATE_FILE);
	if(file.InitCheck() == B_OK)
	{
		BMessage foldersCache;
		foldersCache.Unflatten(&file);
	
		int32 count;
		type_code type;
		entry_ref ref;
		BEntry entry;
		struct stat st;
		time_t oldtime;
				
		foldersCache.GetInfo("refs",&type,&count);
	
		for(int32 i = 0;i < count;i++)
		{
			if(foldersCache.FindRef("refs",i,&ref) != B_OK)
				continue;
			if(entry.SetTo(&ref) != B_OK)
				continue;
			foldersCache.FindInt32("time",i,&oldtime);
			entry.GetStat(&st);
			if(st.st_mtime != oldtime)
				foldersCache.ReplaceInt32("time",i,st.st_mtime);
		}
		file.Seek(0,SEEK_SET);
		foldersCache.Flatten(&file);
	}
	// Refresh mail caches
	int32 count = fNewMailList.CountItems();
	entry_ref ref;
	BEntry *entry;
	for(int32 i = 0;i < count;i++)
	{
		entry = (BEntry*)fNewMailList.ItemAt(i);
		if(!entry)
			continue;
		entry->GetRef(&ref);
		RefreshMailCache(ref);
	}
}

/***********************************************************
 * RefreshMailCache
 ***********************************************************/
void
HDaemonApp::RefreshMailCache(entry_ref &ref)
{
	
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HDaemonApp::QuitRequested()
{
	RemoveDeskbarIcon();
	StopTimer();
	EmptyNewMailList();
	if(fPopClient)
	{
		fPopClient->PostMessage(B_QUIT_REQUESTED);
		fPopClient = NULL;
	}
	return BApplication::QuitRequested();
}
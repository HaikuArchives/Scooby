#include "HIMAP4Folder.h"
#include "HApp.h"
#include "HIMAP4Item.h"
#include "Encoding.h"
#include "HWindow.h"
#include "HFolderList.h"
#include "HString.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <stdlib.h>
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <Directory.h>
#include <ListView.h>
#include <string.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HIMAP4Folder::HIMAP4Folder(const char* name,
						const char* folder_name,
						const char* server,
						int			port,
						const char*	login,
						const char* password,
						BListView *owner)
	:HFolderItem(name,IMAP4_TYPE,owner)
	,fClient(NULL)
	,fServer(server)
	,fPort(port)
	,fLogin(login)
	,fPassword(password)
	,fRemoteFolderName(folder_name)
	,fFolderGathered(false)
	,fChildItem(false)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
HIMAP4Folder::~HIMAP4Folder()
{
	StoreSettings();
	EmptyMailList();
	if(fClient)
	{
		fClient->Logout();
		fClient->Close();
	}
	delete fClient;
}

/***********************************************************
 * StoreSettings
 ***********************************************************/
void
HIMAP4Folder::StoreSettings()
{
	if(fChildItem)
		return;
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	::create_directory(path.Path(),0777);
	path.Append("IMAP4");
	::create_directory(path.Path(),0777);
	
	path.Append(fName.String());
	
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
	
	BMessage msg(B_SIMPLE_DATA);
	msg.AddString("folder",fRemoteFolderName.String() );
	msg.AddString("server",fServer.String() );
	msg.AddInt16("port",fPort);
	msg.AddString("login",fLogin.String() );
	
	BString pass;
	int32 len = fPassword.Length();
	for(int32 i = 0;i < len;i++)
		pass += (char)255-fPassword[i];
	
	msg.AddString("password",pass.String() );	
	ssize_t size;
	msg.Flatten(&file,&size);
	file.SetSize(size);
}

/***********************************************************
 * StartRefreshCache
 ***********************************************************/
void
HIMAP4Folder::StartRefreshCache()
{
	PRINT(("Refresh IMAP4\n"));
	EmptyMailList();
	if(fThread != -1)
		return;
	fDone = false;
	// Set icon to open folder
	BBitmap *icon = ((HApp*)be_app)->GetIcon("CloseFolder");
	SetColumnContent(1,icon,2.0,false,false);
	
	StartGathering();
}

/***********************************************************
 * StartGathering
 ***********************************************************/
void
HIMAP4Folder::StartGathering()
{
	if(fThread != -1)
		return;
	//PRINT(("Query Gathering\n"));
	fThread = ::spawn_thread(GetListThread,"IMAP4Fetch",B_NORMAL_PRIORITY,this);
	::resume_thread(fThread);
}

/***********************************************************
 * GetListThread
 ***********************************************************/
int32
HIMAP4Folder::GetListThread(void *data)
{
	HIMAP4Folder *theItem = static_cast<HIMAP4Folder*>(data);
	theItem->IMAPGetList();
	return 0;
}

/***********************************************************
 * GetList
 ***********************************************************/
void
HIMAP4Folder::IMAPGetList()
{
	if(!fClient)
	{
		if(IMAPConnect() != B_OK)
		{
			if(fOwner->IndexOf(this) == fOwner->CurrentSelection())
				fOwner->Window()->PostMessage(M_STOP_MAIL_BARBER_POLE);
			return;	
		}
	}
	if(!fChildItem)
		GatherChildFolders();
	if(fRemoteFolderName.Length() == 0)
	{
		if(fOwner->IndexOf(this) == fOwner->CurrentSelection())
				fOwner->Window()->PostMessage(M_STOP_MAIL_BARBER_POLE);
		return;
	}	
	
	int32 mail_count = 0;
	if( (mail_count = fClient->Select(fRemoteFolderName.String())) < 0)
	{
		(new BAlert("",_("Could not select remote folder"),_("OK")
						,NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		if(fOwner->IndexOf(this) == fOwner->CurrentSelection())
				fOwner->Window()->PostMessage(M_STOP_MAIL_BARBER_POLE);
		return;
	}
	
	BString subject,from,to,cc,reply,date,priority;
	bool read,attachment;
	Encoding encode;
	
	for(int32 i = 1;i <= mail_count;i++)
	{
		subject = from = to = date = priority = "";
		if(fClient->FetchFields(i,
								subject,
								from,
								to,
								cc,
								reply,
								date,
								priority,
								read,
								attachment) != B_OK)
		{
			PRINT(("FetchFields ERROR\n"));
			continue;
		}
		// ConvertToUTF8
		encode.Mime2UTF8(subject);
		encode.Mime2UTF8(from);
		encode.Mime2UTF8(to);
		//
		fMailList.AddItem(new HIMAP4Item( (read)?"Read":"New",	
											subject.String(),
											from.String(),
											to.String(),
											cc.String(),
											reply.String(),
											MakeTime_t(date.String()),
											priority.String(),
											(attachment)?1:0,
											i,
											fClient,this
											));
		if(!read) fUnread++;
	}
	SetName(fUnread);
	
	fDone = true;
	
	// Set icon to open folder
	BBitmap *icon = ((HApp*)be_app)->GetIcon("OpenIMAP");
	SetColumnContent(1,icon,2.0,false,false);
	
	InvalidateMe();
	fThread = -1;
	return;
}

/***********************************************************
 * Connect
 ***********************************************************/
status_t
HIMAP4Folder::IMAPConnect()
{
	delete fClient;
	fClient = new IMAP4Client();
	PRINT(("IMAP4 Connect Start:%s %d\n",fServer.String(),fPort));
	if( fClient->Connect(fServer.String(),fPort) != B_OK)
	{
		HString str;
		str.Format("%s: Address:%s Port:%d",_("Could not connect to IMAP4 server"),fServer.String(),fPort);
		(new BAlert("",str.String(),_("OK"),
						NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		delete fClient;
		fClient = NULL;
		return B_ERROR;
	}
	if( fClient->Login(fLogin.String(),fPassword.String()) != B_OK)
	{
		(new BAlert("",_("Could not login to IMAP4 server"),_("OK"),
						NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		delete fClient;
		fClient = NULL;	
		return B_ERROR;
	}
	return B_OK;
}


/***********************************************************
 * GatherChildFolders
 ***********************************************************/
void
HIMAP4Folder::GatherChildFolders()
{
	if(fFolderGathered)
		return;
	BList namelist,pointerList;
	
	int32 count = fClient->List(fRemoteFolderName.String(),&namelist);
	if(count <= 0)
		return;
	
	BMessage childMsg(M_ADD_UNDER_ITEM);
	char displayName[B_FILE_NAME_LENGTH];
	char *p;
	
	const char* server = fServer.String();
	const char* login = fLogin.String();
	const char* password = fPassword.String();
	int16 port = fPort;
	BListView *list = fOwner;
	
	HIMAP4Folder *folder;
	
	for(int32 i = 0;i < count;i++)
	{
		char *name = (char*)namelist.ItemAt(i);
		if(strstr(name,"/"))
		{
			int32 namelen = ::strlen(name);
			p = name;
			p += namelen-1;
			while(*p != '/')
				p--;
			p++;
		}
		else
			p = name;
		::strcpy(displayName,p);
		displayName[::strlen(p)] = '\0';
		pointerList.AddItem((folder = new HIMAP4Folder(displayName,name
											,server
											,port
											,login
											,password
											,list)));
		folder->SetFolderGathered(true);
		folder->SetChildFolder(true);
		PRINT(("%s\n",name));
		free( name );
	}
	
	int32 index;
	for(int32 i = 0;i < count;i++)
	{
		folder = (HIMAP4Folder*)pointerList.ItemAt(i);
		index = FindParent(folder->FolderName(),folder->RemoteFolderName(),&pointerList);
		childMsg.AddPointer("parent",(index < 0)?this:(HIMAP4Folder*)pointerList.ItemAt(index));
		childMsg.AddBool("expand",(index < 0)?true:false);
		childMsg.AddPointer("item",folder);
	}
	if(!childMsg.IsEmpty())
		fOwner->Window()->PostMessage(&childMsg,fOwner);
	fFolderGathered = true;
}

/***********************************************************
 * FindParent
 ***********************************************************/
int32
HIMAP4Folder::FindParent(const char* name,const char* folder_path,BList *list)
{
	int32 count = list->CountItems();
	HIMAP4Folder *folder;
	char path[B_PATH_NAME_LENGTH];
	
	if(::strstr(folder_path,"/"))
	{
		::strcpy(path,folder_path);
		path[::strlen(path)-::strlen(name) -1] = '\0';
	}else
		return -1; 
	
	for(int32 i = 0;i < count;i++)
	{
		folder = (HIMAP4Folder*)list->ItemAt(i);
		if(strcmp(folder->RemoteFolderName(),path) == 0)
			return i;
	}
	return -1;
}

/***********************************************************
 * MakeTime_t
 ***********************************************************/
time_t
HIMAP4Folder::MakeTime_t(const char* date)
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
		// 9 Nov 2000 11:30:23 -0000
		num_scan = ::sscanf(date, "%d%3s%d %2d:%2d:%2d %5s"
					,&day,smon,&year,&hour,&min,&sec,offset);
		DEBUG_ONLY(
			if(num_scan != 7)
				printf("Unknown date format\n");
		);
	}
	//PRINT(("M:%s H:%d M:%d S:%d\n",smon,hour,min,sec));
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
	if(op == '-')
		gmt_off  = static_cast<int>(-(off/100.0)*60*60);
	struct tm btime;
	btime.tm_sec = sec;
	btime.tm_min = min;
	btime.tm_hour = hour;
	btime.tm_mday = day;
	btime.tm_mon = mon;
	if(year > 1900)
		btime.tm_year = year-1900;
	else
		btime.tm_year = 2000 + year;
	if(num_scan == 8)
		btime.tm_wday = wday;
	btime.tm_gmtoff = gmt_off;
	return mktime(&btime);	
}

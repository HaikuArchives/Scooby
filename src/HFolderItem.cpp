#include "HFolderItem.h"
#include "HApp.h"
#include "HMailItem.h"
#include "HPrefs.h"
#include "HFolderList.h"
#include "HWindow.h"
#include "HMailCache.h"
#include "Utilities.h"
#include "ColumnListView.h"

#include <Path.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Debug.h>
#include <Directory.h>
#include <Bitmap.h>
#include <File.h>
#include <E-mail.h>
#include <ListView.h>
#include <Window.h>
#include <Autolock.h>
#include <Roster.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <ClassInfo.h>
#include <Autolock.h>
#include <NodeMonitor.h>
#include <StopWatch.h>

#define ICON_COLUMN 1
#define LABEL_COLUMN 2

/***********************************************************
 * Constructor
 ***********************************************************/
HFolderItem::HFolderItem(const entry_ref &ref,BListView *target)
	: CLVEasyItem(0, false, false, 20.0),BHandler()
	,fDone(false)
	,fUnread(0)
	,fThread(-1)
	,fCancel(false)
	,fFolderRef(ref)
	,fOwner(target)
	,fCacheThread(-1)
	,fCacheCancel(false)
	,fRefreshThread(-1)
	,fFolderType(FOLDER_TYPE)
	,fChildItems(0)
{	
	BAutolock lock(target->Window());
	target->Window()->AddHandler( (BHandler*)this );
	
	BEntry entry(&ref);
	entry.GetNodeRef(&fNodeRef);
	::watch_node(&fNodeRef,B_WATCH_DIRECTORY|B_WATCH_NAME
						,this,fOwner->Window());
	
	if(BPath(&ref).InitCheck() == B_OK)
		fName = BPath(&ref).Leaf();
	else
		fName = "Unknown";
	fMailList.MakeEmpty();
	
	BBitmap *icon(NULL);
	HApp* app = (HApp*)be_app;
	if(!entry.IsDirectory())
	{
		icon = app->GetIcon("CloseQuery");
		fFolderType = QUERY_TYPE;
	}else
		icon = app->GetIcon("CloseFolder");

	SetColumnContent(ICON_COLUMN,icon,2.0,false,false);
	SetColumnContent(LABEL_COLUMN,fName.String());

	((HApp*)be_app)->Prefs()->GetData("use_folder_cache",&fUseCache);
}

/***********************************************************
 * Constructor
 ***********************************************************/
HFolderItem::HFolderItem(const char* name,int32 type,BListView *target)
	: CLVEasyItem(0, false, false, 20.0)
	,fDone(false)
	,fUnread(0)
	,fThread(-1)
	,fCancel(false)
	,fName(name)
	,fOwner(target)
	,fCacheThread(-1)
	,fCacheCancel(false)
	,fRefreshThread(-1)
	,fFolderType(IMAP4_TYPE)
	,fChildItems(0)
{	
	fMailList.MakeEmpty();
	BBitmap *icon(NULL);
	HApp* app = (HApp*)be_app;
	if(type == IMAP4_TYPE)
		icon = app->GetIcon("CloseIMAP");
	else
		icon = app->GetIcon("OpenFolder");
	SetColumnContent(ICON_COLUMN,icon,2.0,false,false);
	SetColumnContent(LABEL_COLUMN,name);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HFolderItem::~HFolderItem()
{
	::stop_watching(this,fOwner->Window());
	
	status_t err;
	
	if(fThread >= 0)
	{
		thread_id id = fThread;
		fCancel = true;
		::wait_for_thread(id,&err);
	}
	
	if(fCacheThread >= 0)
	{
		thread_id id = fCacheThread;
		fCacheCancel = true;		
		::wait_for_thread(id,&err);
	}
	
	if(fRefreshThread >= 0)
	{
		::wait_for_thread(fRefreshThread,&err);
	}
	
	if(IsDone() && FolderType() == FOLDER_TYPE && fUseCache)
		CreateCache();
	else if(!IsDone() && FolderType() == FOLDER_TYPE&& fUseCache)
		AddMailsToCacheFile();
	EmptyMailList();
}

/***********************************************************
 * AddMail
 ***********************************************************/
void
HFolderItem::AddMail(HMailItem *item)
{
	fMailList.AddItem(item);
	::watch_node(&item->fNodeRef,B_WATCH_ATTR
						,this,fOwner->Window());
	if(!item->IsRead())
		SetName(fUnread+1);
}

/***********************************************************
 * RemoveMail
 ***********************************************************/
void
HFolderItem::RemoveMail(HMailItem* item)
{
	fMailList.RemoveItem(item);
	if(!item->IsRead())
		SetName(fUnread-1);
}

/***********************************************************
 * RemoveMail
 ***********************************************************/
HMailItem*
HFolderItem::RemoveMail(entry_ref& ref)
{
	int32 count = fMailList.CountItems();
	HMailItem **items = (HMailItem**)fMailList.Items();
	for(register int32 i = 0;i< count;i++)
	{
		HMailItem *item = (HMailItem*)items[i];
		if(!item)
			continue;
		if(item->Ref() == ref)
		{
			item = (HMailItem*)fMailList.RemoveItem(i);
			if(!item->IsRead())
				SetName(fUnread-1);
			return item;
		}
	}
	return NULL;
}

/***********************************************************
 * RemoveMail
 ***********************************************************/
HMailItem*
HFolderItem::RemoveMail(node_ref &nref)
{
	HMailItem *item = FindMail(nref);
	if(item)
	{
		fMailList.RemoveItem(item);
		return item;
	}
	return NULL;
}

/***********************************************************
 * FindMail
 ***********************************************************/
HMailItem*
HFolderItem::FindMail(node_ref &nref)
{
	int32 count = fMailList.CountItems();
	HMailItem **items = (HMailItem**)fMailList.Items();
	
		
	for(int32 i = 0;i< count;i++)
	{
		HMailItem *item = (HMailItem*)items[i];
		if(!item)
			continue;
		if(item->fNodeRef == nref)
			return item;
	}
	return NULL;
}

/***********************************************************
 * AddMail
 ***********************************************************/
void
HFolderItem::AddMails(BList* list)
{
	fMailList.AddList(list);
	
	int32 count = list->CountItems();
	HMailItem *item;
	int32 unread = 0;
	for(int32 i = 0;i < count;i++)
	{
		item = (HMailItem*)list->ItemAtFast(i);
		::watch_node(&item->fNodeRef,B_WATCH_ATTR
						,this,fOwner->Window());
		if(item&&!item->IsRead())
			unread++;
	}
	SetName(fUnread+unread);
}

/***********************************************************
 * RemoveMail
 ***********************************************************/
void
HFolderItem::RemoveMails(BList* list)
{
	int32 count = list->CountItems();
	HMailItem *item;
	for(int32 i = 0;i < count;i++)
	{
		item = (HMailItem*)list->ItemAtFast(i);
		if(!item)
			continue;
		fMailList.RemoveItem(item);
		if(!item->IsRead())
			fUnread--;
	}
}

/***********************************************************
 * StartGathering
 ***********************************************************/
void
HFolderItem::StartGathering()
{
	if(fThread != -1)
		return;
	//PRINT(("Folder Gathering\n"));
	fThread = ::spawn_thread(ThreadFunc,"MailGathering",B_LOW_PRIORITY,this);
	::resume_thread(fThread);
}

/***********************************************************
 * ThreadFunc
 ***********************************************************/
int32
HFolderItem::ThreadFunc(void*data)
{
	HFolderItem *item = (HFolderItem*)data;
	item->Gather();
	BListView *list = item->fOwner;
	item->SetName(item->fUnread);
	BAutolock lock(list->Window());
	list->InvalidateItem(((ColumnListView*)list)->FullListIndexOf(item));
	return 0;
}

/***********************************************************
 * Gather
 ***********************************************************/
void
HFolderItem::Gather()
{
	if(ReadFromCache() == B_OK)
		return;
	
	BPath	path;

	BDirectory dir( &fFolderRef );
   	//load all email
	HMailItem *item(NULL);
	char type[B_MIME_TYPE_LENGTH+1];
	entry_ref ref;
	BNode node;
#ifndef USE_SCANDIR
	char buf[4096];
	dirent *dent;
	int32 count;
	int32 offset;
	
	while((count = dir.GetNextDirents((dirent *)buf, 4096)) > 0)
	{
		offset = 0;
		/* Now we step through the dirents. */ 
		while (count-- > 0)
		{
			dent = (dirent *)buf + offset;
			offset +=  dent->d_reclen;
			/* Skip . and .. directory */
			if(::strcmp(dent->d_name,".") == 0 || ::strcmp(dent->d_name,"..")== 0)
				continue;
			ref.device = dent->d_pdev;
			ref.directory = dent->d_pino;
			ref.set_name(dent->d_name);
			/* Do something with the ref. */
			if(node.SetTo(&ref) != B_OK)
				continue;
			node.ReadAttr("BEOS:TYPE",B_STRING_TYPE,0,type,B_MIME_TYPE_LENGTH);
			if(::strcmp(type,B_MAIL_TYPE) == 0)
			{
				fMailList.AddItem(item = new HMailItem(ref));
				::watch_node(&item->fNodeRef,B_WATCH_ATTR,this,fOwner->Window());
				if(item&&!item->IsRead())
					fUnread++;
			}
		} 
	} 
#else
	int32 count,i=0;
	struct dirent **dirents = NULL;
	path.SetTo(&fFolderRef);

	count = GetAllDirents(path.Path(),&dirents);
	while(count>i)
	{
		if(::strcmp(dirents[i]->d_name,".") == 0 || ::strcmp(dirents[i]->d_name,"..")== 0)
		{
			free(dirents[i++]);
			continue;
		}
		ref.device = dirents[i]->d_pdev;
		ref.directory = dirents[i]->d_pino;
		ref.set_name(dirents[i]->d_name);
		free(dirents[i++]);
		if(node.SetTo(&ref) != B_OK)
			continue;
		node.ReadAttr("BEOS:TYPE",B_STRING_TYPE,0,type,B_MIME_TYPE_LENGTH);
		if(::strcmp(type,B_MAIL_TYPE) == 0)
		{
			fMailList.AddItem(item = new HMailItem(ref));
			::watch_node(&item->fNodeRef,B_WATCH_ATTR,this,fOwner->Window());
			if(item&&!item->IsRead())
				fUnread++;
		}
	}
	free(dirents);
#endif		
	//fMailList.SortItems(HFolderItem::CompareFunc);
	fDone = true;
	// Set icon to open folder
	BBitmap *icon = ((HApp*)be_app)->GetIcon("OpenFolder");
	SetColumnContent(ICON_COLUMN,icon,2.0,false,false);
	
	SetName(fUnread);
	
	fThread = -1;
	InvalidateMe();
	//CreateCache();
	PRINT(("End gathering\n"));
	return;
}

/***********************************************************
 * SetName
 ***********************************************************/
void
HFolderItem::SetName(int32 unread)
{
	//if(!IsDone())
	//	return;
	fUnread = unread;
	
	if(fUnread>0)
	{
		BString title=fName;
		title << " [" << fUnread << "]";
		SetColumnContent(LABEL_COLUMN,title.String());
	}else
		SetColumnContent(LABEL_COLUMN,fName.String());
}

/***********************************************************
 * Refresh Cache
 ***********************************************************/
void
HFolderItem::RefreshCache()
{
	EmptyMailList();
	PRINT(("Refresh Folder\n"));
	SetName(0);
	fDone = false;
	fCancel = false;
	// Set icon to open folder
	BBitmap *icon = ((HApp*)be_app)->GetIcon("CloseFolder");
	SetColumnContent(ICON_COLUMN,icon,2.0,false,false);
	
	InvalidateMe();
	((HApp*)be_app)->MainWindow()->PostMessage(M_START_MAIL_BARBER_POLE);
	fUseCache = false;
	Gather();
	fUseCache = true;
	fRefreshThread = -1;
}

/***********************************************************
 * InvalidateMe
 ***********************************************************/
void
HFolderItem::InvalidateMe()
{
	BMessage msg(M_INVALIDATE);
	msg.AddInt32("index",((ColumnListView*)fOwner)->FullListIndexOf(this));
	fOwner->Window()->PostMessage(&msg,fOwner);
}

/***********************************************************
 * StartRefreshCache
 ***********************************************************/
void
HFolderItem::StartRefreshCache()
{
	if(fRefreshThread != -1&&fThread == -1)
		return;
	
	fRefreshThread = ::spawn_thread(RefreshCacheThread,"RefreshCache",B_NORMAL_PRIORITY,this);
	::resume_thread(fRefreshThread);
}

/***********************************************************
 *
 ***********************************************************/
int32
HFolderItem::RefreshCacheThread(void *data)
{
	HFolderItem *thisItem = (HFolderItem*)data;
	thisItem->RefreshCache();
	return 0;
}

/***********************************************************
 * StartCreateCache
 ***********************************************************/
void
HFolderItem::StartCreateCache()
{
	fCacheCancel = false;
	if(fCacheThread != -1)
		return;
	fCacheThread = ::spawn_thread(CreateCacheThread,"CreateCache",B_NORMAL_PRIORITY,this);
	::resume_thread(fCacheThread);
}


/***********************************************************
 * ReadFromCache
 ***********************************************************/
//#define __CALC__
#define __NEW_CACHE__
status_t
HFolderItem::ReadFromCache()
{
	if(!fUseCache)
		return B_ERROR;
#ifdef __CALC__
	BStopWatch stopWatch("Cache Timer");
#endif
	BPath path(&fFolderRef);
	BString name = path.Leaf();
	name += ".cache";
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("FolderCache");
	::create_directory(path.Path(),0777);
	path.Append(name.String());

	HMailCache cache(path.Path());
	int32 count = cache.CountItems();
	// check entry in directory and refs count
	if(fChildItems+count + fMailList.CountItems() != BDirectory(&fFolderRef).CountEntries() )
	{
		PRINT(("Auto refresh:%s\n",name.String()));
		EmptyMailList();
		return B_ERROR;
	}		
	//
	if(cache.Open(fMailList,this) == B_OK)
	{
		fDone = true;
		// Set icon to open folder
		BBitmap *icon = ((HApp*)be_app)->GetIcon("OpenFolder");
		SetColumnContent(ICON_COLUMN,icon,2.0,false,false);
#ifdef __CALC__
		PRINT(("Done: %9.4lf sec\n",stopWatch.Lap()/1000000.0));
#endif
		return B_OK;
	}else{
		PRINT(("Open ERROR\n"));
	}
	return B_ERROR;
}

/***********************************************************
 * CreateCache
 ***********************************************************/
void
HFolderItem::CreateCache()
{
	BPath path(&fFolderRef);
	
	BPath folder_path(&fFolderRef);
	BString name("");
	name = path.Leaf();
	const char *p;
	folder_path.GetParent(&folder_path);
	
	
	name += ".cache";
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("FolderCache");
	
	if( path.InitCheck() != B_OK)
		::create_directory(path.Path(),0777);
	while(1)
	{
		p = folder_path.Leaf();
		if(::strcmp(p,"mail") == 0)
			break;	
		path.Append(p);
		::create_directory(path.Path(),0777);
		folder_path.GetParent(&folder_path);
	}
	path.Append(name.String());
	fCacheCancel = false;

	HMailCache cache(path.Path());
	cache.Save(fMailList);
}

/***********************************************************
 * AddMailsToCacheFile
 ***********************************************************/
void
HFolderItem::AddMailsToCacheFile()
{
	if(!fUseCache)
		return;
	BPath path(&fFolderRef);
	BString name = path.Leaf();
	name << ".cache";
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("FolderCache");
	
	if( path.InitCheck() != B_OK)
		::create_directory(path.Path(),0777);
	path.Append(name.String());
	fCacheCancel = false;
	
#ifndef __NEW_CACHE__
	BMessage cache;
	
	BFile file(path.Path(),B_READ_WRITE|B_CREATE_FILE);
	if(file.InitCheck() == B_OK)
	{
		cache.Unflatten(&file);
		entry_ref ref;
		int32 count = fMailList.CountItems();
		for(int32 i = 0;i < count;i++)
		{
			HMailItem *item = (HMailItem*)fMailList.ItemAtFast(i);
			if(!item)
				continue;
			ref = item->fRef;
			cache.AddRef("refs",&ref);
			cache.AddString("status",item->fStatus);
			cache.AddString("subject",item->fSubject);
			cache.AddString("from",item->fFrom);
			cache.AddString("to",item->fTo);
			cache.AddString("cc",item->fCC);
			cache.AddString("reply",item->fReply);
			cache.AddInt64("when",(int64)item->fWhen);
			cache.AddString("priority",item->fPriority);
			cache.AddInt8("enclosure",item->fEnclosure);
			cache.AddInt64("ino_t",item->fNodeRef.node);
		}
		
		if(file.InitCheck() == B_OK)
		{
			file.Seek(0,SEEK_SET);
			ssize_t numBytes;
			cache.Flatten(&file,&numBytes);
			file.SetSize(numBytes);
		}
	}
#else
	if(fMailList.CountItems() > 0)
	{
		HMailCache cache(path.Path());
		cache.Append(fMailList);
	}
#endif
}

/***********************************************************
 * RemoveCacheFile
 ***********************************************************/
void
HFolderItem::RemoveCacheFile()
{
	BPath path(&fFolderRef);
	BString name = path.Leaf();
	name << ".cache";
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("FolderCache");
	path.Append(name.String());
	if(path.InitCheck() != B_OK)
		return;

	entry_ref ref;
	::get_ref_for_path(path.Path(),&ref);
	BMessenger tracker("application/x-vnd.Be-TRAK" );
	BMessage msg( B_DELETE_PROPERTY ) ; 

	BMessage specifier( 'sref' ) ;
	specifier.AddRef( "refs", &ref ) ; 
	specifier.AddString( "property", "Entry" ) ; 
	msg.AddSpecifier( &specifier ) ; 

	msg.AddSpecifier( "Poses" ) ; 
	msg.AddSpecifier( "Window", 1 ) ;
	
	BMessage reply ; 
    tracker.SendMessage( &msg, &reply ); 
}

/***********************************************************
 * CreateCacheThread
 ***********************************************************/
int32
HFolderItem::CreateCacheThread(void *data)
{
	HFolderItem *thisItem = (HFolderItem*)data;
	if(thisItem->IsDone())
		thisItem->CreateCache();
	thisItem->fCacheThread = -1;
	return 0;
}

/***********************************************************
 * EmptyMailList
 ***********************************************************/
void
HFolderItem::EmptyMailList()
{
	register int32 count = fMailList.CountItems();
	
	while(count>0)
		delete static_cast<HMailItem*>(fMailList.RemoveItem(--count));	
}

/***********************************************************
 * Launch
 ***********************************************************/
void
HFolderItem::Launch()
{
	BMessenger tracker("application/x-vnd.Be-TRAK" );
	BMessage msg(B_REFS_RECEIVED);
	msg.AddRef("refs",&fFolderRef);
	BMessage reply ; 
	tracker.SendMessage( &msg, &reply ); 
	//be_roster->Launch(&fFolderRef);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HFolderItem::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case B_QUERY_UPDATE:
	case B_NODE_MONITOR:
		NodeMonitor(message);
		break;
	default:
		BHandler::MessageReceived(message);
	}
}

/***********************************************************
 * NodeMonitor
 ***********************************************************/
void
HFolderItem::NodeMonitor(BMessage *message)
{
	int32 opcode;
	const char  *name;
	entry_ref ref;
	HMailItem *item;
	BPath path;
	BPath myPath(&fFolderRef);
	
	BNode node;
	char mime_type[B_MIME_TYPE_LENGTH];
	BNodeInfo ninfo;
		
	if(message->FindInt32("opcode",&opcode) != B_OK)	
		return;
	message->FindInt32("device", &ref.device); 
	message->FindInt64("directory", &ref.directory); 
	message->FindString("name", &name);
	
	
	switch(opcode)
	{
	case B_ENTRY_CREATED:
		ref.set_name(name);
		path.SetTo(&ref);
		if(!path.Path())
			break;
		if(node.SetTo(&ref) != B_OK)
			break;
		ninfo.SetTo(&node);
		ninfo.GetType(mime_type);
		
		if(::strcmp(mime_type,B_MAIL_TYPE) == 0)
		{
			AddMail((item = new HMailItem(ref)));
			if( IsSelected() )
				((HFolderList*)fOwner)->AddToMailList(item);
		
			InvalidateMe();
		}else if(node.IsDirectory()){
			HFolderItem *folder = new HFolderItem(ref,fOwner);
			BMessage msg(M_ADD_UNDER_ITEM);
			msg.AddPointer("parent",this);
			msg.AddPointer("item",folder);
			fOwner->Window()->PostMessage(&msg,fOwner);
		}
		PRINT(("Create\n"));
		break;
	case B_ENTRY_REMOVED:
	{
		node_ref nref;
		message->FindInt32("device", &nref.device);
		message->FindInt64("node", &nref.node);
		BDirectory dir(&nref);
		
		if(dir.IsDirectory())
		{
			BMessage msg(M_REMOVE_FOLDER);
			msg.AddInt64("node",nref.node);
			msg.AddInt32("device",nref.device);
			fOwner->Window()->PostMessage(&msg,fOwner);
		}else{
			HMailItem *item = RemoveMail(nref);
			if(IsSelected())
				((HFolderList*)fOwner)->RemoveFromMailList(item,true);
			else
				delete item;	
			InvalidateMe();
		}
		PRINT(("REMOVE\n"));
		break;
	}
	case B_ENTRY_MOVED:
	{
		node_ref nref;
		node_ref from_nref;
		BPath to_path,from_path;
		BDirectory to_dir,from_dir;
		entry_ref ref;
	
		message->FindInt32("device", &nref.device);
		message->FindInt32("device", &from_nref.device);
		message->FindInt64("to directory", &nref.node);
		message->FindInt64("from directory", &from_nref.node);
		to_dir.SetTo(&nref);
		from_dir.SetTo(&from_nref);
		
		to_path.SetTo(&to_dir,NULL,false);
		from_path.SetTo(&from_dir,NULL,false);
			
		message->FindString("name", &name);
		// Add mail
		if(to_path == myPath)
		{
			BPath file_path(to_path);
			file_path.Append(name);
			if(node.SetTo(file_path.Path()) != B_OK)
				break;
			
			ninfo.SetTo(&node);
			ninfo.GetType(mime_type);
			if(::strcmp(mime_type,B_MAIL_TYPE) == 0)
			{
				::get_ref_for_path(file_path.Path(),&ref);
				HMailItem* item;
				AddMail((item = new HMailItem(ref)));
				if(IsSelected())
					((HFolderList*)fOwner)->AddToMailList(item);
				InvalidateMe();
			}else if(node.IsDirectory()){
				::get_ref_for_path(file_path.Path(),&ref);
				HFolderItem *folder = new HFolderItem(ref,fOwner);
				BMessage msg(M_ADD_UNDER_ITEM);
				msg.AddPointer("parent",this);
				msg.AddPointer("item",folder);
				fOwner->Window()->PostMessage(&msg,fOwner);
			}	
		}
		// Remove mails
		if(from_path == myPath)
		{
			node_ref old_nref;
			old_nref.device =from_nref.device;
			message->FindInt64("node", &old_nref.node);
			BDirectory dir(&old_nref);
			
			if(dir.IsDirectory())
			{
				BMessage msg(M_REMOVE_FOLDER);
				msg.AddInt64("node",old_nref.node);
				msg.AddInt32("device",old_nref.device);
				fOwner->Window()->PostMessage(&msg,fOwner);
			}else{
				HMailItem *item = RemoveMail(old_nref);
				if(!item)
					break;
				if(IsSelected())
					((HFolderList*)fOwner)->RemoveFromMailList(item,true);
				else
					delete item;
				InvalidateMe();
			}
		}
		break;
		}
	case B_ATTR_CHANGED:
	{
		node_ref nref;
		message->FindInt64("node", &nref.node);
		message->FindInt32("device",&nref.device);
		
		HMailItem *item = FindMail(nref);
		bool read;
		if(item)
		{
			//PRINT(("Attr Changed\n"));
			read = item->IsRead();
			item->RefreshStatus();
			if(read != item->IsRead())
			{
				SetName((read)?fUnread+1:fUnread-1);
				InvalidateMe();
			}
			BMessage msg(M_INVALIDATE_MAIL);
			msg.AddPointer("mail",item);
			fOwner->Window()->PostMessage(&msg);
		}
		
		break;
	}
	}
}

/***********************************************************
 * CompareFunc
 ***********************************************************/
int
HFolderItem::CompareFunc(const void *data1,const void* data2)
{
	const HMailItem *item1 = (*(const HMailItem**)data1);
	const HMailItem *item2 = (*(const HMailItem**)data2);

	if(item1->fWhen > item2->fWhen)
		return 1;
	else if(item1->fWhen < item2->fWhen)
		return -1;
	else
		return 0;
	return 0;	
}

/***********************************************************
 * IsSelected
 ***********************************************************/
bool
HFolderItem::IsSelected()
{
	int32 sel =  fOwner->CurrentSelection();
	if(sel < 0 || sel != ((ColumnListView*)fOwner)->FullListIndexOf(this))
		return false;
	return true;
}

/***********************************************************
 * RemoveSettings
 ***********************************************************/
void
HFolderItem::RemoveSettings()
{
	
}

/***********************************************************
 * CompareItems
 ***********************************************************/
int HFolderItem::CompareItems(const CLVListItem *a_Item1, 
								const CLVListItem *a_Item2, 
								int32 KeyColumn)
{
	const HFolderItem* Item1 = cast_as(a_Item1,const HFolderItem);
	const HFolderItem* Item2 = cast_as(a_Item2,const HFolderItem);
	
	if(Item1->FolderType() != Item2->FolderType())
		return (Item1->FolderType() == QUERY_TYPE)?1:-1;	
	
	if(Item1 == NULL || Item2 == NULL || Item1->m_column_types.CountItems() <= KeyColumn ||
		Item2->m_column_types.CountItems() <= KeyColumn)
		return 0;
	
	int32 type1 = ((int32)Item1->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;
	int32 type2 = ((int32)Item2->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;

	if(!((type1 == CLVColStaticText || type1 == CLVColTruncateText || type1 == CLVColTruncateUserText ||
		type1 == CLVColUserText) && (type2 == CLVColStaticText || type2 == CLVColTruncateText ||
		type2 == CLVColTruncateUserText || type2 == CLVColUserText)))
		return 0;

	const char* text1 = NULL;
	const char* text2 = NULL;

	if(type1 == CLVColStaticText || type1 == CLVColTruncateText)
		text1 = (const char*)Item1->m_column_content.ItemAt(KeyColumn);
	else if(type1 == CLVColTruncateUserText || type1 == CLVColUserText)
		text1 = Item1->GetUserText(KeyColumn,-1);

	if(type2 == CLVColStaticText || type2 == CLVColTruncateText)
		text2 = (const char*)Item2->m_column_content.ItemAt(KeyColumn);
	else if(type2 == CLVColTruncateUserText || type2 == CLVColUserText)
		text2 = Item2->GetUserText(KeyColumn,-1);
	
	// display in and out folder first.
	if(strcmp(text1,"in") == 0) return -1;
	if(strcmp(text2,"in") == 0) return  1;
	if(strcmp(text1,_("Local Folders")) == 0) return -1;
	if(strcmp(text2,_("Local Folders")) == 0) return  1;
	
	if(strcmp(text1,"out") == 0 && strcmp(text2,"in") != 0) return -1;
	if(strcmp(text2,"out") == 0 && strcmp(text1,"in") != 0) return  1;
	
	return strcasecmp(text1,text2);
}
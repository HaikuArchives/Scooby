#include "HFolderItem.h"
#include "HApp.h"
#include "ResourceUtils.h"
#include "HMailItem.h"
#include "HPrefs.h"
#include "HFolderList.h"

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

#define ICON_COLUMN 1
#define LABEL_COLUMN 2

/***********************************************************
 * Constructor
 ***********************************************************/
HFolderItem::HFolderItem(entry_ref ref,BListView *target)
	: CLVEasyItem(0, false, false, 20.0)
	,fDone(false)
	,fUnread(0)
	,fThread(-1)
	,fCancel(false)
	,fFolderRef(ref)
	,fOwner(target)
	,fCacheThread(-1)
	,fCacheCancel(false)
	,fRefreshThread(-1)
	,fIsQuery(false)
{	
	if(BPath(&ref).InitCheck() == B_OK)
		fName = BPath(&ref).Leaf();
	else
		fName = "Unknown";
	fMailList.MakeEmpty();
	BEntry entry(&ref);
	
	BBitmap *icon(NULL);
	if(!entry.IsDirectory())
	{
		icon = ResourceUtils().GetBitmapResource('BBMP',"CloseQuery");
		fIsQuery = true;
	}else
		icon = ResourceUtils().GetBitmapResource('BBMP',"CloseFolder");

	SetColumnContent(ICON_COLUMN,icon,2.0,true,false);
	SetColumnContent(LABEL_COLUMN,BPath(&ref).Leaf());
	//if(!fIsQuery)
	//	StartGathering();
	((HApp*)be_app)->Prefs()->GetData("use_folder_cache",&fUseCache);
	delete icon;
}

/***********************************************************
 * Destructor
 ***********************************************************/
HFolderItem::~HFolderItem()
{
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
	
	if(IsDone() && !IsQuery() && fUseCache)
		CreateCache();
	else if(!IsDone() && !IsQuery()&& fUseCache)
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
}

/***********************************************************
 * RemoveMail
 ***********************************************************/
void
HFolderItem::RemoveMail(HMailItem* item)
{
	fMailList.RemoveItem(item);
}

/***********************************************************
 * RemoveMail
 ***********************************************************/
HMailItem*
HFolderItem::RemoveMail(entry_ref ref)
{
	int32 count = fMailList.CountItems();
	for(int32 i = 0;i< count;i++)
	{
		HMailItem *item = (HMailItem*)fMailList.ItemAt(i);
		if(!item)
			continue;
		if(item->Ref() == ref)
			return (HMailItem*)fMailList.RemoveItem(i);
	}
	return NULL;
}

/***********************************************************
 * RemoveMail
 ***********************************************************/
HMailItem*
HFolderItem::RemoveMail(node_ref nref)
{
	node_ref old_nref;
	int32 count = fMailList.CountItems();
	for(int32 i = 0;i< count;i++)
	{
		HMailItem *item = (HMailItem*)fMailList.ItemAt(i);
		if(!item)
			continue;
		entry_ref ref = item->Ref();
		BEntry entry(&ref);
		entry.GetNodeRef(&old_nref);
		if(old_nref.node == nref.node)
			return (HMailItem*)fMailList.RemoveItem(i);
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
	for(int32 i = 0;i < count;i++)
	{
		item = (HMailItem*)list->ItemAt(i);
		if(item&&!item->IsRead())
			fUnread++;
	}
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
		item = (HMailItem*)list->ItemAt(i);
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
	fThread = ::spawn_thread(ThreadFunc,"MailGathering",B_NORMAL_PRIORITY,this);
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
	list->InvalidateItem(list->IndexOf(item));
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
   	status_t err = B_NO_ERROR;
	HMailItem *item(NULL);
	char type[B_MIME_TYPE_LENGTH+1];
	entry_ref ref;
	BNode node;
	dir.Rewind();
	
	while( err == B_OK && !fCancel)
	{
		if( (err = dir.GetNextRef( &ref )) == B_OK)
		{
			item = NULL;
			type[0] = '\0';
			if(node.SetTo(&ref) != B_OK)
				continue;
			node.ReadAttr("BEOS:TYPE",B_STRING_TYPE,0,type,B_MIME_TYPE_LENGTH);
			if(::strcmp(type,"text/x-email") == 0)
			{
				fMailList.AddItem(item = new HMailItem(ref));
				if(item&&!item->IsRead())
					fUnread++;
			}
		}
	}
	
	//fMailList.SortItems(HFolderItem::CompareFunc);
	fDone = true;
	// Set icon to open folder
	BBitmap *icon = ResourceUtils().GetBitmapResource('BBMP',"OpenFolder");
	SetColumnContent(ICON_COLUMN,icon,2.0,true,false);
	delete icon;
	
	SetName(fUnread);
	
	fThread = -1;
	InvalidateMe();
	//CreateCache();
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
		SetColumnContent(LABEL_COLUMN,BPath(&fFolderRef).Leaf());
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
	BBitmap *icon = ResourceUtils().GetBitmapResource('BBMP',"CloseFolder");
	SetColumnContent(ICON_COLUMN,icon,2.0,true,false);
	delete icon;
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
	msg.AddInt32("index",fOwner->IndexOf(this));
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
status_t
HFolderItem::ReadFromCache()
{
	if(!fUseCache)
		return B_ERROR;
		
	BPath path(&fFolderRef);
	BString name = path.Leaf();
	name << ".cache";
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("FolderCache");
	::create_directory(path.Path(),0777);
	path.Append(name.String());
	
	BFile file(path.Path(),B_READ_ONLY);
	BMessage msg;
	if(file.InitCheck() == B_OK)
	{
		msg.Unflatten(&file);
		int32 count;
		type_code type;
		msg.GetInfo("refs",&type,&count);
		
		// check entry in directory and refs count
		BDirectory dir(&fFolderRef);
		if(count + fMailList.CountItems() != dir.CountEntries() )
		{
			EmptyMailList();
			return B_ERROR;
		}
		//
		
		entry_ref ref;
		const char *status,*subject,*from,*to,*cc,*priority;
		int8 enclosure;
		int64 when;
		HMailItem *item;
		for(register int32 i = 0;i < count;i++)
		{
			if(msg.FindRef("refs",i,&ref) ==B_OK)
			{
				msg.FindString("subject",i,&subject);
				msg.FindString("status",i,&status);
				msg.FindString("from",i,&from);
				msg.FindString("to",i,&to);
				msg.FindString("cc",i,&cc);
				msg.FindInt64("when",i,&when);
				msg.FindString("priority",i,&priority);
				msg.FindInt8("enclosure",i,&enclosure);
		
				fMailList.AddItem(item = new HMailItem(ref,
													status,
													subject,
													from,
													to,
													cc,
													(time_t)when,
													priority,
													enclosure));
				if(item&&!item->IsRead())
					fUnread++;
			}
		}	
		fDone = true;
		// Set icon to open folder
		BBitmap *icon = ResourceUtils().GetBitmapResource('BBMP',"OpenFolder");
		SetColumnContent(ICON_COLUMN,icon,2.0,true,false);
		delete icon;
		return B_OK;
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
	
	
	name << ".cache";
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
	
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
	if(file.InitCheck() == B_OK)
	{
		int32 count = fMailList.CountItems();
		BMessage cache(B_SIMPLE_DATA);
		cache.MakeEmpty();
		HMailItem *item;
		entry_ref ref;
		for(register int32 i =0;i < count;i++)
		{
			item = (HMailItem*)fMailList.ItemAt(i);
			if(!item)
				continue;
			ref = item->fRef;
			cache.AddRef("refs",&ref);
			cache.AddString("status",item->fStatus);
			cache.AddString("subject",item->fSubject);
			cache.AddString("from",item->fFrom);
			cache.AddString("to",item->fTo);
			cache.AddString("cc",item->fCC);
			cache.AddInt64("when",(int64)item->fWhen);
			cache.AddString("priority",item->fPriority);
			cache.AddInt8("enclosure",item->fEnclosure);
			if(fCacheCancel)
				break;
		}
		file.Seek(0,SEEK_SET);
		ssize_t numBytes;
		cache.Flatten(&file,&numBytes);
		file.SetSize(numBytes);
	}
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
	
	BMessage cache;
	
	BFile file(path.Path(),B_READ_WRITE|B_CREATE_FILE);
	if(file.InitCheck() == B_OK)
	{
		cache.Unflatten(&file);
		entry_ref ref;
		int32 count = fMailList.CountItems();
		for(int32 i = 0;i < count;i++)
		{
			HMailItem *item = (HMailItem*)fMailList.ItemAt(i);
			if(!item)
				continue;
			ref = item->fRef;
			cache.AddRef("refs",&ref);
			cache.AddString("status",item->fStatus);
			cache.AddString("subject",item->fSubject);
			cache.AddString("from",item->fFrom);
			cache.AddString("to",item->fTo);
			cache.AddString("cc",item->fCC);
			cache.AddInt64("when",(int64)item->fWhen);
			cache.AddString("priority",item->fPriority);
		}
		
		if(file.InitCheck() == B_OK)
		{
			file.Seek(0,SEEK_SET);
			ssize_t numBytes;
			cache.Flatten(&file,&numBytes);
			file.SetSize(numBytes);
		}
	}
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
 * CompareItems
 ***********************************************************/
int HFolderItem::CompareItems(const CLVListItem *a_Item1, 
								const CLVListItem *a_Item2, 
								int32 KeyColumn)
{
	const HFolderItem* Item1 = cast_as(a_Item1,const HFolderItem);
	const HFolderItem* Item2 = cast_as(a_Item2,const HFolderItem);
	
	if(Item1->IsQuery() != Item2->IsQuery())
		return (Item1->IsQuery())?1:-1;	
	
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
	
	if(strcmp(text1,"out") == 0 && strcmp(text2,"in") != 0) return -1;
	if(strcmp(text2,"out") == 0 && strcmp(text1,"in") != 0) return  1;
	
	return strcasecmp(text1,text2);
}
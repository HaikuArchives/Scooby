#include "HQueryItem.h"
#include "HMailItem.h"
#include "ResourceUtils.h"
#include "HFolderList.h"

#include <Node.h>
#include <Bitmap.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Query.h>
#include <Debug.h>
#include <Path.h>
#include <File.h>
#include <Window.h>
#include <NodeMonitor.h>
#include <Autolock.h>
#include <FindDirectory.h>
#include <SymLink.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HQueryItem::HQueryItem(const entry_ref &ref,
						BListView *target)
			:HFolderItem(ref,target)
			,fPredicate("")
			,fQuery(NULL)
			,fMessenger(NULL)
{
	BNode node(&ref);
	fQuery = new BQuery();
	
	fMessenger = new BMessenger((BHandler*)this,(BLooper*)target->Window());
	fQuery->SetTarget(*fMessenger);
	BString type;
	
	if(node.IsSymLink())
	{
		BEntry entry(&ref);
		BSymLink link(&entry);
		entry_ref new_ref;
		BPath path;
		
		char buf[B_PATH_NAME_LENGTH+1];
		ssize_t len = link.ReadLink(buf,B_PATH_NAME_LENGTH);
		buf[len] = '\0';
		path.SetTo(buf);
		::get_ref_for_path(path.Path(),&new_ref);
		node.SetTo(&new_ref);
	}
	if(node.InitCheck() == B_OK)
		node.ReadAttrString("_trk/qrystr",&fPredicate);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HQueryItem::~HQueryItem()
{
	EmptyMailList();
	delete fQuery;
	delete fMessenger;
}

/***********************************************************
 * StartRefreshCache
 ***********************************************************/
void
HQueryItem::StartRefreshCache()
{
	PRINT(("Refresh Query\n"));
	EmptyMailList();
	if(fThread != -1)
		return;
	fDone = false;
	// Set icon to open folder
	BBitmap *icon = ResourceUtils().GetBitmapResource('BBMP',"CloseQuery");
	SetColumnContent(1,icon,2.0,true,false);
	delete icon;
	StartGathering();
}

/***********************************************************
 * StartGathering
 ***********************************************************/
void
HQueryItem::StartGathering()
{
	if(fThread != -1)
		return;
	//PRINT(("Query Gathering\n"));
	fThread = ::spawn_thread(FetchingThread,"QueryFetch",B_NORMAL_PRIORITY,this);
	::resume_thread(fThread);
}

/***********************************************************
 * Fetching
 ***********************************************************/
void
HQueryItem::Fetching()
{
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);
	fQuery->SetVolume(&volume);
	
	fQuery->SetPredicate(fPredicate.String());
	
	BString type;
	BNode node;
	HMailItem *item(NULL);
	if(fQuery->Fetch() == B_OK)
	{
		entry_ref ref;
		while(fQuery->GetNextRef(&ref) == B_OK)
		{
			if(node.SetTo(&ref) != B_OK)
				continue;
			if(node.ReadAttrString("BEOS:TYPE",&type)!= B_OK)
				continue;
			if(type.Compare("text/x-email") == 0)
			{
				fMailList.AddItem(item = new HMailItem(ref));
				if(item && !item->IsRead() )
					fUnread++;
			}
		}
		fDone = true;
		
		// Set icon to open folder
		BBitmap *icon = ResourceUtils().GetBitmapResource('BBMP',"OpenQuery");
		SetColumnContent(1,icon,2.0,true,false);
		delete icon;
		SetName(fUnread);
		InvalidateMe();
	}DEBUG_ONLY(
	else{
		PRINT(("Query fetching was failed\n"));
		}
	);
	fThread = -1;
}

/***********************************************************
 * FetchingThread
 ***********************************************************/
int32
HQueryItem::FetchingThread(void* data)
{
	HQueryItem *item =(HQueryItem*)data;
	item->Fetching();
	return 0;
}
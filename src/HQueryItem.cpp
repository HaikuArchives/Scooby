#include "HQueryItem.h"
#include "HMailItem.h"
#include "HApp.h"
#include "HFolderList.h"
#include "Utilities.h"

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
#include <E-mail.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HQueryItem::HQueryItem(const entry_ref &ref,
						BListView *target)
			:HFolderItem(ref,target)
			,fPredicate("")
			,fMessenger(NULL)
{
	BNode node(&ref);
	fMessenger = new BMessenger((BHandler*)this,(BLooper*)target->Window());
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
		ReadNodeAttrString(&node,"_trk/qrystr",&fPredicate);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HQueryItem::~HQueryItem()
{
	// Make sure the fetching thread isn't running
	if(fThread >= 0)
	{
		status_t err;
		fCancel = true;
		::wait_for_thread(fThread,&err);
		fThread = -1;
	}

	EmptyMailList();
	while (fQueries.CountItems())
	{
		delete static_cast<BQuery *>(fQueries.RemoveItem((int32)0));
	}
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
	BBitmap *icon = ((HApp*)be_app)->GetIcon("CloseQuery");
	SetColumnContent(1,icon,2.0,false,false);
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
	bool some_success = false;
	BVolume volume;
	BVolumeRoster roster;
	

	while (fQueries.CountItems())
	{
		delete static_cast<BQuery *>(fQueries.RemoveItem((int32)0));
	}
	uint32 count_items = 0;


	while (roster.GetNextVolume(&volume) == B_OK)
	{
		BQuery *query = new BQuery;
		fQueries.AddItem((void*)query);
	
		query->Clear();
		query->SetTarget(*fMessenger);
		query->SetVolume(&volume);
		query->SetPredicate(fPredicate.String());
		
		char type[B_MIME_TYPE_LENGTH+1];
		BNode node;
		HMailItem *item(NULL);
		if(query->Fetch() == B_OK)
		{
			some_success = true;
			entry_ref ref;
			char buf[4096];
			dirent *dent;
			int32 count;
			int32 offset;
		
			while (((count = query->GetNextDirents((dirent *)buf, 4096)) > 0) && (!fCancel))
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
					if(node.SetTo(&ref) != B_OK)
						continue;
					node.ReadAttr("BEOS:TYPE",'MIMS',0,type,B_MIME_TYPE_LENGTH);
					if(::strcmp(type,B_MAIL_TYPE) == 0)
					{
						fMailList.AddItem(item = new HMailItem(ref));
						if(item && !item->IsRead() )
							count_items++;
					}
				}
			}
		}DEBUG_ONLY(
		else{
			PRINT(("Query fetching was failed\n"));
			}
		);
	}

	if (some_success)
	{
		fDone = true;
		// Set icon to open folder
		BBitmap *icon = ((HApp*)be_app)->GetIcon("OpenQuery");
		SetColumnContent(1,icon,2.0,false,false);
		SetUnreadCount(count_items);
		InvalidateMe();
	}

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

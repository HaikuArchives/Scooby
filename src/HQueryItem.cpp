#include "HQueryItem.h"
#include "HMailItem.h"
#include "ResourceUtils.h"

#include <Node.h>
#include <Bitmap.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Query.h>
#include <Debug.h>
#include <Path.h>
#include <File.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HQueryItem::HQueryItem(const entry_ref &ref,
						BListView *target)
			:HFolderItem(ref,target)
			,fPredicate("")
{
	BNode node(&ref);
	BString type;
	if(node.InitCheck() == B_OK)
		node.ReadAttrString("_trk/qrystr",&fPredicate);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HQueryItem::~HQueryItem()
{
	EmptyMailList();
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
	BBitmap *icon = ResourceUtils().GetBitmapResource('BBMP',"CloseFolder");
	SetColumnContent(0,icon,2.0,true,false);
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
 *
 ***********************************************************/
void
HQueryItem::Fetching()
{
	BQuery query;
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);
	query.SetVolume(&volume);
	
	query.SetPredicate(fPredicate.String());
	
	BString type;
	BNode node;
	HMailItem *item(NULL);
	if(query.Fetch() == B_OK)
	{
		entry_ref ref;
		while(query.GetNextRef(&ref) == B_OK)
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
		SetColumnContent(0,icon,2.0,true,false);
		delete icon;
		InvalidateMe();
	}DEBUG_ONLY(
	else{
		PRINT(("Query fetching was failed"));
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
#include "HFolderList.h"
#include "CLVColumn.h"
#include "HFolderItem.h"
#include "HQueryItem.h"
#include "ResourceUtils.h"
#include "HMailList.h"
#include "HApp.h"
#include "HPrefs.h"

#include <Window.h>
#include <StorageKit.h>
#include <Autolock.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Debug.h>
#include <Path.h>
#include <ClassInfo.h>
#include <NodeMonitor.h>

#define TREE_MODE

/***********************************************************
 * Constructor
 ***********************************************************/
HFolderList::HFolderList(BRect frame,
						BetterScrollView **scroll,
						const char* title)
	:ColumnListView(frame,
					(CLVContainerView**)scroll,
					title,
					B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM,
					B_WILL_DRAW|B_PULSE_NEEDED,
					B_SINGLE_SELECTION_LIST,
					true   )
	,fCancel(false)
	,fThread(-1)
	,fWatching(false)
	,fSkipGathering(false)
{
	CLVColumn *expander_col;
	this->AddColumn( expander_col = new CLVColumn(NULL,0.0,CLV_EXPANDER|CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE));
	expander_col->SetShown(false);
	this->AddColumn(new CLVColumn(NULL,20,CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE|
		CLV_NOT_RESIZABLE|CLV_PUSH_PASS|CLV_MERGE_WITH_RIGHT));
	this->AddColumn(new CLVColumn("Folders",1024,CLV_SORT_KEYABLE|CLV_NOT_MOVABLE|CLV_PUSH_PASS));
	SetViewColor(tint_color( ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));
	SetInvocationMessage(new BMessage(M_OPEN_FOLDER));
	SetSortFunction(HFolderItem::CompareItems);

	SetSortKey(2);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HFolderList::~HFolderList()
{
	::stop_watching(this,Window());
	if(fThread >= 0)
	{
		fCancel = true;
		status_t err;
		::wait_for_thread(fThread,&err);
	}
	DeleteAll();
	SetInvocationMessage(NULL);
}

/***********************************************************
 * WatchQueryFolder
 ***********************************************************/
void
HFolderList::WatchQueryFolder()
{
	// Watch Query Folder
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append(QUERY_FOLDER);
	entry_ref ref;
	::get_ref_for_path(path.Path(),&ref);
	
	BEntry entry(&ref);
	node_ref nref;
	entry.GetNodeRef(&nref);
	::watch_node(&nref,B_WATCH_DIRECTORY|B_WATCH_NAME,this,Window());
}


/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HFolderList::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_GET_VOLUMES:
	{
		fThread = ::spawn_thread(GetFolders,"GetVolume",B_NORMAL_PRIORITY,this);
		::resume_thread(fThread);
		break;
	}
	case M_ADD_UNDER_ITEM:
	{
		CLVColumn *expander_col = ColumnAt(0);
		if(!expander_col->IsShown())
			expander_col->SetShown(true);
		int32 count;
		type_code type;
		message->GetInfo("item",&type,&count);
		HFolderItem *item;
		HFolderItem *parent;
		
		bool gather;
		((HApp*)be_app)->Prefs()->GetData("load_list_on_start_up",&gather);
	
		for(int32 i = 0;i < count;i++)
		{
			message->FindPointer("item",i,(void**)&item);
			message->FindPointer("parent",i,(void**)&parent);
			AddUnder(item,parent);
			fPointerList.AddItem(item);
			if(gather && !item->IsQuery())
					item->StartGathering();
		}
		break;
	}
	case M_ADD_FOLDER:
	{
		int32 count;
		type_code type;
		message->GetInfo("item",&type,&count);
		HFolderItem *item;
		BList list;
		bool gather;
		((HApp*)be_app)->Prefs()->GetData("load_list_on_start_up",&gather);
	
		for(int32 i = 0;i < count;i++)
		{
			if(message->FindPointer("item",i,(void**)&item) ==B_OK)
			{
				list.AddItem(item);	
				if(gather && !item->IsQuery())
					item->StartGathering();
			}
		}
		AddList(&list);
		SortItems();
		fPointerList.AddList(&list);
		break;
	}
	
	case M_INVALIDATE:
	{
		int32 i;
		if(message->FindInt32("index",&i) == B_OK)
		{
			InvalidateItem(i);
			
			//if(CurrentSelection()==i)
			//	SelectionChanged();
		}
		break;
	}	
	case B_NODE_MONITOR:
	{
		NodeMonitor(message);
		break;
	}
	default:
		if(message->WasDropped())
			this->WhenDropped(message);
		else
			ColumnListView::MessageReceived(message);
	}
}

/***********************************************************
 * FindFolder
 ***********************************************************/
int32
HFolderList::FindFolder(entry_ref ref)
{
	int32 count = CountItems();
	for(int32 i = 0;i < count;i++)
	{
		HFolderItem *item = cast_as(ItemAt(i),HFolderItem);
		if(item && !item->IsQuery())
		{
			if(item->Ref() == ref)
				return i;
		}
	}
	return -1;
}

/***********************************************************
 * FindQuery
 ***********************************************************/
int32
HFolderList::FindQuery(const char* name)
{
	int32 count = CountItems();
	for(int32 i = 0;i < count;i++)
	{
		HFolderItem *item = cast_as(ItemAt(i),HFolderItem);
		if(item && item->IsQuery())
		{
			if(strcmp(item->Name() , name) == 0)
				return i;
		}
	}
	return -1;
}

/***********************************************************
 * GetFolders
 ***********************************************************/
int32
HFolderList::GetFolders(void* data)
{
	HFolderList *list = (HFolderList*)data;
	BPath path;
	find_directory(B_USER_DIRECTORY,&path);
	path.Append("mail");
	BDirectory dir( path.Path());
	status_t err = B_NO_ERROR;
	BEntry entry(path.Path());
	entry_ref ref;
	HFolderItem *item;
	//
   	entry.GetRef(&ref);

	BMessage msg(M_ADD_FOLDER);
	msg.MakeEmpty();
	BMessage childMsg(M_ADD_UNDER_ITEM);
	childMsg.MakeEmpty();
	while( err == B_OK && !list->fCancel )
	{
		err = dir.GetNextEntry(&entry,false);	
		if( entry.InitCheck() != B_NO_ERROR )
			break;
		else if(entry.IsDirectory()){
			char link_path[B_PATH_NAME_LENGTH];
			BSymLink link(path.Path());
			if(B_BAD_VALUE == link.ReadLink(link_path,B_PATH_NAME_LENGTH)&& entry.IsDirectory())
			{
				char name[B_FILE_NAME_LENGTH];
				entry.GetName(name);
				entry.GetRef(&ref);
				item = new HFolderItem(ref,list);
				msg.AddPointer("item",item);

				GetChildFolders(entry,item,list,childMsg);
				//node_ref nref;
				//entry.GetNodeRef(&nref);
				//::watch_node(&nref,B_WATCH_DIRECTORY|B_WATCH_NAME,list,list->Window());
			}
		}
	}
	err = B_OK;
	/********* QUERY ***********/
	find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append( APP_NAME );
	path.Append( QUERY_FOLDER );
	if(dir.SetTo(path.Path()) != B_OK)
		dir.CreateDirectory(path.Path(),&dir);
	HQueryItem *query(NULL);
	BNode node;
	
	BString type;
	while( err == B_OK && !list->fCancel )
	{
		err = dir.GetNextEntry(&entry,false);	
		if( entry.InitCheck() != B_NO_ERROR )
			break;
		if( entry.GetPath(&path) != B_NO_ERROR )
			break;
		else{
			entry.GetRef(&ref);
			if( node.SetTo(&ref) != B_OK)
				continue;
			// Read SymLink
			if(entry.IsSymLink())
			{
				BSymLink link(&entry);
				
				char buf[B_PATH_NAME_LENGTH+1];
				ssize_t len = link.ReadLink(buf,B_PATH_NAME_LENGTH);
				buf[len] = '\0';
				path.SetTo(buf);
				if(path.InitCheck() != B_OK)
					continue;
				::get_ref_for_path(path.Path(),&ref);
				if(node.SetTo(&ref) != B_OK)
					continue;
			}	
			if( node.ReadAttrString("BEOS:TYPE",&type) != B_OK)
				continue;
			if(type.Compare("application/x-vnd.Be-query") != 0)
				continue;
			
			query = new HQueryItem(ref,list);
			msg.AddPointer("item",query);
		}
	}
	// Send add list items message
	if(!msg.IsEmpty())
		list->Window()->PostMessage(&msg,list);
	if(!childMsg.IsEmpty())
		list->Window()->PostMessage(&childMsg,list);
	list->fThread =  -1;
	return 0;
}

/***********************************************************
 * GetChildFolders
 ***********************************************************/
void
HFolderList::GetChildFolders(const BEntry &inEntry,
							HFolderItem *parentItem,
							HFolderList *list,
							BMessage &outList)
{
	BDirectory dir(&inEntry);
	BEntry entry;
	status_t err = B_OK;
	entry_ref ref;
	HFolderItem *item(NULL);
	
	while(err == B_OK)
	{
		err = dir.GetNextEntry(&entry,false);
		if(err != B_OK)
			continue;
		if(entry.IsDirectory())
		{
			entry.GetRef(&ref);
			outList.AddPointer("item",(item = new HFolderItem(ref,list)));
			outList.AddPointer("parent",parentItem);
			if(!parentItem->IsSuperItem())
				parentItem->SetSuperItem(true);
			GetChildFolders(entry,item,list,outList);
		}else // if the subfolder contains files, 
			  // 	break loop to improve speed
			break;
	}
}

/***********************************************************
 * DeleteAll
 ***********************************************************/
void
HFolderList::DeleteAll()
{
	BAutolock lock(Window());
	int32 count = fPointerList.CountItems();

	while(count>0)
	{
		HFolderItem *item = (HFolderItem*)fPointerList.RemoveItem(--count);
		delete item;
	}
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HFolderList::Pulse()
{
	if(!fWatching)
		return;
	int32 sel = this->CurrentSelection();
	if(sel >=0)
	{
		HFolderItem *theItem = (HFolderItem*)this->ItemAt(sel);
		if(theItem->IsDone()) // Check wait for the end of gathering
			this->SelectionChanged();
	}
}

/***********************************************************
 * WhenDropped
 ***********************************************************/
void
HFolderList::WhenDropped(BMessage *message)
{
	int32 count;
	type_code type;
	message->GetInfo("pointer",&type,&count);
	int32 from;
	message->FindInt32("sel",&from);
	
	HFolderItem *fromFolder = cast_as(ItemAt(from),HFolderItem);
	HFolderItem *toFolder = cast_as(ItemAt(CurrentSelection()),HFolderItem);
	if(fromFolder == toFolder)
		return;
	
	PRINT(("From:%d To:%d\n",from,CurrentSelection()));
	
	BMessage msg(M_MOVE_MAIL);
	HMailItem *mail;
	for(int32 i = 0;i < count;i++)
	{
		message->FindPointer("pointer",i,(void**)&mail);
		msg.AddPointer("mail",mail);
	}
	
	if(fromFolder&&toFolder)
	{
		msg.AddPointer("to",toFolder);
		msg.AddPointer("from",fromFolder);
		Window()->PostMessage(&msg);
	}
	
	SelectWithoutGathering(from);
}

/***********************************************************
 * SelectionChanged
 ***********************************************************/
void
HFolderList::SelectionChanged()
{
	int32 sel = this->CurrentSelection();
	if(sel >=0)
	{
		if(fSkipGathering)
			return;
		Window()->PostMessage(M_START_MAIL_BARBER_POLE);
		HFolderItem *theItem = (HFolderItem*)this->ItemAt(sel);
		if(!theItem->IsDone()) // if it has not gathered , assign it to check target
		{
			theItem->StartGathering();
			fWatching = true;
			return;
		}		
		fWatching = false;
		BList *list = theItem->MailList();
		InvalidateItem(sel);
		BMessage msg(M_LOCAL_SELECTION_CHANGED);
		msg.AddPointer("pointer",list);
		entry_ref ref = theItem->Ref();
		msg.AddRef("refs",&ref);
		msg.AddBool("query",theItem->IsQuery());
		Window()->PostMessage(&msg);
	}
}


/***********************************************************
 * MouseMoved
 ***********************************************************/
void
HFolderList::MouseMoved(BPoint point,
						uint32 transit,
						const BMessage *message)
{
	if(message)
	{
		if(message->what == M_MAIL_DRAG && transit == B_INSIDE_VIEW)
		{
			if( SelectItem(point) != B_OK)
			{
				int32 sel;
				message->FindInt32("sel",&sel);
				SelectWithoutGathering(sel);
			}
		}else if(message->what == M_MAIL_DRAG&&
				(transit == B_EXITED_VIEW||transit == B_OUTSIDE_VIEW)){
			int32 sel;
			message->FindInt32("sel",&sel);
			
			SelectWithoutGathering(sel);
		}
	}
	ColumnListView::MouseMoved(point,transit,message);
}

/***********************************************************
 * SelectWithoutGathering
 ***********************************************************/
void
HFolderList::SelectWithoutGathering(int32 index)
{
	fSkipGathering = true;
	Select(index);
	fSkipGathering = false;
}

/***********************************************************
 * Find item by point
 ***********************************************************/
status_t
HFolderList::SelectItem(const BPoint point)
{
	int32 count = CountItems();
	
	CLVColumn *col = ColumnAt(2);
	const float column_width = col->Width();

	for(register int32 i = 0;i < count;i++)
	{
		BRect rect = ItemFrame(i);
		// Only name column
		rect.right = rect.left + column_width;
		//rect.PrintToStream();
		//Bounds().PrintToStream();
		if( rect.Contains(point) )
		{
			HFolderItem *item = cast_as( ItemAt(i), HFolderItem);
			if(item->IsQuery())
				return B_ERROR;
			if(item)
			{
				SelectWithoutGathering(i);
				return B_OK;
			}
		}
	}
	return B_ERROR;
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HFolderList::MouseDown(BPoint pos)
{
	int32 buttons = 0; 
	ResourceUtils utils;
	
	BPoint point = pos;
	
    Window()->CurrentMessage()->FindInt32("buttons", &buttons); 
    this->MakeFocus(true);
	
    // 右クリックのハンドリング 
    if(buttons == B_SECONDARY_MOUSE_BUTTON)
    {
    	 /*int32 sel = IndexOf(pos);
    	 if(sel >= 0)
    	 	Select(sel);
    	 else
    	 	DeselectAll();
    	 */
    	 int32 sel = CurrentSelection();
    	 BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
    	 BFont font(be_plain_font);
    	 font.SetSize(10);
    	 theMenu->SetFont(&font);
    	 
    	
    	 BMenuItem *item = new BMenuItem("Recreate Cache",new BMessage(M_REFRESH_CACHE),0,0);
    	 if(sel < 0)
    	 	item->SetEnabled(false);
    	 else{
    	 	HFolderItem *folder= cast_as(ItemAt(sel),HFolderItem);
    	 	if(folder)
    	 		item->SetEnabled((folder->IsDone())?true:false);
    	 	else
    	 		item->SetEnabled(false);
    	 }
    	 theMenu->AddItem(item);
    	 
    	 
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
				this->Window()->PostMessage(aMessage);
	 	} 
	 	delete theMenu;
	 }else
	 	ColumnListView::MouseDown(point);
}


/***********************************************************
 * NodeMonitor
 ***********************************************************/
void
HFolderList::NodeMonitor(BMessage *message)
{
	int32 opcode;
	entry_ref ref,dir_ref;
	const char *name;
	BPath query_folder;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&query_folder);
	query_folder.Append(APP_NAME);
	query_folder.Append(QUERY_FOLDER);
	
	if(message->FindInt32("opcode",&opcode) == B_OK)
	{
		switch(opcode)
		{
		case B_ENTRY_MOVED:
		{
			node_ref nref;
			node_ref from_nref;
			BPath to_path,from_path;
			BDirectory to_dir,from_dir;
			
			message->FindInt32("device", &nref.device);
			message->FindInt32("device", &from_nref.device);
			message->FindInt64("to directory", &nref.node);
			message->FindInt64("from directory", &from_nref.node);
			to_dir.SetTo(&nref);
			from_dir.SetTo(&from_nref);
			
			to_path.SetTo(&to_dir,NULL,false);
			from_path.SetTo(&from_dir,NULL,false);
			
			PRINT(("To:%s\n",to_path.Path()));
			PRINT(("From:%s\n",from_path.Path()));
			message->FindString("name", &name);
			if( ::strcmp(to_path.Path(),query_folder.Path()) == 0)
			{
				::get_ref_for_path(to_path.Path(),&dir_ref);
	
				to_path.Append(name);
				entry_ref ref;
				::get_ref_for_path(to_path.Path(),&ref);
				AddQuery(ref);
			}else if(::strcmp(from_path.Path(),query_folder.Path()) == 0){
				to_path.Append(name);
				entry_ref ref;
				::get_ref_for_path(to_path.Path(),&ref);
				RemoveQuery(ref);
			}
			break;
		}
		
		case B_ENTRY_CREATED:
		{
			PRINT(("CREATE\n"));
			
			message->FindInt32("device", &ref.device);
			message->FindInt64("directory", &ref.directory);
			message->FindString("name", &name);
			ref.set_name(name);
			AddQuery(ref);
			break;
		}
		case B_ENTRY_REMOVED:
		{
			PRINT(("REMOVED\n"));
			
			message->FindInt32("device", &ref.device);
			message->FindInt64("directory", &ref.directory);
			message->FindString("name", &name);
			ref.set_name(name);
			
			BPath path(&ref);
			if(::strcmp(path.Path(),query_folder.Path()) == 0)
			{
				RemoveQuery(ref);
			}
			break;
		}
		}
	}
}


/***********************************************************
 * AddQuery
 ***********************************************************/
status_t
HFolderList::AddQuery(entry_ref ref)
{
	BPath path(&ref);
	BNode node(&ref);
	BEntry entry(&ref);
	
	if(entry.IsSymLink())
	{
		BSymLink link(&entry);
		
		char buf[B_PATH_NAME_LENGTH+1];
		ssize_t len = link.ReadLink(buf,B_PATH_NAME_LENGTH);
		buf[len] = '\0';
		path.SetTo(buf);
		if(path.InitCheck() != B_OK)
			return B_ERROR;
		::get_ref_for_path(path.Path(),&ref);
		if(node.SetTo(&ref) != B_OK)
			return B_ERROR;
	}	
	BString type;
	node.ReadAttrString("BEOS:TYPE",&type);
	if(type.Compare("application/x-vnd.Be-query") != 0)
		return B_ERROR;
	AddItem(new HQueryItem(ref,this));	
	return B_OK;
}

/***********************************************************
 * RemoveQuery
 ***********************************************************/
status_t
HFolderList::RemoveQuery(entry_ref ref)
{
	BEntry entry(&ref);
	char name[B_FILE_NAME_LENGTH+1];
	entry.GetName(name);
				
	int32 fol = FindQuery(name);
	if(fol >= 0)
	{
		HQueryItem *item = cast_as(RemoveItem(fol),HQueryItem);
		delete item;
	}else
		return B_ERROR;
	
	return B_OK;
}
#include "HFolderList.h"
#include "CLVColumn.h"
#include "HFolderItem.h"
#include "HQueryItem.h"
#include "ResourceUtils.h"
#include "HMailList.h"
#include "HApp.h"
#include "HPrefs.h"
#include "HIMAP4Folder.h"
#include "HIMAP4Window.h"
#include "RectUtils.h"
#include "HWindow.h"
#include "Utilities.h"

#include <Window.h>
#include <StorageKit.h>
#include <Autolock.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Debug.h>
#include <Path.h>
#include <ClassInfo.h>
#include <NodeMonitor.h>
#include <E-mail.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HFolderList::HFolderList(BRect frame,
						BetterScrollView **scroll,
						const char* title)
	:_inherited(frame,
					(CLVContainerView**)scroll,
					title,
					B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM,
					B_WILL_DRAW|B_PULSE_NEEDED|B_FRAME_EVENTS,
					B_SINGLE_SELECTION_LIST,
					true   )
	,fCancel(false)
	,fThread(-1)
	,fWatching(false)
	,fSkipGathering(false)
	,fSkipMoveMail(false)
{
	CLVColumn *expander_col;
	this->AddColumn( expander_col = new CLVColumn(NULL,0.0,CLV_EXPANDER|CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE));
	//expander_col->SetShown(false);
	this->AddColumn(new CLVColumn(NULL,20,CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE|
		CLV_NOT_RESIZABLE|CLV_PUSH_PASS|CLV_MERGE_WITH_RIGHT));
	this->AddColumn(new CLVColumn(_("Folders"),400,CLV_SORT_KEYABLE|CLV_NOT_MOVABLE|CLV_PUSH_PASS));
	SetViewColor(tint_color( ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));
	SetInvocationMessage(new BMessage(M_OPEN_FOLDER));
	SetSortFunction(HFolderItem::CompareItems);
	SetHighlightTextOnly(true);
	SetSortKey(2);
	
	fLocalFolders = new HSimpleFolderItem(_("Local Folders"),this);
	fPointerList.AddItem(fLocalFolders);
	fIMAP4Folders = new HSimpleFolderItem(_("IMAP4 Folders"),this);
	fPointerList.AddItem(fIMAP4Folders);
	fQueryFolders = new HSimpleFolderItem(_("Queries"),this);
	fPointerList.AddItem(fQueryFolders);
	
	AddItem(fLocalFolders);
	fLocalFolders->Added(true);
	rgb_color selection_col = {184,194, 255,255};
	SetItemSelectColor(true, selection_col);
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
	case M_GATHER_ALL_MAILS:
	{
		bool gather;
		((HApp*)be_app)->Prefs()->GetData("load_list_on_start_up",&gather);
		if(!gather)
			break;
		int32 count = fPointerList.CountItems();
		for(int32 i = 0;i < count;i++)
		{
			HFolderItem *item = (HFolderItem*)fPointerList.ItemAt(i);
			if(item && item->FolderType() == FOLDER_TYPE)
				item->StartGathering();		
		}
		break;
	}
	case M_ADD_IMAP4_FOLDER:
	{
		RectUtils utils;
		BRect rect = utils.CenterRect(230,200);
		HIMAP4Window *window = new HIMAP4Window(rect,this);
		window->Show();
		break;
	}
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
		
		
		bool expand = false;
		
		for(int32 i = 0;i < count;i++)
		{
			message->FindPointer("item",i,(void**)&item);
			message->FindPointer("parent",i,(void**)&parent);
			if(message->FindBool("expand",i,&expand) != B_OK)
				expand = false;
			
			
			if(!parent->IsSuperItem())
			{
				parent->SetSuperItem(true);
				parent->SetExpanded(expand);
			}
			AddUnder(item,parent);
			
			fPointerList.AddItem(item);
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
		int32 item_type;
		((HApp*)be_app)->Prefs()->GetData("load_list_on_start_up",&gather);
		
		for(int32 i = 0;i < count;i++)
		{
			if(message->FindPointer("item",i,(void**)&item) ==B_OK)
			{
				item_type = item->FolderType();
				switch(item_type)
				{
				case FOLDER_TYPE:
					AddUnder(item,fLocalFolders);
					break;
				case IMAP4_TYPE:
					if(!fIMAP4Folders->IsAdded())
					{
						AddItem(fIMAP4Folders);
						fIMAP4Folders->Added(true);
					}
					AddUnder(item,fIMAP4Folders);
					break;
				case QUERY_TYPE:
					if(!fQueryFolders->IsAdded())
					{
						AddItem(fQueryFolders);
						fQueryFolders->Added(true);
					}
					AddUnder(item,fQueryFolders);
					break;
				default:
					AddUnder(item,fLocalFolders);
				}
				fPointerList.AddItem(item);
			}
		}
		SortItems();
		fPointerList.AddList(&list);
		// Check inbox
		bool check_inbox;
		((HApp*)be_app)->Prefs()->GetData("check_inbox",&check_inbox);
		if(check_inbox)
			Select(1);
		
		break;
	}
	
	case M_INVALIDATE:
	{
		int32 i;
		if(message->FindInt32("index",&i) == B_OK)
		{
			InvalidateItem(i);
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
			_inherited::MessageReceived(message);
	}
}

/***********************************************************
 * FindFolder
 ***********************************************************/
int32
HFolderList::FindFolder(entry_ref ref)
{
	int32 count = CountItems();
	HFolderItem **items = (HFolderItem**)Items();
	for(int32 i = 0;i < count;i++)
	{
		HFolderItem *item = cast_as(items[i],HFolderItem);
		if(item && item->FolderType() == FOLDER_TYPE)
		{
			if( item->Ref() == ref)
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
	HFolderItem **items = (HFolderItem**)fPointerList.Items();
	for(int32 i = 0;i < count;i++)
	{
		HFolderItem *item = items[i];
		if(item && item->FolderType() == QUERY_TYPE)
		{
			if(strcmp(item->FolderName() , name) == 0)
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
	char name[B_FILE_NAME_LENGTH];
	//
   	entry.GetRef(&ref);
	char buf[4096];
	dirent *dent;
	int32 count;
	int32 offset;
	
	BMessage msg(M_ADD_FOLDER);
	msg.MakeEmpty();
	BMessage childMsg(M_ADD_UNDER_ITEM);
	childMsg.MakeEmpty();
	
	bool tree_mode;
	((HApp*)be_app)->Prefs()->GetData("tree_mode",&tree_mode);
	
	BList folderList;
#ifndef USE_SCANDIR	
	while( (count = dir.GetNextDirents((dirent *)buf, 4096)) > 0 && !list->fCancel )
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
			
			if(entry.SetTo(&ref) != B_OK)
				continue;
			char link_path[B_PATH_NAME_LENGTH];
			BSymLink link(path.Path());
			if(B_BAD_VALUE == link.ReadLink(link_path,B_PATH_NAME_LENGTH)&& entry.IsDirectory())
			{
				entry.GetRef(&ref);
				item = new HFolderItem(ref,list);
				msg.AddPointer("item",item);
				folderList.AddItem(item);
				if(tree_mode)
					GetChildFolders(entry,item,list,childMsg);
			}
		}
	}
#else
	int32 i=0;
	struct dirent **dirents = NULL;
	
	count = GetAllDirents(path.Path(),&dirents);
	while(count>i && !list->fCancel )
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
		if(entry.SetTo(&ref) != B_OK)
			continue;
		char link_path[B_PATH_NAME_LENGTH];
		BSymLink link(path.Path());
		if(B_BAD_VALUE == link.ReadLink(link_path,B_PATH_NAME_LENGTH)&& entry.IsDirectory())
		{
			entry.GetRef(&ref);
			item = new HFolderItem(ref,list);
			msg.AddPointer("item",item);
			folderList.AddItem(item);
			if(tree_mode)
				GetChildFolders(entry,item,list,childMsg);
		}
	}
	free(dirents);
#endif
	/********* QUERY ***********/
	err = B_OK;
	find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append( APP_NAME );
	path.Append( QUERY_FOLDER );
	if(dir.SetTo(path.Path()) != B_OK)
		dir.CreateDirectory(path.Path(),&dir);
	HQueryItem *query(NULL);
	BNode node;
	
	char type[B_MIME_TYPE_LENGTH+1];
	while( (count = dir.GetNextDirents((dirent *)buf, 4096)) > 0 && !list->fCancel )
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
			if( node.SetTo(&ref) != B_OK)
				continue;
			// Read SymLink
			if(node.IsSymLink())
			{
				BSymLink link(&ref);
				entry_ref new_ref;
				char pathBuf[B_PATH_NAME_LENGTH+1];
				ssize_t len = link.ReadLink(pathBuf,B_PATH_NAME_LENGTH);
				pathBuf[len] = '\0';
			
				::get_ref_for_path(pathBuf,&new_ref);
				if(node.SetTo(&new_ref) != B_OK)
					continue;
			}
			
			node.ReadAttr("BEOS:TYPE",B_STRING_TYPE,0,type,B_MIME_TYPE_LENGTH);
			if(::strcmp(type,"application/x-vnd.Be-query") != 0)
				continue;
			
			query = new HQueryItem(ref,list);
			msg.AddPointer("item",query);
		}
	}
	// Gather IMAP4 folders
	err = B_OK;
	find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append( APP_NAME );
	path.Append("Accounts");
	path.Append("IMAP4");
	if(dir.SetTo(path.Path()) != B_OK)
		dir.CreateDirectory(path.Path(),&dir);
	
	BMessage setting;
	BFile file;
	const char* folder;
	const char* addr;
	const char* login;
	const char* pass;
	int16 port;
		
	while( err == B_OK && !list->fCancel )
	{
		err = dir.GetNextEntry(&entry,false);	
		if( entry.InitCheck() != B_NO_ERROR )
			break;
		if( entry.GetPath(&path) != B_NO_ERROR )
			break;
		else{
			BFile file(path.Path(),B_READ_ONLY);
			if(file.InitCheck() != B_OK)
				continue;
			entry.GetName(name);
			setting.Unflatten(&file);
			setting.FindString("server",&addr);
			setting.FindString("login",&login);
			setting.FindString("password",&pass);
			BString password;
			int32 len = strlen(pass);
			for(int32 i =0;i < len;i++)
				password += (char)255-pass[i];
			setting.FindString("folder",&folder);
			setting.FindInt16("port",&port);
			msg.AddPointer("item",new HIMAP4Folder(name,folder,addr,port,login,password.String(),list));
		}
	}	
	
	// Send add list items message
	if(!msg.IsEmpty())
		list->Window()->PostMessage(&msg,list);

	if(!childMsg.IsEmpty())
		list->Window()->PostMessage(&childMsg,list);
	
	list->fThread =  -1;
	list->Window()->PostMessage(M_GATHER_ALL_MAILS,list);
	return 0;
}

/***********************************************************
 * GetChildFolders
 ***********************************************************/
void
HFolderList::GetChildFolders(const BEntry &inEntry,
							HFolderItem *parentItem,
							HFolderList *list,
							BMessage &childMsg)
{
#ifndef USE_SCANDIR
	BDirectory dir(&inEntry);
	BEntry entry;
	entry_ref ref;
	HFolderItem *item(NULL);

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
			
			if(entry.SetTo(&ref) != B_OK)
				continue;
			if(entry.IsDirectory())
			{
				entry.GetRef(&ref);
				childMsg.AddPointer("item",(item = new HFolderItem(ref,list)));
				childMsg.AddPointer("parent",parentItem);
				parentItem->IncreaseChildItemCount();
				GetChildFolders(entry,item,list,childMsg);			
			}
		} 
	}
#else
	BDirectory dir(&inEntry);
	HFolderItem *item(NULL);
	
	int32 count,i=0;
	struct dirent **dirents = NULL;
	entry_ref ref;
	BEntry entry;
	BPath path;
	inEntry.GetPath(&path);
	
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
		if(entry.SetTo(&ref) != B_OK)
			continue;
		if(entry.IsDirectory())
		{
			entry.GetRef(&ref);
			childMsg.AddPointer("item",(item = new HFolderItem(ref,list)));
			childMsg.AddPointer("parent",parentItem);	
			parentItem->IncreaseChildItemCount();
			GetChildFolders(entry,item,list,childMsg);			
		} 
	}
	free(dirents);
#endif 
}

/***********************************************************
 * DeleteAll
 ***********************************************************/
void
HFolderList::DeleteAll()
{
	int32 count = fPointerList.CountItems();
	MakeEmpty();
	
	while(count>0)
	{
		HFolderItem *item = static_cast<HFolderItem*>(fPointerList.RemoveItem(--count));
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
			Window()->PostMessage(M_LOCAL_SELECTION_CHANGED);
			return;
		}		
		fWatching = false;
		BList *list = theItem->MailList();
		InvalidateItem(sel);
		BMessage msg(M_LOCAL_SELECTION_CHANGED);
		msg.AddPointer("pointer",list);
		entry_ref ref = theItem->Ref();
		msg.AddRef("refs",&ref);
		GetFolderPath(theItem,msg);
		msg.AddInt32("folder_type",theItem->FolderType());
		Window()->PostMessage(&msg);
	}else{
		Window()->PostMessage(M_LOCAL_SELECTION_CHANGED);
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
	_inherited::MouseMoved(point,transit,message);
}

/***********************************************************
 * SelectWithoutGathering
 ***********************************************************/
void
HFolderList::SelectWithoutGathering(int32 index)
{
	fSkipGathering = true;
	HFolderItem *item = cast_as(ItemAt(index),HFolderItem);
	int32 type = item->FolderType();
	if(type == FOLDER_TYPE)
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
			if(item->FolderType() == QUERY_TYPE)
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
	
    // Right click
    if(buttons == B_SECONDARY_MOUSE_BUTTON)
    {
    	 int32 sel = CurrentSelection();
    	 BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
    	 BFont font(be_plain_font);
    	 font.SetSize(10);
    	 theMenu->SetFont(&font);
    	 
    	
    	 BMenuItem *item = new BMenuItem(_("Recreate Cache"),new BMessage(M_REFRESH_CACHE),0,0);
    	 
    	 if(sel < 0)
    	 	item->SetEnabled(false);
    	 else{
    	 	HFolderItem *folder= cast_as(ItemAt(sel),HFolderItem);
    	 	if(folder)
    	 	{
    	 		if(folder->FolderType() != SIMPLE_TYPE)
    	 			item->SetEnabled((folder->IsDone())?true:false);
    	 		else
    	 			item->SetEnabled(false);
    	 	}else
    	 		item->SetEnabled(false);
    	 }
    	 theMenu->AddItem(item);
    	 theMenu->AddSeparatorItem();
    	 item = new BMenuItem(_("Add IMAP4 Folders"),new BMessage(M_ADD_IMAP4_FOLDER),0,0);
    	 item->SetEnabled((sel<0)?true:false);
    	 theMenu->AddItem(item);
    	 item = new BMenuItem(_("IMAP4 Folder Properties"),new BMessage(M_OPEN_FOLDER),0,0);
    	 if(sel < 0)
    	 {
    	 	item->SetEnabled(false);
    	 }else{
    	 	HFolderItem *folder = cast_as(ItemAt(sel),HFolderItem);
    	 	item->SetEnabled((folder->FolderType() == IMAP4_TYPE)?true:false);
    	 }
    	 theMenu->AddItem(item);
    	 theMenu->AddSeparatorItem();
    	 item = new BMenuItem(_("Delete Folders"),new BMessage(M_DELETE_FOLDER),0,0);
    	 item->SetEnabled((sel<0)?false:true);
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
			{
				if(aMessage->what == M_ADD_IMAP4_FOLDER)
					Window()->PostMessage(aMessage,this);
				else
					Window()->PostMessage(aMessage);
	 		}
	 	} 
	 	delete theMenu;
	 }else
	 	_inherited::MouseDown(point);
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
			ino_t node;
			message->FindInt32("device", &nref.device);
			message->FindInt32("device", &from_nref.device);
			message->FindInt64("node", &node);
			message->FindInt64("to directory", &nref.node);
			message->FindInt64("from directory", &from_nref.node);
			to_dir.SetTo(&nref);
			from_dir.SetTo(&from_nref);
			
			to_path.SetTo(&to_dir,NULL,false);
			from_path.SetTo(&from_dir,NULL,false);
			
			//PRINT(("To:%s\n",to_path.Path()));
			//PRINT(("From:%s\n",from_path.Path()));
			message->FindString("name", &name);
			if( ::strcmp(to_path.Path(),query_folder.Path()) == 0)
			{
				::get_ref_for_path(to_path.Path(),&dir_ref);
	
				to_path.Append(name);
				entry_ref ref;
				::get_ref_for_path(to_path.Path(),&ref);
				AddQuery(ref);
			}else if(::strcmp(from_path.Path(),query_folder.Path()) == 0){
				node_ref old_nref;
				old_nref.device = from_nref.device;
				old_nref.node = node;
				PRINT(("%ld\n",node));
				RemoveQuery(old_nref);
			}
			break;
		}
		
		case B_ENTRY_CREATED:
		{
			message->FindInt32("device", &ref.device);
			message->FindInt64("directory", &ref.directory);
			message->FindString("name", &name);
			ref.set_name(name);
			ino_t node;
			message->FindInt64("node",&node);
			AddQuery(ref);		
			break;
		}
		case B_ENTRY_REMOVED:
		{
			PRINT(("REMOVED\n"));
			node_ref nref;
			message->FindInt32("device", &nref.device);
			message->FindInt64("node", &nref.node);
			
			RemoveQuery(nref);
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
		entry_ref new_ref;
		
		char buf[B_PATH_NAME_LENGTH+1];
		ssize_t len = link.ReadLink(buf,B_PATH_NAME_LENGTH);
		buf[len] = '\0';
		path.SetTo(buf);
		if(path.InitCheck() != B_OK)
			return B_ERROR;
		::get_ref_for_path(path.Path(),&new_ref);
		if(node.SetTo(&new_ref) != B_OK)
			return B_ERROR;
	}	
	BString type;
	node.ReadAttrString("BEOS:TYPE",&type);
	if(type.Compare("application/x-vnd.Be-query") != 0)
		return B_ERROR;
	
	HQueryItem *item = new HQueryItem(ref,this);
	BMessage msg(M_ADD_FOLDER);
	msg.AddPointer("item",item);
	Window()->PostMessage(&msg,this);	
	return B_OK;
}

/***********************************************************
 * RemoveQuery
 ***********************************************************/
status_t
HFolderList::RemoveQuery(node_ref& in_nref)
{
	BListItem **items = static_cast<BListItem**>(fPointerList.Items());
	int32 count = fPointerList.CountItems();
	node_ref nref;
	BEntry entry;
	
	for(int32 i = 0;i < count;i++)
	{
		HFolderItem *item = cast_as(items[i],HFolderItem);
		if(!item)
			continue;
		if(item->FolderType() != QUERY_TYPE)
			continue;
		nref = item->NodeRef();
		if(nref== in_nref)
		{
			RemoveItem(item);
			fPointerList.RemoveItem(item);
			delete item;
			return B_OK;
		}
	}
	return B_ERROR;
}

/***********************************************************
 * RemoveQuery
 ***********************************************************/
status_t
HFolderList::RemoveQuery(entry_ref& ref)
{
	HFolderItem **items = (HFolderItem**)fPointerList.Items();
	int32 count = fPointerList.CountItems();
	
	for(int32 i = 0;i < count;i++)
	{
		HQueryItem *item = cast_as(items[i],HQueryItem);
		if(item && item->FolderType() == QUERY_TYPE && item->Ref() == ref)
		{
			RemoveItem(item);
			fPointerList.RemoveItem(item);
			delete item;
			return B_OK;
		}
	}
	return B_ERROR;
}

/***********************************************************
 * ProcessMails
 ***********************************************************/
void
HFolderList::ProcessMails(BMessage *message)
{
	int32 opcode,folder_index;
	entry_ref ref,dir_ref;
	const char *name;
	
	BPath folder_path;
	BPath mail_folder;
	::find_directory(B_USER_DIRECTORY,&mail_folder);
	mail_folder.Append("mail");
	int32 mail_path_len = strlen(mail_folder.Path());
	char mime_type[B_MIME_TYPE_LENGTH];
	BNode node;
	BNodeInfo ninfo;
	HFolderItem *folder;

	if(message->FindInt32("opcode",&opcode) == B_OK)
	{
		switch(opcode)
		{
		case B_ENTRY_MOVED:
		{
			if(fSkipMoveMail)
			{
				fSkipMoveMail = false;
				break;
			}
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
			bool moved = false;
			// Add mail
			if(::strncmp(to_path.Path(),mail_folder.Path(),mail_path_len) == 0)
			{
				BPath file_path(to_path);
				file_path.Append(name);
				if(node.SetTo(file_path.Path()) == B_OK)
				{
					ninfo.SetTo(&node);
					ninfo.GetType(mime_type);
					if(::strcmp(mime_type,B_MAIL_TYPE) != 0)
						break;
					::get_ref_for_path(to_path.Path(),&dir_ref);
					folder_index = FindFolder(dir_ref);
					if(folder_index <0)
						break;
					folder = cast_as(ItemAt(folder_index),HFolderItem);
					if(folder)
					{
						::get_ref_for_path(file_path.Path(),&ref);
						HMailItem* item;
						folder->AddMail((item = new HMailItem(ref)));
						if(folder_index == CurrentSelection())
							AddToMailList(item);
						InvalidateItem(folder_index);
					}
				}	
				moved = true;	
			}
			// Remove mails
			if(::strncmp(from_path.Path(),mail_folder.Path(),mail_path_len) == 0)
			{
				BPath file_path(from_path);
				if(node.SetTo(file_path.Path()) == B_OK)
				{
					ninfo.SetTo(&node);
					::get_ref_for_path(from_path.Path(),&ref);
					folder_index = FindFolder(ref);
					if(folder_index <0)
						break;
					folder = cast_as(ItemAt(folder_index),HFolderItem);
					if(folder)
					{
						entry_ref old_ref(from_nref.device,from_nref.node,name);
						HMailItem *item = folder->RemoveMail(old_ref);
						if(folder_index == CurrentSelection())
							RemoveFromMailList(item,true);
						else
							delete item;
						InvalidateItem(folder_index);
					}
				}
			}
			// Move message will call 2 times; from "From dir" and "To dir"
			// if move mails between mail folders.
			// So skip next move message
			if(::strncmp(to_path.Path(),mail_folder.Path(),mail_path_len) == 0
				&&::strncmp(from_path.Path(),mail_folder.Path(),mail_path_len) == 0
				&& ::strcmp(from_path.Path(),to_path.Path()) != 0)
				fSkipMoveMail = true;
			
			break;
		}
		case B_ENTRY_CREATED:
		{
			message->FindInt32("device", &ref.device);
			message->FindInt64("directory", &ref.directory);
			message->FindString("name", &name);
			ref.set_name(name);
			if(node.SetTo(&ref) == B_OK)
			{
				ninfo.SetTo(&node);
				ninfo.GetType(mime_type);
				if(::strcmp(mime_type,B_MAIL_TYPE) != 0)
					break;
				BPath path(&ref);
				path.GetParent(&path);
				::get_ref_for_path(path.Path(),&dir_ref);
				folder_index = FindFolder(dir_ref);
				if(folder_index <0)
					break;
				folder = cast_as(ItemAt(folder_index),HFolderItem);
				if(folder)
				{
					HMailItem *item;
					folder->AddMail((item = new HMailItem(ref)));
					if(folder_index == CurrentSelection())
						AddToMailList(item);
					InvalidateItem(folder_index);
				}
			}
			break;
		}
		case B_ENTRY_REMOVED:
		{
			node_ref nref,dir_nref;
			message->FindInt32("device", &nref.device);
			message->FindInt64("node", &nref.node);
			message->FindInt64("directory", &dir_nref.node);
			dir_nref.device = nref.device;

			BDirectory dir(&dir_nref);
			BEntry entry;
			dir.GetEntry(&entry);
			entry.GetRef(&dir_ref);
			folder_index = FindFolder(dir_ref);
			if(folder_index <0)
				break;
			folder = cast_as(ItemAt(folder_index),HFolderItem);
			if(folder)
			{
				HMailItem *item = folder->RemoveMail(nref);
				if(folder_index == CurrentSelection())
					RemoveFromMailList(item,true);
				else
					delete item;	
				InvalidateItem(folder_index);
			}
			break;
		}
		} // end switch
	}
}

/***********************************************************
 * RemoveFolder
 ***********************************************************/
HFolderItem*
HFolderList::RemoveFolder(int32 index)
{
	HFolderItem *item = cast_as(RemoveItem(index),HFolderItem);	
	fPointerList.RemoveItem(item);
	switch(item->FolderType())
	{
	case QUERY_TYPE:
		if(!FullListHasItem(fQueryFolders))
			RemoveItem(fQueryFolders);
		break;
	case IMAP4_TYPE:
		if(!FullListHasItem(fIMAP4Folders))
			RemoveItem(fIMAP4Folders);
		break;
	}
	return item;
}

/***********************************************************
 * AddToMailList
 ***********************************************************/
void
HFolderList::AddToMailList(HMailItem *item)
{
	BMessage msg(M_ADD_MAIL_TO_LIST);
	msg.AddPointer("mail",item);
	Window()->PostMessage(&msg);
}

/***********************************************************
 * RemoveFromMailList
 ***********************************************************/
void
HFolderList::RemoveFromMailList(HMailItem *item,bool free)
{
	BMessage msg(M_REMOVE_MAIL_FROM_LIST);
	msg.AddPointer("mail",item);
	msg.AddBool("free",free);
	Window()->PostMessage(&msg);
}

/***********************************************************
 * GenarateFolderPathes
 ***********************************************************/
int32
HFolderList::GenarateFolderPathes(BMessage &msg)
{
	int32 count = FullListCountItems();
	int32 result_count = 0;
	for(int32 i = 0; i < count;i++)
	{
		HFolderItem *item = cast_as(FullListItemAt(i),HFolderItem);
		
		if(strcmp(item->FolderName(),_("Local Folders")) == 0)
			continue;
		if(strcmp(item->FolderName(),_("IMAP4 Folders")) == 0 ||
			strcmp(item->FolderName(),_("Queries")) == 0 )
			break;
		
		GetFolderPath(item,msg);
		result_count++;
	}
	return result_count;
}

/***********************************************************
 * GetFolderPath
 ***********************************************************/
void
HFolderList::GetFolderPath(HFolderItem *item,BMessage &msg)
{
	BString path("");
	
	path += item->FolderName();
	HFolderItem *parent = cast_as(Superitem(item),HFolderItem);
	
	while(parent)
	{
		if(strcmp(parent->FolderName(),_("Local Folders")) == 0 ||
			strcmp(parent->FolderName(),_("IMAP4 Folders")) == 0 ||
			strcmp(parent->FolderName(),_("Queries")) == 0)
			break;
		path.Insert("/",0);
		path.Insert(parent->FolderName(),0);
		parent = cast_as(Superitem(parent),HFolderItem);
	}
	msg.AddString("path",path.String() );
	return; 
}
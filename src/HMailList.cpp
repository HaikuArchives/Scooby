#include "HMailList.h"
#include "HFolderList.h"
#include "ResourceUtils.h"
#include "MenuUtils.h"
#include "HApp.h"
#include "HPrefs.h"
#include "CLVColumn.h"
#include "HWindow.h"
#include "IconMenuItem.h"
#include "OpenWithMenu.h"
#include "HIMAP4Item.h"

#include <StorageKit.h>
#include <Window.h>
#include <Autolock.h>
#include <Debug.h>
#include <Message.h>
#include <ClassInfo.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <E-mail.h>

/***********************************************************
 * Scripting properties
 ***********************************************************/
#define kScriptingSuites "suite/vnd.x-vnd.takamatsu-scooby-maillist"
const uint32 kNextSpecifier = 'snxt';
const uint32 kPreviousSpecifier = 'sprv';

const property_info kMailPropertyList[] = {
	{	"Entry",
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER, B_INDEX_SPECIFIER, kPreviousSpecifier, kNextSpecifier },
		"get Entry [next|previous|index] # returns specified entries",
		0,
		{ B_REF_TYPE },
		{},
		{}
	},
	{	NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};

/***********************************************************
 * Constructor
 ***********************************************************/
HMailList::HMailList(BRect frame,
					BetterScrollView **scroll,
					const char* title)
	:_inherited(frame,
					(CLVContainerView**)scroll,
					title,
					B_FOLLOW_ALL,
					B_WILL_DRAW|B_FRAME_EVENTS,
					B_MULTIPLE_SELECTION_LIST)
	,fCurrentFolder(NULL)
	,fScrollView(*scroll)
	,fFolderType(FOLDER_TYPE)
	,fOldSelection(NULL)
{
	AddColumn( new CLVColumn(NULL,22,CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE|
		CLV_NOT_RESIZABLE|CLV_PUSH_PASS|CLV_SORT_KEYABLE) );
	SetSortFunction(HMailItem::CompareItems);
	
	const char* label[] = {_("Subject"),_("From"),_("To"),_("When"),_("P"),_("A")};
	CLVColumn*			column;
	for(int32 i = 0;i < 6;i++)
	{
		if(i>=4)
			column = new CLVColumn(label[i],20,CLV_SORT_KEYABLE|CLV_NOT_RESIZABLE);
		else
			column = new CLVColumn(label[i],100,CLV_SORT_KEYABLE|CLV_TELL_ITEMS_WIDTH);
		AddColumn(column);
	}
	SetInvocationMessage(new BMessage(M_INVOKE_MAIL));
	
	int32 display_order[7] = {0,5,6,1,2,3,4};
	SetDisplayOrder(display_order);
	int32 sort_key = 4,sort_mode = 1;
	SetSortMode(sort_key,(CLVSortMode)sort_mode);
	SetSortKey(sort_key);
	rgb_color selection_col = {184,194, 255,255};
	SetItemSelectColor(true, selection_col);
}

/***********************************************************
 * Desturctor
 ***********************************************************/
HMailList::~HMailList()
{
	int32 sel = CurrentSelection();
	if(sel >= 0)
	{
		fOldSelection = cast_as(ItemAt(sel),HMailItem);
		MarkOldSelectionAsRead();
	}
	SetInvocationMessage(NULL);
	delete fCurrentFolder;
}

/***********************************************************
 * IsColumnShown
 ***********************************************************/
bool
HMailList::IsColumnShown(ColumnType type)
{
	CLVColumn *col = ColumnAt((int32)type+1);
	return col->IsShown();
}

/***********************************************************
 * IsColumnShown
 ***********************************************************/
void
HMailList::SetColumnShown(ColumnType type,bool shown)
{
	CLVColumn *col = ColumnAt((int32)type+1);
	col->SetShown(shown);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HMailList::MessageReceived(BMessage *message)
{
	if(HandleScriptingMessage(message))
		return;
	
	switch(message->what)
	{
	// Select next mail
	case M_SELECT_NEXT_MAIL:
	{
		int32 sel = CurrentSelection();
		if(sel<0)
			break;
		Select(sel+1);
		ScrollToSelection();
		break;
	}
	// Select prev mail
	case M_SELECT_PREV_MAIL:
	{
		int32 sel = CurrentSelection();
		if(sel<0)
			break;
		Select(sel-1);
		ScrollToSelection();
		break;
	}
	case M_LOCAL_SELECTION_CHANGED:
	{
		BList *list;
		MarkOldSelectionAsRead();
		// Save Columns
		SaveColumnsAndPos();
		MakeEmpty();
		delete fCurrentFolder;
		fCurrentFolder=NULL;
		if(message->FindPointer("pointer",(void**)&list) == B_OK)
		{
			// Refresh Columns
			message->FindInt32("folder_type",&fFolderType);
			entry_ref ref;
			BMessage settings;
			const char* path;
			
			//PRINT(("%s\n",path));
			if(message->FindString("path",&path) != B_OK)
				RefreshColumns(NULL);
			else
			{
				if (ReadSettings(path, &settings) != B_OK) RefreshColumns(NULL);
				else RefreshColumns(&settings);
			}
			//Add Mails
			AddList(list);
			SortItems();
			//refresh scroll position
			RefreshScrollPos(&settings);
		}
		break;
	}
	// To avoid endless loop
	case B_SELECT_ALL:
		Select(0,CountItems()-1);
		break;
	default:
		_inherited::MessageReceived(message);
	}
}


/***********************************************************
 * SelectionChanged
 ***********************************************************/
void
HMailList::SelectionChanged()
{
	int32 sel = this->CurrentSelection();
	MarkOldSelectionAsRead();
	
	if(sel <0 || HasSomeSelection())
	{
		// set content view empty
		Window()->PostMessage(M_SET_CONTENT);
		return;
	}
	
	HMailItem *item = cast_as(ItemAt(sel),HMailItem);
	if(item)
	{
		BMessage msg(M_SET_CONTENT);
		entry_ref ref = item->Ref();
		msg.AddRef("refs",&ref);
		msg.AddString("subject",item->fSubject);
		msg.AddString("from",item->fFrom);
		msg.AddString("when",item->fDate);
		msg.AddBool("read",item->IsRead());
		msg.AddString("cc",item->fCC);
		msg.AddString("to",item->fTo);
		fOldSelection = item;
		Window()->PostMessage(&msg);
		item->RefreshEnclosureAttr();		
		item->RefreshStatus();
		InvalidateItem(sel);
	}
}

/***********************************************************
 * KeyDown
 ***********************************************************/
void 	
HMailList::KeyDown(const char *bytes, int32 numBytes) 
{
	if(bytes[0] == B_SPACE)
		return;
	else if(bytes[0] == B_DELETE&&numBytes == 1)
		Window()->PostMessage(M_DELETE_MSG);		
	_inherited::KeyDown(bytes,numBytes);
}

/***********************************************************
 * SaveColumns
 ***********************************************************/
void
HMailList::SaveColumnsAndPos()
{
	if(!fCurrentFolder)
		return;
	/*
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Attribute");
	::create_directory(path.Path(),0777);
	if(fFolderType == QUERY_TYPE)
	{
		path.Append("Query");
		::create_directory(path.Path(),0777);
	}else if(fFolderType == IMAP4_TYPE)
	{
		path.Append("IMAP4");
		::create_directory(path.Path(),0777);
	}else{
		const char* p;
		BPath folder_path(fCurrentFolder);
		folder_path.GetParent(&folder_path);
		while(1)
		{
			p = folder_path.Leaf();
			if(::strcmp(p,"mail") == 0)
				break;	
			path.Append(p);
			::create_directory(path.Path(),0777);
			folder_path.GetParent(&folder_path);
		}
	}
	
	BString name( BPath(fCurrentFolder).Leaf());
	name << ".attr";
	
	path.Append(name.String());
	*/
	BFile file(fCurrentFolder,B_WRITE_ONLY|B_CREATE_FILE);
	if(file.InitCheck() != B_OK)
	{
		PRINT(("Fail to write attr setting\n"));
		return;
	}
	const int32 kFlags[] = {COLUMN_SUBJECT,COLUMN_FROM,COLUMN_TO
							,COLUMN_WHEN,COLUMN_PRIORITY,COLUMN_ATTACHMENT};
	
	BMessage msg;
	int32 flags = 0;
	char width_name[] = "width1";
	float width;
	for(int32 i = 1;i < 7;i++)
	{
		CLVColumn *col = ColumnAt(i);
		
		width = col->Width();
		msg.AddFloat(width_name,width);
		width_name[5]++;
		if(col->IsShown())
			flags |= kFlags[i-1];
	}
	msg.AddInt32("flags",flags);
	
	int32 mode = 0,sort_key = -1;
	
	int32 sortKeys[7];
	CLVSortMode sortModes[7];
	int32 num = GetSorting(sortKeys,sortModes);
	
	for(int32 i = 0;i < num;i++)
	{
		if(sortModes[i] != NoSort)
		{
			sort_key = sortKeys[i];
			mode = sortModes[i];
		}
	}
	msg.AddInt32("sort_key",sort_key);
	msg.AddInt32("sort_mode",mode);
	
	// Save Display order
	int32 display_order[7];
	GetDisplayOrder(display_order);
	char column_name[] = "column1";
	for(int32 i = 0;i < 7;i++)
	{
		msg.AddInt32(column_name,display_order[i]);
		column_name[6]++;
	}
	
	msg.AddPoint("scroll_pos", Bounds().LeftTop());
	
	msg.Flatten(&file);
	entry_ref ref;
	fCurrentFolder->GetRef(&ref);
	BString folder_path(BPath(&ref).Path());
	file.WriteAttrString("FolderPath",&folder_path);
}

/***********************************************************
 * AddMail
 ***********************************************************/
void
HMailList::AddMail(HMailItem *mail)
{
	// Find sortkey
	int32 sort_key = -1;
	for(int32 i = 1;i < 6;i++)
	{
		CLVColumn *col = ColumnAt(i);
		
		if(col->SortMode() != NoSort )
		{
			sort_key = i;
			break;
		}
	}
	// no sort key
	if(sort_key < 0)
	{
		AddItem(mail);
		return;
	}
	// Get sort mode
	CLVSortMode sort_mode = ColumnAt(sort_key)->SortMode();
	int32 insert_pos;
	int32 count = CountItems();
	insert_pos = CalcInsertPosition(count,sort_key,sort_mode,mail);
	AddItem(mail,insert_pos);
}

/***********************************************************
 * CalcInsertPosition
 ***********************************************************/
int32
HMailList::CalcInsertPosition(int32 count,
							int32 sort_key,
							int32 sort_mode,
							HMailItem *mail)
{
	if(sort_mode == NoSort)
		return 0;

	int32 key = 1;
	if(sort_mode == Ascending)	
		key = -1;
	else
		key = 1;
	int32 i;
	for(i = 0;i < count;i++)
	{
		HMailItem *item = cast_as(ItemAt(i),HMailItem);
		if(!item)
			continue;
		if( HMailItem::CompareItems((CLVListItem*)mail,
									(CLVListItem*)item,
									sort_key) == key)
		{
			return i;
		}
	}
	return i;
}
/***********************************************************
 * RemoveMails
 ***********************************************************/
void
HMailList::RemoveMails(BList *list)
{
	int32 count = list->CountItems();
	
	if(count<2)
	{
		for(int32 i = 0;i < count;i++)
			RemoveItem((BListItem*)list->ItemAt(i));
		return;
	}
	HMailItem *item = (HMailItem*)list->RemoveItem(count-1);
	
	BScrollBar *bar = this->ScrollBar(B_VERTICAL);
	bar->SetTarget((BView*)NULL);
	for(int32 i = 0;i < count-1;i++)
			RemoveItem((BListItem*)list->ItemAt(i));

	bar->SetTarget(this);
	RemoveItem(item);
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HMailList::MouseDown(BPoint pos)
{
	int32 buttons = 0; 
	ResourceUtils utils;
	BPoint point = pos;
	BMessage *msg;
    Window()->CurrentMessage()->FindInt32("buttons", &buttons); 
    MakeFocus(true);
	IconMenuItem *item;
    // Right click 
    if(buttons == B_SECONDARY_MOUSE_BUTTON)
    {
    	 // CurrentSelection func takes much time if too many items are selected
    	 int32 sel = CurrentSelection();
    	 /*int32 sel = -1;
    	 int32 n = 0;
    	 PRINT(("Enter\n"));
    	 while(1)
    	 {
    	 	if(!ItemAt(n))
    	 		break;
    	 	if(IsItemSelected(n++))
    	 	{
    	 		sel = n-1;
    	 		break;
    	 	}
    	 }
    	 PRINT(("OUT\n"));
    	 */
    	 BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
    	 BFont font(be_plain_font);
    	 font.SetSize(10);
    	 theMenu->SetFont(&font);
    	 
    	 item = new IconMenuItem(_("New Message"),new BMessage(M_NEW_MSG),'N',0,
							utils.GetBitmapResource('BBMP',"New Message"));
		 theMenu->AddItem(item);
		 theMenu->AddSeparatorItem();
	
    	 
    	 msg = new BMessage(M_REPLY_MESSAGE);
    	 msg->AddBool("reply_all",false);
    	 item = new IconMenuItem(_("Reply"),msg,'R',0,
    	 						utils.GetBitmapResource('BBMP',"Reply"));
    	 item->SetEnabled( (sel >= 0)?true:false);
    	 theMenu->AddItem(item);
    	 
     	 msg = new BMessage(M_REPLY_MESSAGE);
    	 msg->AddBool("reply_all",true);
    	 item = new IconMenuItem(_("Reply To All"),msg,'R',B_SHIFT_KEY,
    	 						utils.GetBitmapResource('BBMP',"Reply To All"));
    	 item->SetEnabled( (sel >= 0)?true:false);
    	 theMenu->AddItem(item);
    	 
    	 item = new IconMenuItem(_("Forward"),new BMessage(M_FORWARD_MESSAGE),'J',0,
    	 						utils.GetBitmapResource('BBMP',"Forward"));
    	 item->SetEnabled( (sel >= 0)?true:false);
    	 theMenu->AddItem(item);
    	 theMenu->AddSeparatorItem();
    	 item = new IconMenuItem(_("Move To Trash"),new BMessage(M_DELETE_MSG),'T',0,
    	 						utils.GetBitmapResource('BBMP',"Trash"));
    	 item->SetEnabled( (sel >= 0)?true:false);
    	 theMenu->AddItem(item);
    	// Add to people menu
    	theMenu->AddSeparatorItem();
    	item = new IconMenuItem(_("Save as People"),new BMessage(M_ADD_TO_PEOPLE),0,0,
    	 						utils.GetBitmapResource('BBMP',"Person"));
    	item->SetEnabled( (sel >= 0)?true:false);
    	theMenu->AddItem(item);
    	item = new IconMenuItem(_("Add To BlackList"),new BMessage(M_ADD_TO_BLACK_LIST),0,0,
    							utils.GetBitmapResource('BBMP',"BlackList"));
    	item->SetEnabled( (sel >= 0)?true:false);
    	theMenu->AddItem(item);
    	theMenu->AddSeparatorItem();
 		// Status menu
    	BMenu *statusMenu = new BMenu(_("Change Status"));
    	statusMenu->SetFont(&font);
    	const char* status[] = {"New","Read","Replied","Forwarded"};
    	
    	for(int32 i = 0;i < (int32)(sizeof(status)/sizeof(status[0]));i++)
    	{
    		msg = new BMessage(M_CHANGE_MAIL_STATUS);
    		msg->AddString("status",status[i]);
    		item = new IconMenuItem(_(status[i]),msg,0,0,NULL);
    		statusMenu->AddItem(item);
    	}
    	theMenu->AddItem(statusMenu);
    	theMenu->AddSeparatorItem();
    	// Open with menu
    	 OpenWithMenu *submenu = new OpenWithMenu(_("Open With…"),"text/x-email");
    	 submenu->SetFont(&font);
		// Get icon for executable file
		BMimeType exe("application/x-vnd.Be-elfexecutable");
		BBitmap *icon = new BBitmap(BRect(0,0,15,15),B_COLOR_8_BIT);
    	exe.GetIcon(icon,B_MINI_ICON);
    	item = new IconMenuItem(submenu,NULL,0,0,icon);
    	item->SetEnabled( (sel>=0)?true:false);
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
	 }else{
	 	_inherited::MouseDown(point);
	}
}

/***********************************************************
 * InitiateDrag
 ***********************************************************/
bool
HMailList::InitiateDrag(BPoint  point,
						int32 index,
						bool wasSelected)
{
	if (wasSelected) 
	{
		BMessage msg(M_MAIL_DRAG);
		HMailItem *item = cast_as(ItemAt(index),HMailItem);
		if(!item)
			return false;
		BRect	theRect = this->ItemFrame(index);
		int32 sel = ((HWindow*)Window())->FolderSelection();
		if(sel < 0)
			return false;
		msg.AddInt32("sel",sel);
		int32 selected; 
		int32 sel_index = 0;
		while((selected = CurrentSelection(sel_index++)) >= 0)
		{
			item=cast_as(ItemAt(selected),HMailItem);
			if(!item)
				continue;
			msg.AddPointer("pointer",item);
			msg.AddRef("refs",&item->fRef);
			//PRINT(("Sel:%d\n",selected));
		}
			
		const char *subject = item->GetColumnContentText(1);
		theRect.OffsetTo(B_ORIGIN);
		theRect.right = theRect.left + StringWidth(subject) + 20;
		BBitmap *bitmap = new BBitmap(theRect,B_RGBA32,true);
		BView *view = new BView(theRect,"",B_FOLLOW_NONE,0);
		bitmap->AddChild(view);
		bitmap->Lock();
		view->SetHighColor(0,0,0,0);
		view->FillRect(view->Bounds());
		
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetHighColor(0,0,0,128);
		view->SetBlendingMode(B_CONSTANT_ALPHA,B_ALPHA_COMPOSITE);
		const BBitmap *icon = item->GetColumnContentBitmap(0);
		if(icon)
			view->DrawBitmap(icon);
		
		BFont font;
		GetFont(&font);	
		view->SetFont(&font);
		view->MovePenTo(theRect.left+18, theRect.bottom-3);
		
		if(subject)
			view->DrawString( subject );
		bitmap->Unlock();
		
		DragMessage(&msg, bitmap, B_OP_ALPHA,
				BPoint(bitmap->Bounds().Width()/2,bitmap->Bounds().Height()/2));
		// must not delete bitmap
	}	
	return (wasSelected);
}


/***********************************************************
 * ReadSettings
 ***********************************************************/
status_t
HMailList::ReadSettings(const char* relativepath, BMessage *msg)
{
	BPath path;
	BFile file;
	
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Attribute");
	::create_directory(path.Path(),0777);
	
	if(fFolderType == QUERY_TYPE)
	{
		path.Append("Query");
		::create_directory(path.Path(),0777);
	}else if(fFolderType == IMAP4_TYPE){
		path.Append("IMAP4");
		::create_directory(path.Path(),0777);
	}
	MakePath(path,relativepath);
	PRINT(("%s\n",path.Path() ));

	if(path.InitCheck() != B_OK)
		return B_ERROR;
	// Store the setting files ref
	fCurrentFolder = new BEntry(path.Path());

	if(file.SetTo(path.Path(),B_READ_ONLY) != B_OK)
		return B_ERROR;
	
	msg->Unflatten(&file);
	
	return B_OK;
}
 

/***********************************************************
 * MakePath
 ***********************************************************/ 
void
HMailList::MakePath(BPath &basePath,const char* relative)
{
	int32 len = ::strlen(relative);
	char *path = new char[len+1];
	::strcpy(path,relative);
	path[len] = '\0';
	BString leafName;
	for(int32 i = 0;i < len;i++)
	{
		leafName = "";
		while(path[i] != '/')
		{
			if(path[i] == '\0')
			{
				leafName += ".attr";
				basePath.Append(leafName.String());
				goto end;
			}
			leafName += path[i++];	
		}
		basePath.Append(leafName.String());
		::create_directory(basePath.Path(),0777);
	}
end:
	delete[] path;
}

/***********************************************************
 * RefreshColumns
 ***********************************************************/
void
HMailList::RefreshColumns(BMessage *settings)
{
	int32 display_order[7];
	float column_width[6];
	char column_name[] = "column1";
	char width_name[] = "width1";
	int32 flags,sort_key=-1,sort_mode=0;
	float width;
	
	if (settings)
	{
		settings->FindInt32("flags",&flags);
		settings->FindInt32("sort_key",&sort_key);
		settings->FindInt32("sort_mode",&sort_mode);
		
		for(int32 i = 0;i < 7;i++)
		{
			display_order[i] = settings->FindInt32(column_name);
			column_name[6]++;
		}
		
		for(int32 i = 0;i < 6;i++)
		{
			if(settings->FindFloat(width_name,&width) != B_OK)
				width = (i < 4)?100:20;
			column_width[i] = width;
			width_name[5]++;
		}
	}
	else
	{
		PRINT(("Enter default column\n"));
		display_order[0] = 0;display_order[1] = 5;display_order[2] = 6;
		display_order[3] = 1;display_order[4] = 2;display_order[5] = 3;
		display_order[6] = 4;
		
		sort_key = 4;
		sort_mode = 1;
		flags = COLUMN_SUBJECT|COLUMN_FROM|COLUMN_TO|COLUMN_WHEN|COLUMN_PRIORITY|COLUMN_ATTACHMENT;
		for(int32 i = 0;i < 6;i++)
		{
			column_width[i] = (i < 4)?100:20;
		}
	}
	SetColumns(flags,display_order,sort_key,sort_mode,column_width);
}

/***********************************************************
 * SetColumns
 ***********************************************************/
void
HMailList::SetColumns(int32 flags,
						int32 *display_order,
						int32 sort_key,
						int32 sort_mode,
						float *column_width)
{
	const int32 kFlags[] = {COLUMN_SUBJECT,COLUMN_FROM,COLUMN_TO
							,COLUMN_WHEN,COLUMN_PRIORITY,COLUMN_ATTACHMENT};

	for(int32 i = 0;i < 6;i++)
	{
		if( flags & kFlags[i])
			SetColumnShown((ColumnType)i,true);	
		else
			SetColumnShown((ColumnType)i,false);	
	}
	
	if(display_order)
		SetDisplayOrder(display_order);
	
	SetSortMode(sort_key,(CLVSortMode)sort_mode);
	SetSortKey(sort_key);
	
	for(int32 i = 0;i < 6;i++)
	{
		CLVColumn *col = ColumnAt(i+1);
		col->SetWidth(column_width[i]);
	}
}

/***********************************************************
 * RefreshScrollPos
 ***********************************************************/
void
HMailList::RefreshScrollPos(BMessage *settings)
{
	BPoint scroll_pos(0, 0);
	settings->FindPoint("scroll_pos", &scroll_pos);
	ScrollTo(scroll_pos);
}

/***********************************************************
 * HasSomeSelection
 ***********************************************************/
bool
HMailList::HasSomeSelection()
{
	int32 count = 0,i = 0;
	
	while(CurrentSelection(i++) >= 0)
	{
		count++;
		if(count > 1)
			break;
	}
	return (count > 1)?true:false;
}

/***********************************************************
 * MarkOldSelectionAsRead
 ***********************************************************/
void
HMailList::MarkOldSelectionAsRead()
{
	if(fOldSelection)
	{
		if(IndexOf(fOldSelection) < 0)
		{
			PRINT(("Invalid Item\n"));
			fOldSelection = NULL;
			return;
		}
		if( is_kind_of(fOldSelection,HIMAP4Item) )
			fOldSelection->SetRead();
		else{
			entry_ref ref = fOldSelection->Ref();
			BNode node(&ref);
			BString status;
			node.ReadAttrString(B_MAIL_ATTR_STATUS,&status);
			if(status.Compare("New") == 0)
				node.WriteAttr(B_MAIL_ATTR_STATUS,B_STRING_TYPE,0,"Read",5);
		}
		InvalidateItem(IndexOf(fOldSelection));
		fOldSelection = NULL;
	}
}

/***********************************************************
 * ResolveSpecifier
 ***********************************************************/
BHandler*
HMailList::ResolveSpecifier(BMessage *message,
									int32 index,
									BMessage *specifier,
									int32 what,
									const char *property)
{
	BPropertyInfo propertyInfo(const_cast<property_info *>(kMailPropertyList));
	
	int32 result = propertyInfo.FindMatch(message, index, specifier, what, property);
	if (result < 0) 
		return _inherited::ResolveSpecifier(message, index, specifier,what, property);

	return this;
}

/***********************************************************
 * GetSupportedSuites
 ***********************************************************/
status_t
HMailList::GetSupportedSuites(BMessage *data)
{
	data->AddString("suites", kScriptingSuites);
	BPropertyInfo propertyInfo(const_cast<property_info *>(kMailPropertyList));
	data->AddFlat("messages", &propertyInfo);
	
	return _inherited::GetSupportedSuites(data);
}

/***********************************************************
 * HandleScriptingMessage
 ***********************************************************/
bool
HMailList::HandleScriptingMessage(BMessage *message)
{
	if (message->what != B_GET_PROPERTY
		&& message->what != B_SET_PROPERTY
		&& message->what != B_CREATE_PROPERTY
		&& message->what != B_COUNT_PROPERTIES
		&& message->what != B_DELETE_PROPERTY
		&& message->what != B_EXECUTE_PROPERTY)
		return false;
	
	// dispatch scripting messages
	BMessage reply(B_REPLY);
	const char *property = 0;
	bool handled = false;

	int32 index = 0;
	int32 form = 0;
	BMessage specifier;
	status_t result = message->GetCurrentSpecifier(&index, &specifier,
		&form, &property);

	if (result != B_OK || index == -1) 
		return false;
		
	switch (message->what) 
	{
		case B_GET_PROPERTY:
			handled = GetProperty(&specifier, form, property, &reply);
			break;
	}
	if (handled) 
		// done handling message, send a reply
		message->SendReply(&reply);
	return handled;
}

/***********************************************************
 * GetProperty
 ***********************************************************/
bool
HMailList::GetProperty(BMessage *specifier, int32 form, const char *property,
	BMessage *reply)
{
	bool handled = false;
	status_t error = B_OK;
	entry_ref item_ref;
	HMailItem *item(NULL);
	
	if (strcmp(property, "Entry") == 0) {
		switch (form) {
			case B_DIRECT_SPECIFIER:
			{
				int32 count = CountItems();
				// return all entries in MailList
				for (int32 index = 0; index < count; index++)
				{
					item = (HMailItem*)ItemAt(index);
					if(!item)
						continue;
					item_ref = item->Ref(); 
					reply->AddRef("result",&item_ref);
				}
				handled = true;
				break;
			}
			case B_INDEX_SPECIFIER:
				{
					// return entry at index
					int32 index;
					if (specifier->FindInt32("index", &index) != B_OK)
						break;
					
					if (!ItemAt(index)) {
						error = B_BAD_INDEX;
						handled = true;
						break;
					}
					item = (HMailItem*)ItemAt(index);
					item_ref = item->Ref(); 
					reply->AddRef("result", &item_ref);
	
					handled = true;
					break;
				}
			case kPreviousSpecifier:
			case kNextSpecifier:
				{
					// return entry and index of MailItem before or after specified pose
					entry_ref ref;
					if (specifier->FindRef("data", &ref) != B_OK)
						break;
					
					int32 count = CountItems();
					int32 tmp=-1;
					for(int32 i = 0;i < count;i++)
					{
						item = (HMailItem*)ItemAt(i);
						if(!item)
							continue;
						if(item->Ref() == ref)
						{
							tmp = IndexOf(item);
							break;	
						}
					}
					if(form == (int32)kPreviousSpecifier)
						tmp--;
					else if(form == (int32)kNextSpecifier)
						tmp++;
						
					if (tmp < 0 || count == tmp) {
						error = B_ENTRY_NOT_FOUND;
						handled = true;
						break;
					}					
					item = (HMailItem*)ItemAt(tmp);
					item_ref = item->Ref();
					reply->AddRef("result",&item_ref);
					reply->AddInt32("index",tmp);
	
					handled = true;
					break;
				}
		}
	}
	
	if (error != B_OK)
		reply->AddInt32("error", error);

	return handled;
}
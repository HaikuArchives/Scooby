#include "HFilterView.h"
#include "HCriteriaView.h"
#include "HApp.h"
#include "TrackerUtils.h"
#include "Utilities.h"

#include <ScrollView.h>
#include <Menu.h>
#include <Box.h>
#include <MenuItem.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <Button.h>
#include <Debug.h>
#include <String.h>
#include <ClassInfo.h>
#include <File.h>
#include <Alert.h>
#include <Beep.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HFilterView::HFilterView(BRect rect)
	:BView(rect,"Filters",B_FOLLOW_ALL,B_WILL_DRAW|B_PULSE_NEEDED)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	InitGUI();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HFilterView::~HFilterView()
{
	fListView->SetSelectionMessage(NULL);
	int32 count = fListView->CountItems();
	while(count>0)
	{
		BStringItem *item = cast_as(fListView->RemoveItem(--count),BStringItem);
		delete item;
	}
	RemoveAllCriteria();
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HFilterView::InitGUI()
{
	BRect rect(Bounds());
	rect.top += 10;
	rect.left +=5;
	rect.bottom-= 65;
	rect.right = rect.left +100;
	BBox *box;
	BRect frame;
	BPath path;
	BDirectory dir;
	BEntry entry;

	
	fListView = new BListView(rect,"list");
	fListView->SetFont(be_bold_font);
	fListView->SetSelectionMessage(new BMessage(M_FILTER_CHG));
	fListView->SetTarget(this,Window());
	BScrollView *scroll = new BScrollView("scrollview",fListView,
						B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT,
						0,false,true);
	AddChild(scroll);
	frame= rect;
	frame.top = frame.bottom + 5;
	frame.right= frame.left + 55;
	BButton *button;
	button = new BButton(frame,"del",_("Delete"),new BMessage(M_DEL_FILTER_MSG));
	button->SetEnabled(false);
	AddChild(button);
	frame.OffsetBy(60,0);
	button = new BButton(frame,"add",_("Add"),new BMessage(M_ADD_FILTER_MSG));
	AddChild(button);
	
	rect.OffsetBy(rect.Width()+30,0);
	rect.bottom = rect.top + 150;
	rect.right= Bounds().right - B_V_SCROLL_BAR_WIDTH*2;
	
	box = new BBox(rect,"cri");
	box->SetLabel(_("Criteria"));
	frame.OffsetTo(B_ORIGIN);
	frame= box->Bounds();
	frame.InsetBy(2,0);
	frame.top += 15;
	frame.left += 10;
	frame.right -= B_V_SCROLL_BAR_WIDTH+10;
	frame.bottom -= 30;
	BView *criteriaBG = new BView(frame,"criteria_bg",
								B_FOLLOW_ALL,B_WILL_DRAW);
	criteriaBG->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	scroll = new BScrollView("criteria_scroll",criteriaBG,
						B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT,
						0,false,true);
	box->AddChild(scroll);
	BScrollBar *sbar = scroll->ScrollBar(B_VERTICAL);
	sbar->SetValue(0.0);
	sbar->SetSteps(10.0,30.0);
	sbar->SetRange(0,0);
	frame.OffsetBy(B_V_SCROLL_BAR_WIDTH,frame.Height()+2);
	frame.bottom = frame.top + 25;
	frame.left = frame.right - 40;
	float width = frame.Width();
	button = new BButton(frame,"criteria_del",_("Delete"),new BMessage(M_DEL_CRITERIA_MSG));
	button->ResizeToPreferred();
	box->AddChild(button);
	if(width < button->Bounds().Width() )
		button->MoveBy(width-button->Bounds().Width(),0 );
	frame = button->Frame();
	frame.OffsetBy(-button->Bounds().Width()-5,0);
	button = new BButton(frame,"criteria_add",_("Add"),new BMessage(M_ADD_CRITERIA_MSG));
	button->ResizeToPreferred();
	box->AddChild(button);
	button->SetEnabled(false);

	AddChild(box);
	
	rect.OffsetBy(0,rect.Height());
	rect.bottom = rect.top + 45;
	box = new BBox(rect,"act");
	box->SetLabel(_("Action"));
	rect = box->Bounds();
	rect.top += 15;
	rect.left+=10;
	rect.bottom = rect.top + 25;
	BMenu *menu  = new BMenu("action");
	menu->AddItem( new BMenuItem(_("Move to"),NULL));
	menu->SetRadioMode(true);
	BMenuItem *item = menu->ItemAt(0);
	if(item)
		item->SetMarked(true);
	menu->SetLabelFromMarked(true);
	fActionMenu = new BMenuField(rect,"action","",menu);
	fActionMenu->SetDivider(0);
	box->AddChild(fActionMenu);
	AddChild(box);
	menu  = new BMenu(_("folder"));
/*	
	::find_directory(B_USER_DIRECTORY,&path);
	path.Append("mail");
	BEntry entry(path.Path());

	AddFolderItem(entry,menu);
*/
	menu->SetRadioMode(true);
	item = menu->ItemAt(0);
	if(item)
		item->SetMarked(true);
	menu->SetLabelFromMarked(true);
	rect.OffsetBy(StringWidth(_("Move to")) + 30,0);
	fFolderMenu = new BMenuField(rect,"folder","",menu);
	fFolderMenu->SetDivider(0);
	box->AddChild(fFolderMenu);
	
	rect.bottom = Bounds().bottom-65;
	rect.top = rect.bottom - 25;
	rect.OffsetBy(30,0);
	fNameControl = new BTextControl(rect,"name",_("Name:"),"",NULL);
	fNameControl->SetDivider(StringWidth("Name:")+5);
	// Disallow charactors that could not use filename
	DisallowFilenameKeys(fNameControl->TextView());
	DisallowMetaKeys(fNameControl->TextView());
	AddChild(fNameControl);
	
	// Apply change button
	rect.bottom = Bounds().bottom - 30;
	rect.right = Bounds().right - 10;
	rect.left = rect.right - 80;
	rect.top = rect.bottom - 30;
	
	width = rect.Width();
	button = new BButton(rect,"apply",_("Apply Changes"),new BMessage(M_FILTER_SAVE_CHANGED));
	button->ResizeToPreferred();
	if(width < button->Bounds().Width() )
		button->MoveBy(width-button->Bounds().Width(),0 );
	AddChild(button);
	
	// Load saved filters
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Filters");
	dir.SetTo(path.Path());
	status_t err = B_OK;
	while(err == B_OK)
	{
		if((err = dir.GetNextEntry(&entry)) == B_OK )
		{
			char name[B_FILE_NAME_LENGTH+1];
			entry.GetName(name);
			fListView->AddItem( new BStringItem(name));
		}
	}

	SetEnableControls(false);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HFilterView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_ADD_CRITERIA_MSG:
		AddCriteria();
		break;
	case M_DEL_CRITERIA_MSG:
		RemoveCriteria();
		break;
	case M_ADD_FILTER_MSG:
		New();
		break;
	case M_DEL_FILTER_MSG:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel>=0)
		{
			fListView->DeselectAll();
			BStringItem *item = cast_as(fListView->RemoveItem(sel),BStringItem);
			BPath path;
			::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
			path.Append(APP_NAME);
			path.Append("Filters");
			path.Append(item->Text());
			entry_ref ref;
			::get_ref_for_path(path.Path(),&ref);
			TrackerUtils().MoveToTrash(ref);
			
			delete item;
		}
		break;
	}
	case M_FILTER_CHG:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel>=0)
		{
			BStringItem *item = cast_as(fListView->ItemAt(sel),BStringItem);
			SetEnableControls(true);
			OpenItem(item->Text());	
			fNameControl->SetText(item->Text() );
		}else
			SetEnableControls(false);
		break;
	}
	case M_FILTER_SAVE_CHANGED:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel < 0)
			break;
		SaveItem(sel);
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HFilterView::Pulse()
{
	BButton *delete_btn = cast_as(FindView("criteria_del"),BButton);
	if(!delete_btn) return;
	BView *view = FindView("criteria_bg");
	if(!view) return;
	
	BView *focus(NULL);
	int32 count = view->CountChildren();
	
	for(int32 i = 0;i < count;i++)
	{
		focus = view->ChildAt(i);
		if(focus && focus->IsFocus() )
		{
			delete_btn->SetEnabled(true);
			return;
		}
	}
	delete_btn->SetEnabled(false);
}

/***********************************************************
 * New
 ***********************************************************/
void
HFilterView::New()
{
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Filters");
	::create_directory(path.Path(),0777);
	
	BDirectory dir(path.Path());
	BFile file;
	entry_ref ref;
	TrackerUtils().SmartCreateFile(&file,&dir,_("Untitled"),"",B_READ_WRITE,&ref);
	path.SetTo(&ref);
	fListView->AddItem(new BStringItem(path.Leaf()));
}

/***********************************************************
 * SetEnableControls
 ***********************************************************/
void
HFilterView::SetEnableControls(bool enable)
{
	fNameControl->SetText("");
	fNameControl->SetEnabled(enable);
	fFolderMenu->SetEnabled(enable);
	fActionMenu->SetEnabled(enable);
	
	BButton *button;
	button = cast_as(FindView("del"),BButton);
	int32 sel = fListView->CurrentSelection();
	button->SetEnabled((sel <0)?false:true);
	
	button = cast_as(FindView("apply"),BButton);
	button->SetEnabled(enable);
	
	button = cast_as(FindView("criteria_add"),BButton);
	button->SetEnabled(enable);

	if(!enable)
		RemoveAllCriteria();
}

/***********************************************************
 * SaveItem
 ***********************************************************/
void
HFilterView::SaveItem(int32 index,bool rename)
{
	if(index<0)
		return;
	BStringItem *item = cast_as(fListView->ItemAt(index),BStringItem);
	if(!item)
		return;
	BString name = fNameControl->Text();
	
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	::create_directory(path.Path(),0777);
	path.Append("Filters");
	
	path.Append(item->Text());
	if(rename)
	{
		if( name.Compare(item->Text()) != 0)
		{
			BEntry entry(path.Path());
			if(entry.Rename(name.String()) != B_OK)
			{
				(new BAlert("",_("Could not rename filer file"),_("OK")))->Go();
				return;
			}
			entry.GetPath(&path);
			PRINT(("RENAMED\n"));
		}
		item->SetText(name.String());
		fListView->InvalidateItem(index);
	}
	
	BMenu *menu;
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
	
	BView *view = cast_as(FindView("criteria_bg"),BView);
	int32 count = view->CountChildren();
	if(count == 0)
	{
	    // There is no criteria
		beep();
		(new BAlert("",_("You must add one or more criteria."),_("OK"),
						NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}

	BMessage msg(B_SIMPLE_DATA);
	BMenuItem *menuitem;
	menu = fFolderMenu->Menu();
	if(!(menuitem = menu->FindMarked()))
	{
		(new BAlert("",_("You have to set filter action"),_("OK")))->Go();
		return;
	}else
		msg.AddString("action_value",menuitem->Label());

	for(int32 i = 0;i < count;i++)
	{
		HCriteriaView *criteria = cast_as(view->ChildAt(i),HCriteriaView);
		if(!criteria)
			continue;
		msg.AddInt32("attribute", criteria->Attribute() );
		msg.AddString("attr_value",criteria->AttributeValue());
		msg.AddInt32("operation1",criteria->Operator());
		msg.AddInt32("operation2",criteria->Operator2());
	}
	menu = fActionMenu->Menu();

	msg.AddInt32("action",menu->IndexOf(menu->FindMarked() ) );
	ssize_t numBytes;
	msg.Flatten(&file,&numBytes);
	file.SetSize(numBytes);
	PRINT(("SAVED\n"));
}

/***********************************************************
 * OpenItem
 ***********************************************************/
status_t
HFilterView::OpenItem(const char* name)
{
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	::create_directory(path.Path(),0777);
	path.Append("Filters");
	path.Append(name);
	
	BFile file(path.Path(),B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return B_ERROR;
	
	BMessage msg;
	msg.Unflatten(&file);
	BMenu *menu;
	BMenuItem *item;
	int32 value;
	
	RemoveAllCriteria();
	
	int32 count;
	type_code type;
	msg.GetInfo("attribute",&type,&count);
	int32 attr,op,op2;
	const char* attr_value;
	
	for(int32 i = 0;i < count;i++)
	{
		msg.FindInt32("attribute",i,&attr);
		msg.FindInt32("operation1",i,&op);
		msg.FindInt32("operation2",i,&op2);
		
		msg.FindString("attr_value",i,&attr_value);
		AddCriteria(attr,op,attr_value,op2);
	}
	
	if( msg.FindInt32("action",&value) == B_OK)
	{
		menu = fActionMenu->Menu();
		item = menu->ItemAt(value);
		if(item)
			item->SetMarked(true);
	}
	
	const char* action_value;
	if(msg.FindString("action_value",&action_value) == B_OK)
	{
		menu = fFolderMenu->Menu();
		item = menu->FindItem(action_value);
		if(item)
			item->SetMarked(true);
	}
	return B_OK;
}

/***********************************************************
 * AddFolderItem
 ***********************************************************/
void
HFilterView::AddFolderItem(BMessage *msg)
{
	BMenuField *field = cast_as(FindView("folder"),BMenuField);
	
	BMenu *menu = field->Menu();
	type_code type;
	int32 count;
	msg->GetInfo("path",&type,&count);
	for(int32 i = 0;i < count;i++)
	{
		const char* path;
		if(msg->FindString("path",i,&path) == B_OK)
			menu->AddItem( new BMenuItem(path,NULL));
	}
}

/***********************************************************
 * AddCriteria
 ***********************************************************/
void
HFilterView::AddCriteria(int32 attr,
						int32 operation,
						const char* attr_value,
						int32 operation2)
{
	BView *view = cast_as(FindView("criteria_bg"),BView);
	int32 count = view->CountChildren();
	
	const char kViewHeight = 30;
	BRect rect(0,0,view->Bounds().Width(),kViewHeight);
	rect.OffsetBy(0,count*kViewHeight);
	
	HCriteriaView *criteria = new HCriteriaView(rect,"");
	view->AddChild(criteria);
	
	if(attr>=0)
		criteria->SetValue(attr,operation,attr_value,operation2);
	RefreshCriteriaScroll();
}

/***********************************************************
 * RemoveCriteria
 ***********************************************************/
void
HFilterView::RemoveCriteria()
{
	BView *view = cast_as(FindView("criteria_bg"),BView);
	int32 count = view->CountChildren();
	BView *child;
	
	for(int32 i = 0;i < count;i++)
	{
		child = view->ChildAt(i);
		if(!child)
			continue;
		if(child->IsFocus())
		{
			BView *sibling = child->NextSibling();
			while( sibling )
			{
				sibling->MoveBy(0,-30);
				sibling = sibling->NextSibling();
			}
			view->RemoveChild(child);
			delete child;
			break;
		}
	}
	RefreshCriteriaScroll();
}

/***********************************************************
 * RemoveAllCriteria
 ***********************************************************/
void
HFilterView::RemoveAllCriteria()
{
	BView *view = cast_as(FindView("criteria_bg"),BView);
	int32 count = view->CountChildren();
	BView *child;
	
	for(int32 i = 0;i < count;i++)
	{
		child = view->ChildAt(0);
		if(!child)
			continue;
		view->RemoveChild(child);
		delete child;	
	}
	RefreshCriteriaScroll();
}

/***********************************************************
 * RefreshCriteriaScroll
 ***********************************************************/
void
HFilterView::RefreshCriteriaScroll()
{
	BScrollView *scroll = cast_as(FindView("criteria_scroll"),BScrollView);
	BView *view = cast_as(FindView("criteria_bg"),BView);
	int32 count = view->CountChildren();
	BScrollBar *sbar = scroll->ScrollBar(B_VERTICAL);
	sbar->SetRange(0,(scroll->Bounds().Height()<count*30)?(count*30-scroll->Bounds().Height()):0 );
}

/***********************************************************
 * AttachedToWindow
 ***********************************************************/
void
HFilterView::AttachedToWindow()
{
	BButton *button;
	button = cast_as(FindView("add"),BButton);
	button->SetTarget(this);
	button = cast_as(FindView("del"),BButton);
	button->SetTarget(this);
	button = cast_as(FindView("criteria_add"),BButton);
	button->SetTarget(this);
	button = cast_as(FindView("criteria_del"),BButton);
	button->SetTarget(this);
	button = cast_as(FindView("apply"),BButton);
	button->SetTarget(this);
	fListView->SetTarget(this);
}
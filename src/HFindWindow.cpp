#include "HFindWindow.h"
#include "HApp.h"
#include "ComboBox.h"

#include <View.h>
#include <Button.h>
#include <TextControl.h>
#include <ClassInfo.h>
#include <Debug.h>
#include <File.h>
#include <Path.h>
#include <FindDirectory.h>

enum{
	M_TEXT_CHANGED = 'mTEC',
	M_FIND = 'mFnd'
};

#define MAX_HISTORIES 10

/***********************************************************
 * Constructor
 ***********************************************************/
HFindWindow::HFindWindow(BRect rect,const char* name)
		:BWindow(rect,name,B_FLOATING_WINDOW,B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
		,fTarget(NULL)
		,fCanQuit(false)
{
	BRect bounds(Bounds());
	BView *view = new BView(bounds,"",B_FOLLOW_ALL,B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	bounds.InsetBy(10,10);
	bounds.bottom = bounds.top + 30;
	
	BString label(_("Find"));
	label += ": ";
	ComboBox *control = new ComboBox(bounds,"text",label.String(),
									"",NULL);
	control->SetModificationMessage(new BMessage(M_TEXT_CHANGED));
	control->SetDivider(view->StringWidth(label.String()) + 3);
	view->AddChild(control);
	
	bounds.OffsetBy(0,bounds.Height()+3);
	bounds.bottom = bounds.top + 20;
	bounds.left = bounds.right - 70;
	BButton *btn = new BButton(bounds,"find",_("Find"),new BMessage(M_FIND));
	view->AddChild(btn);
	btn->SetEnabled(false);
	btn->MakeDefault(true);
	AddChild(view);	
	control->TextControl()->MakeFocus(true);
	
	LoadHistory();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HFindWindow::~HFindWindow()
{
	ComboBox *ctrl = cast_as(FindView("text"),ComboBox);
	ctrl->SetModificationMessage(NULL);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HFindWindow::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_FIND:
	{	
		ComboBox *combo = cast_as(FindView("text"),ComboBox);
		BTextControl *ctrl = combo->TextControl();
		const char* text = ctrl->Text();
		BMessage msg('find');
		msg.AddString("findthis",text);
		if(fTarget)
			fTarget->Looper()->PostMessage(&msg,fTarget);
		AddToHistory(text);
		break;
	}
	case M_TEXT_CHANGED:
	{
		BButton *button = cast_as(FindView("find"),BButton);
		ComboBox *combo = cast_as(FindView("text"),ComboBox);
		BTextControl *ctrl = combo->TextControl();
		if(ctrl->TextView()->TextLength() > 0)
			button->SetEnabled(true);
		else
			button->SetEnabled(false);
		break;
	}
	default:
		BWindow::MessageReceived(message);
	}
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HFindWindow::QuitRequested()
{
	if(!fCanQuit)
	{
		Hide();
		return false;
	}
	SaveHistory();
	return BWindow::QuitRequested();
}

/***********************************************************
 * SaveHistory
 ***********************************************************/
void
HFindWindow::SaveHistory()
{
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append("Scooby");
	path.Append("SearchHistory");
	
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
	
	if(file.InitCheck() != B_OK)
		return;
	ComboBox *combo = cast_as(FindView("text"),ComboBox);
	
	int32 count = combo->CountItems();
	
	BMessage msg;
	for(int32 i = 0;i < count;i++)
		msg.AddString("text",combo->ItemAt(i));
	msg.Flatten(&file);
}

/***********************************************************
 * LoadHistory
 ***********************************************************/
void
HFindWindow::LoadHistory()
{
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append("Scooby");
	path.Append("SearchHistory");
	
	BFile file(path.Path(),B_READ_ONLY);

	if(file.InitCheck() != B_OK)
		return;
	BMessage msg;
	msg.Unflatten(&file);
	
	ComboBox *combo = cast_as(FindView("text"),ComboBox);
	
	int32 count;
	type_code type;
	const char *text;
	msg.GetInfo("text",&type,&count);
	for(int32 i = count-1;i >= 0;i--)
	{
		if(msg.FindString("text",i,&text) == B_OK)
			combo->AddItem(text);
	}
}

/***********************************************************
 * AddToHistory
 ***********************************************************/
void
HFindWindow::AddToHistory(const char* text)
{
	ComboBox *combo = cast_as(FindView("text"),ComboBox);
	combo->AddItem(text);
	
	int32 count = combo->CountItems();
	if(count > MAX_HISTORIES)
		combo->RemoveItem(MAX_HISTORIES+1);
}
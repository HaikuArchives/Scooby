#include "HFindWindow.h"
#include "HApp.h"

#include <View.h>
#include <Button.h>
#include <TextControl.h>
#include <ClassInfo.h>

enum{
	M_TEXT_CHANGED = 'mTEC',
	M_FIND = 'mFnd'	
};

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
	BTextControl *control = new BTextControl(bounds,"text",label.String(),
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
	control->MakeFocus(true);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HFindWindow::~HFindWindow()
{
	BTextControl *ctrl = cast_as(FindView("text"),BTextControl);
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
		BTextControl *ctrl = cast_as(FindView("text"),BTextControl);
		const char* text = ctrl->Text();
		BMessage msg('find');
		msg.AddString("findthis",text);
		if(fTarget)
			fTarget->Looper()->PostMessage(&msg,fTarget);
		break;
	}
	case M_TEXT_CHANGED:
	{
		BButton *button = cast_as(FindView("find"),BButton);
		BTextControl *ctrl = cast_as(FindView("text"),BTextControl);
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
	return BWindow::QuitRequested();
}
#include "HDetailView.h"
#include "MenuUtils.h"
#include "HApp.h"

#include <TextControl.h>
#include <StringView.h>
#include <Query.h>
#include <Menu.h>
#include <MenuField.h>
#include <String.h>
#include <Entry.h>
#include <Window.h>
#include <Debug.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <ClassInfo.h>
#include <List.h>
#include <StringView.h>
#include <ScrollBar.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HDetailView::HDetailView(BRect rect,bool readOnly)
	:BView(rect,"WriteView",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,B_WILL_DRAW)
	,fReadOnly(readOnly)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	InitGUI();
	SetReadOnly(readOnly);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HDetailView::~HDetailView()
{
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HDetailView::InitGUI()
{
	float divider = StringWidth(_("Subject:")) +5;
	divider = max_c(divider, StringWidth(_("From:")));
	divider = max_c(divider ,StringWidth(_("When:"))); 
	
	BRect rect = Bounds();
	rect.top += 3;
	rect.left += 5;
	rect.right -= 5 + B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 20;
	
	BTextControl *ctrl;

	const char* kName[] = {"subject","from","when"};
	const char* kLabel[] = {_("Subject:"),_("From:"),_("When:")};
	
	for(int32 i = 0;i < 3;i++)
	{
		ctrl = new BTextControl(BRect(rect.left,rect.top
								,rect.right
								,rect.bottom)
								,kName[i],kLabel[i],"",NULL
								,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,B_WILL_DRAW|B_NAVIGABLE);
		
		ctrl->SetDivider(divider);

		AddChild(ctrl);
	
		rect.OffsetBy(0,20);
	}
	fSubject = cast_as(FindView("subject"),BTextControl);
	fFrom = cast_as(FindView("from"),BTextControl);
	fWhen = cast_as(FindView("when"),BTextControl);

}


/***********************************************************
 * SetReadOnly
 ***********************************************************/
void
HDetailView::SetReadOnly(bool enable)
{
	fSubject->SetEnabled(!enable);
	fFrom->SetEnabled(!enable);
	fWhen->SetEnabled(!enable);
}

/***********************************************************
 * Subject
 ***********************************************************/
const char*
HDetailView::Subject()
{
	return fSubject->Text();
}

/***********************************************************
 * SetInfo
 ***********************************************************/
void
HDetailView::SetInfo(const char* subject,
					const char* from,
					const char* when)
{
	fSubject->SetText(subject);
	fFrom->SetText(from);
	fWhen->SetText(when);
}

/***********************************************************
 * FocusedView
 ***********************************************************/
BTextControl*
HDetailView::FocusedView() const
{
	int32 count = CountChildren();
	
	BTextControl *child(NULL);
	for(int32 i = 0;i < count;i++)
	{
		child = cast_as(ChildAt(i),BTextControl);
		if(child && child->TextView()->IsFocus())
			return child;
	}
	return NULL;
}
#include "HDetailView.h"
#include "MenuUtils.h"
#include "HApp.h"
#include "MultiLineTextControl.h"
#include "ArrowButton.h"

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
	rect.left += 20;
	rect.right -= 5 + B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 18;
	
	MultiLineTextControl *ctrl;
	const char* kName[] = {"subject","from","when","cc","to"};
	const char* kLabel[] = {_("Subject:"),_("From:"),_("When:"),_("Cc:"),_("To:")};
	
	for(int32 i = 0;i < 5;i++)
	{
		
		ctrl = new MultiLineTextControl(BRect(rect.left,rect.top
								,rect.right
								,rect.bottom)
								,kName[i],kLabel[i],"","",NULL
								,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,B_WILL_DRAW|B_NAVIGABLE);
		ctrl->SetDivider(divider);
		ctrl->SetTextMargin(1);
		AddChild(ctrl);
		// Add Arrow button
		if(i == 2)
		{
			BRect arrowRect(rect);
			arrowRect.OffsetBy(0,3);
			arrowRect.left = Bounds().left - 2;
			arrowRect.right = arrowRect.left + 16;
			arrowRect.bottom = arrowRect.top + 16;
			ArrowButton *arrow = new ArrowButton(arrowRect,"addr_arrow"
										,new BMessage(M_EXPAND_ATTRIBUTES));
			AddChild(arrow);
		}
		rect.OffsetBy(0,21);
	}
	fSubject = cast_as(FindView("subject"),MultiLineTextControl);
	fFrom = cast_as(FindView("from"),MultiLineTextControl);
	fWhen = cast_as(FindView("when"),MultiLineTextControl);
	fCc = cast_as(FindView("cc"),MultiLineTextControl);
	fTo = cast_as(FindView("to"),MultiLineTextControl);
}


/***********************************************************
 * SetReadOnly
 ***********************************************************/
void
HDetailView::SetReadOnly(bool enable)
{
	//fSubject->SetEnabled(!enable);
	//fFrom->SetEnabled(!enable);
	//fWhen->SetEnabled(!enable);
	fSubject->TextView()->MakeEditable(!enable);
	fFrom->TextView()->MakeEditable(!enable);
	fWhen->TextView()->MakeEditable(!enable);
	fCc->TextView()->MakeEditable(!enable);
	fTo->TextView()->MakeEditable(!enable);
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
					const char* when,
					const char* cc,
					const char* to)
{
	fSubject->SetText(subject);
	fFrom->SetText(from);
	fWhen->SetText(when);
	fCc->SetText(cc);
	fTo->SetText(to);
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

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HDetailView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	// Expand attributes
	case M_EXPAND_ATTRIBUTES:
	{
		BWindow *window = Window();
		BView *siblingView = window->FindView("scroller");
		if(Bounds().Height() <= DETAIL_VIEW_HEIGHT)
		{
			ResizeBy(0,DETAIL_VIEW_HEIGHT_EXPANDED-DETAIL_VIEW_HEIGHT);
			siblingView->ResizeBy(0,DETAIL_VIEW_HEIGHT-DETAIL_VIEW_HEIGHT_EXPANDED);
			siblingView->MoveBy(0,DETAIL_VIEW_HEIGHT_EXPANDED-DETAIL_VIEW_HEIGHT);
		}else{
			ResizeBy(0,DETAIL_VIEW_HEIGHT-DETAIL_VIEW_HEIGHT_EXPANDED);
			siblingView->ResizeBy(0,DETAIL_VIEW_HEIGHT_EXPANDED-DETAIL_VIEW_HEIGHT);
			siblingView->MoveBy(0,DETAIL_VIEW_HEIGHT-DETAIL_VIEW_HEIGHT_EXPANDED);		
		}
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}

#include "HPrefWindow.h"
#include "HFilterView.h"
#include "HAccountView.h"
#include "HGeneralSettingView.h"
#include "HSignatureView.h"
#include "HApp.h"
#include "HPrefs.h"

#include <Message.h>
#include <TabView.h>
#include <Rect.h>
#include <Button.h>
#include <Application.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HPrefWindow::HPrefWindow(BRect win_rect)
	:BWindow(win_rect,_("Preferences"),B_TITLED_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_ASYNCHRONOUS_CONTROLS|B_NOT_CLOSABLE)
{
	//this->AddShortcut(B_RETURN,0,new BMessage(M_APPLY_MESSAGE));	
	this->AddShortcut('W',0,new BMessage(B_QUIT_REQUESTED));
	
	BRect rect = Bounds();
	rect.bottom -= 35;
	BTabView *tabview = new BTabView(rect,"tabview");
	tabview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect frame = tabview->Bounds();
//*********** Other Setting ******************/
	tabview->AddTab(fGeneralView = new HGeneralSettingView(frame));
	tabview->TabAt(0)->SetLabel(_("General"));
	
//*********** Account Setting ******************/
	tabview->AddTab(fAccountView = new HAccountView(frame));
	tabview->TabAt(1)->SetLabel(_("Account"));

//*********** Filter Setting ******************/
	tabview->AddTab(fFilterView = new HFilterView(frame));
	tabview->TabAt(2)->SetLabel(_("Filters"));
//*********** Signature Setting ******************/
	tabview->AddTab(fSignatureView = new HSignatureView(frame));
	tabview->TabAt(3)->SetLabel(_("Signature"));
	
	AddChild(tabview);
	
	BRect bgrect = Bounds();
	bgrect.top = bgrect.bottom - 35;
	BView *bgview = new BView(bgrect,"bgview",B_FOLLOW_ALL,B_WILL_DRAW);
	bgview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	bgrect.OffsetTo(B_ORIGIN);
	bgrect.top += 5;
	bgrect.right -= 10;
	bgrect.left = bgrect.right - 80;
	bgrect.bottom -= 5;

	BButton *button = new BButton(bgrect,"close",_("Close"),new BMessage(B_QUIT_REQUESTED));
	bgview->AddChild(button);
	this->AddChild(bgview);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HPrefWindow::~HPrefWindow()
{
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HPrefWindow::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_ADD_FILTER_MSG:
	case M_DEL_FILTER_MSG:
	case M_FILTER_CHG:
	case M_FILTER_SAVE_CHANGED:
	case M_ADD_CRITERIA_MSG:
	case M_DEL_CRITERIA_MSG:
		PostMessage(message,fFilterView);
		break;
	case M_ADD_ACCOUNT:
	case M_DEL_ACCOUNT:
	case M_CHANGE_ACCOUNT:
	case M_ACCOUNT_SAVE_CHANGED:
		PostMessage(message,fAccountView);
		break;
	case M_SIGNATURE_SAVE_CHANGED:
	case M_ADD_SIGNATURE:
	case M_DEL_SIGNATURE:
	case M_CHANGE_SIGNATURE:
		PostMessage(message,fSignatureView);
		break;
	case M_FONT_CHANGED:
	case 'FSel':
		PostMessage(message,fGeneralView);
		break;
	default:
		BWindow::MessageReceived(message);
	}
}


/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HPrefWindow::QuitRequested()
{
	return BWindow::QuitRequested();
}
#include "HPrefWindow.h"
#include "HFilterView.h"
#include "HAccountView.h"
#include "HGeneralSettingView.h"
#include "HSignatureView.h"
#include "HApp.h"
#include "HPrefs.h"
#include "HSpamFilterView.h"

#include <Message.h>
#include <TabView.h>
#include <Rect.h>
#include <Button.h>
#include <Application.h>
#include <ClassInfo.h>

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
	tabview->TabAt(1)->SetLabel(_("Accounts"));

//*********** Filter Setting ******************/
	tabview->AddTab(fFilterView = new HFilterView(frame));
	tabview->TabAt(2)->SetLabel(_("Filters"));
//*********** Signature Setting ******************/
	tabview->AddTab(fSignatureView = new HSignatureView(frame));
	tabview->TabAt(3)->SetLabel(_("Signatures"));
//*********** SpamFilter Setting ******************/
	tabview->AddTab(fSpamFilterView = new HSpamFilterView(frame));
	tabview->TabAt(4)->SetLabel(_("Spam Filters"));
	
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
	case M_FONT_CHANGED:
	case 'FSel':
		PostMessage(message,fGeneralView);
		break;
	case M_ADD_FOLDERS:
		fFilterView->AddFolderItem(message);
		break;
	default:
		BWindow::MessageReceived(message);
	}
}

/***********************************************************
 * CreateFilter
 ***********************************************************/
void
HPrefWindow::CreateFilter(const char* subject,
						const char* from,
						const char* to,
						const char* cc,
						const char* account)
{
	// Select filter tab
	BTabView *tabview = cast_as(FindView("tabview"),BTabView);
	tabview->Select(2);
	// Create new filter
	fFilterView->New(true);
	// Set criteria
	if(subject&& strlen(subject) > 0)
		fFilterView->AddCriteria(0,1,subject,0);
	if(from && strlen(from) > 0)
		fFilterView->AddCriteria(1,1,from,0);
	if(to && strlen(to) > 0)
		fFilterView->AddCriteria(2,1,to,0);
	if(cc && strlen(cc) > 0)
		fFilterView->AddCriteria(3,1,cc,0);
	if(account && strlen(account)>0)
		fFilterView->AddCriteria(4,1,account,0);
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HPrefWindow::QuitRequested()
{
	return BWindow::QuitRequested();
}
#include "HReadWindow.h"
#include "MenuUtils.h"
#include "HMailView.h"
#include "HDetailView.h"
#include "ResourceUtils.h"
#include "HToolbar.h"
#include "HApp.h"
#include "HPrefs.h"
#include "HWindow.h"

#include <Menu.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <E-mail.h>
#include <Debug.h>
#include <ScrollView.h>
#include <Beep.h>
#include <Messenger.h>
#include <ClassInfo.h>


/***********************************************************
 * Constructor
 ***********************************************************/
HReadWindow::HReadWindow(BRect rect,entry_ref ref,BMessenger *messenger)
	:BWindow(rect,"",B_DOCUMENT_WINDOW,0)
	,fMessenger(messenger)
	,fRef(ref)
	,fCurrentIndex(0)
	,fEntryList(NULL)
{
	InitMenu();
	InitGUI();

	LoadMessage(ref);
	
	if(fMessenger)
		InitIndex();
	fMailView->MakeFocus(true);
	
	AddShortcut(B_UP_ARROW,0,new BMessage(M_PREV_MESSAGE));
	AddShortcut(B_DOWN_ARROW,0,new BMessage(M_NEXT_MESSAGE));
}

/***********************************************************
 * Destructor
 ***********************************************************/
HReadWindow::~HReadWindow()
{
	delete fMessenger;
	delete fEntryList;
}


/***********************************************************
 * InitGUI
 ***********************************************************/
void
HReadWindow::InitGUI()
{
	int16 mode;
	((HApp*)be_app)->Prefs()->GetData("toolbar_mode",&mode);
	const int32 kToolbarHeight = (mode == 0)?50:30;

	BRect rect = Bounds();
	rect.top += KeyMenuBar()->Bounds().Height() + kToolbarHeight;
	/********** TextView *********/
	rect.bottom = rect.top + DETAIL_VIEW_HEIGHT;
	fDetailView = new HDetailView(rect,true);
	AddChild(fDetailView);
	
	rect.top = rect.bottom+1;
	rect.bottom = Bounds().bottom;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	fMailView = new HMailView(rect,true,NULL);
	fMailView->MakeEditable(false);
	BScrollView *scroll = new BScrollView("scroller",fMailView,B_FOLLOW_ALL,0,false,true);
	AddChild(scroll);
	/*
	KeyMenuBar()->FindItem(B_CUT)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_COPY)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_PASTE)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_SELECT_ALL)->SetTarget(fMailView,this);
	*/
	KeyMenuBar()->FindItem(B_UNDO)->SetTarget(fMailView,this);
	/********** Toolbar ***********/
	BRect toolrect = Bounds();
	toolrect.top += (KeyMenuBar()->Bounds()).Height();
	toolrect.bottom = toolrect.top + kToolbarHeight;
	toolrect.right += 2;
	toolrect.left -= 1;
	ResourceUtils utils;
	HToolbar *toolbox = new HToolbar(toolrect,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	if(kToolbarHeight == 50)
		toolbox->UseLabel(true);
	toolbox->AddButton("New",utils.GetBitmapResource('BBMP',"New Message"),new BMessage(M_NEW_MSG),"New Message");
	toolbox->AddSpace();
	BMessage *msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",false);
	toolbox->AddButton("Reply",utils.GetBitmapResource('BBMP',"Reply"),msg,"Reply Message");
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",true);
	toolbox->AddButton("All",utils.GetBitmapResource('BBMP',"Reply To All"),msg,"Reply Message To All");
	toolbox->AddButton("Fwd",utils.GetBitmapResource('BBMP',"Forward"),new BMessage(M_FORWARD_MESSAGE),"Forward Message");
	if(fMessenger)
	{
		toolbox->AddSpace();
		toolbox->AddButton("Next",utils.GetBitmapResource('BBMP',"Next"),new BMessage(M_NEXT_MESSAGE),"Next Message");
		toolbox->AddButton("Prev",utils.GetBitmapResource('BBMP',"Prev"),new BMessage(M_PREV_MESSAGE),"Prev Message");
	}
	
	AddChild(toolbox);
}

/***********************************************************
 * InitMenu
 ***********************************************************/
void
HReadWindow::InitMenu()
{
	BMenuBar *menubar = new BMenuBar(Bounds(),"");
    BMenu		*aMenu;
	MenuUtils utils;
	BPath path;
	ResourceUtils rsrc_utils;
	BMessage *msg;
//// ------------------------ File Menu ----------------------    
	aMenu = new BMenu(_("File"));
	utils.AddMenuItem(aMenu,_("Print Message"),M_PRINT_MESSAGE,this,this,'P',0,
							rsrc_utils.GetBitmapResource('BBMP',"Printer"));
	utils.AddMenuItem(aMenu,_("Page Setup…"),M_PAGE_SETUP_MESSAGE,be_app,be_app,'P',B_SHIFT_KEY,
							rsrc_utils.GetBitmapResource('BBMP',"PageSetup"));
	aMenu->AddSeparatorItem();					
	utils.AddMenuItem(aMenu,_("Close"),B_QUIT_REQUESTED,this,this,'W',0);
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Quit"),B_QUIT_REQUESTED,be_app,be_app,'Q',0);
	menubar->AddItem( aMenu );
	// Edit
	aMenu = new BMenu(_("Edit"));
   	utils.AddMenuItem(aMenu,_("Undo"),B_UNDO,this,this,'Z',0);
   	aMenu->AddSeparatorItem();
   	utils.AddMenuItem(aMenu,_("Cut"),B_CUT,this,this,'X',0);
   	utils.AddMenuItem(aMenu,_("Copy"),B_COPY,this,this,'C',0);
   	utils.AddMenuItem(aMenu,_("Paste"),B_PASTE,this,this,'V',0);
   	aMenu->AddSeparatorItem();
   	utils.AddMenuItem(aMenu,_("Select All"),B_SELECT_ALL,this,this,'A',0);
   	menubar->AddItem(aMenu);
	////------------------------- Message Menu ---------------------
	aMenu = new BMenu(_("Message"));
	utils.AddMenuItem(aMenu,_("New Message"),M_NEW_MSG,this,this,'N',0,
							rsrc_utils.GetBitmapResource('BBMP',"New Message"));
	aMenu->AddSeparatorItem();
	
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",false);
	utils.AddMenuItem(aMenu,_("Reply"),msg,this,this,'R',0,
							rsrc_utils.GetBitmapResource('BBMP',"Reply"));
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",true);
	utils.AddMenuItem(aMenu,_("Reply To All"),msg,this,this,'R',B_SHIFT_KEY,
							rsrc_utils.GetBitmapResource('BBMP',"Reply To All"));
							
	utils.AddMenuItem(aMenu,_("Forward"),M_FORWARD_MESSAGE,this,this,'J',0,
							rsrc_utils.GetBitmapResource('BBMP',"Forward"));
	aMenu->AddSeparatorItem();
	
    utils.AddMenuItem(aMenu,_("Show Header"),M_HEADER,this,this,'H',0);
    utils.AddMenuItem(aMenu,_("Show Raw Message"),M_RAW,this,this,0,0);
    menubar->AddItem( aMenu );
	
    AddChild(menubar);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HReadWindow::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_FORWARD_MESSAGE:
	case M_NEW_MSG:
	case M_REPLY_MESSAGE:
	{
		BMessage msg(*message);
		msg.AddRef("refs",&fRef);
		((HApp*)be_app)->MainWindow()->PostMessage(&msg);
		break;
	}
	case M_PREV_MESSAGE:
		SiblingItem(-1);
		break;
		
	case M_NEXT_MESSAGE:
		SiblingItem(1);
		break;
	// Show header 
	case M_HEADER:
		message->AddBool("header",!fMailView->IsShowingHeader());
		PostMessage(message,fMailView);
		break;
	case M_RAW:
		message->AddBool("raw",!fMailView->IsShowingRawMessage());
		PostMessage(message,fMailView);
		break;
	case M_PRINT_MESSAGE:
	{
		BMessage msg(*message);
		msg.AddString("job_name",fDetailView->Subject());
		msg.AddPointer("view",fMailView);
		msg.AddPointer("detail",fDetailView);
		be_app->PostMessage(&msg);
		break;
	}
	// Edit menu items
	case B_SELECT_ALL:
	case B_PASTE:
	case B_CUT:
	case B_COPY:
	{
		BView *view = CurrentFocus();
		if(view)
			PostMessage(message,view);
		break;
	}
	default:
		BWindow::MessageReceived(message);
	}
}

/***********************************************************
 * LoadMessage
 ***********************************************************/
void
HReadWindow::LoadMessage(entry_ref ref)
{
	fRef = ref;
	BFile *file = new BFile(&ref,B_READ_ONLY);
	if(file->InitCheck() != B_OK)
	{
		delete file;
		fMailView->SetContent(NULL);
		return;
	}
	BString from,subject;
	time_t when;
	file->ReadAttrString(B_MAIL_ATTR_FROM,&from);
	file->ReadAttrString(B_MAIL_ATTR_SUBJECT,&subject);
	file->ReadAttr(B_MAIL_ATTR_WHEN,B_TIME_TYPE,0,&when,sizeof(time_t));
	const char* kTimeFormat = "%a, %m/%d/%Y, %r";
	char *buf = new char[64];
	struct tm* time = localtime(&when);
	 ::strftime(buf, 64,kTimeFormat, time);
	fDetailView->SetInfo(subject.String(),from.String(),buf);
	delete[] buf;
	fMailView->SetContent(file);
	SetTitle(subject.String());
}

/***********************************************************
 * MenusBeginning
 ***********************************************************/
void
HReadWindow::MenusBeginning()
{
	BMenuItem *item;
	item = KeyMenuBar()->FindItem(M_HEADER);
	item->SetMarked(fMailView->IsShowingHeader());
	item = KeyMenuBar()->FindItem(M_RAW);
	item->SetMarked(fMailView->IsShowingRawMessage());
	
	BTextControl *ctrl(NULL);
	int32 start,end;
	// Copy
	if(CurrentFocus() == fMailView)
	{	
		fMailView->GetSelection(&start,&end);
	
		if(start != end)
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(true);
		else
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(false);
	}else if((ctrl = fDetailView->FocusedView())){
		BTextView *textview = ctrl->TextView();
		textview->GetSelection(&start,&end);
		
		if(start != end)
		{
			KeyMenuBar()->FindItem(B_CUT)->SetEnabled(true);
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(true);
		} else {
			KeyMenuBar()->FindItem(B_CUT)->SetEnabled(false);
			KeyMenuBar()->FindItem(B_COPY )->SetEnabled(false);
		}
	}
	// Disabled items
	KeyMenuBar()->FindItem(B_UNDO)->SetEnabled(false);
	KeyMenuBar()->FindItem(B_CUT)->SetEnabled(false);
	KeyMenuBar()->FindItem(B_PASTE)->SetEnabled(false);
	// Select All
	BView *view = CurrentFocus();
	if(is_kind_of(view,BTextView))
		item->SetEnabled(true);
	else if(ctrl)
		item->SetEnabled(true);
	else
		item->SetEnabled(false);
}

/***********************************************************
 * InitIndex
 ***********************************************************/
void
HReadWindow::InitIndex()
{
	BMessage msg,reply;
	msg.what =  B_GET_PROPERTY;
	msg.AddSpecifier("Entry");
	
	fMessenger->SendMessage(&msg,&reply);
	fEntryList = new BMessage(reply);
	int32 count;
	type_code type;
	entry_ref ref;
	fEntryList->GetInfo("result", &type, &count);
	
	for(int32 i = 0;i < count;i++)
	{
		fEntryList->FindRef("result",i,&ref);
		if(fRef == ref)
		{
			fCurrentIndex= i;
			break;
		}
	}
}


/***********************************************************
 * SiblingItem
 ***********************************************************/
void
HReadWindow::SiblingItem(int32 how_far)
{
	SetRead();
	if(!fEntryList)
	{
		beep();
		return;
	}
	entry_ref ref;
	if(fEntryList->FindRef("result",fCurrentIndex+how_far,&ref) == B_OK)
	{
		Select(ref);
		LoadMessage(ref);
		fRef = ref;
		fCurrentIndex += how_far;
	}else
		beep();
}


/***********************************************************
 * Select
 ***********************************************************/
void
HReadWindow::Select(entry_ref ref)
{
	BMessage msg;
	msg.what = B_SET_PROPERTY;
	msg.AddSpecifier("Selection");
	msg.AddRef("data", &ref);
	
	fMessenger->SendMessage(&msg); 
}


/***********************************************************
 * KeyDown
 ***********************************************************/
void
HReadWindow::DispatchMessage(BMessage *message,BHandler *handler)
{
	if(message->what == B_KEY_DOWN&&handler == fMailView)
	{
		// Scroll down with space key
		const char* bytes;
		int32 modifiers;
		
		message->FindInt32("modifiers",&modifiers);
		message->FindString("bytes",&bytes);
		char c = (modifiers & B_SHIFT_KEY)?B_PAGE_UP:B_PAGE_DOWN;
		if(bytes[0] == B_SPACE)
		{
			char b[1];
			b[0] = c;
			fMailView->KeyDown(b,1);
		}
	}
	BWindow::DispatchMessage(message,handler);
}

/***********************************************************
 * SetRead
 ***********************************************************/
void
HReadWindow::SetRead()
{
	BNode node(&fRef);
	if(node.InitCheck() == B_OK)
	{
		BString status;
		node.ReadAttrString(B_MAIL_ATTR_STATUS,&status);
		if(status.Compare("New") == 0)
		{
			status = "Read";
			node.WriteAttrString(B_MAIL_ATTR_STATUS,&status);
		}
	}
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HReadWindow::QuitRequested()
{
	SetRead();
	((HApp*)be_app)->Prefs()->SetData("read_win_rect",Frame());
	return BWindow::QuitRequested();
}
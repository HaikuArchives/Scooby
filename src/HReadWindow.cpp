#include "HReadWindow.h"
#include "MenuUtils.h"
#include "HMailView.h"
#include "HDetailView.h"
#include "ResourceUtils.h"
#include "HToolbar.h"
#include "HApp.h"
#include "HPrefs.h"
#include "HWindow.h"
#include "HHtmlMailView.h"
#include "HMailList.h"
#include "HAttachmentList.h"
#include "TrackerUtils.h"
#include "HTabView.h"
#include "Utilities.h"
#include "StatusBar.h"

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
	:BWindow(rect,"",B_DOCUMENT_WINDOW,B_ASYNCHRONOUS_CONTROLS)
	,fMessenger(messenger)
	,fRef(ref)
	,fCurrentIndex(0)
{
	InitMenu();
	InitGUI();

	LoadMessage(ref);
	
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
	
	rect.top = rect.bottom;
	rect.bottom = Bounds().bottom;
	bool html;
	((HApp*)be_app)->Prefs()->GetData("use_html",&html);
	if(!html)
	{
		rect.right -= B_V_SCROLL_BAR_WIDTH;
		rect.bottom -= B_H_SCROLL_BAR_HEIGHT;
		HMailView *mailView = new HMailView(rect,true,NULL);
		mailView->MakeEditable(false);
		BScrollView *scroll = new BScrollView("scroller",mailView,B_FOLLOW_ALL,B_WILL_DRAW,false,true);
		AddChild(scroll);
		//================ StatusBar ==================
		BRect statusRect(Bounds());
		statusRect.top = statusRect.bottom - B_H_SCROLL_BAR_HEIGHT;
		statusRect.OffsetBy(0,1);
		StatusBar *statusbar = new StatusBar(statusRect,NULL,B_FOLLOW_BOTTOM|B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW);
		AddChild(statusbar);
		BString label;
		label = _("Size");
		label += ": 0 byte";
		statusbar->AddItem("size",label.String(),SizeUpdate);
		//=============================================
		fMailView = cast_as(mailView,BView);
	}else{	
		rect.top -= 1;
		HHtmlMailView *htmlMailView = new HHtmlMailView(rect,"scroller",false,B_FOLLOW_ALL);
		AddChild(htmlMailView);
		
		fMailView = cast_as(htmlMailView,BView);
	}
	/*
	KeyMenuBar()->FindItem(B_CUT)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_COPY)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_PASTE)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_SELECT_ALL)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_UNDO)->SetTarget(fMailView,this);
	*/
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
	toolbox->AddSpace();
	toolbox->AddButton("Trash",utils.GetBitmapResource('BBMP',"Trash"),new BMessage(M_DELETE_MSG),_("Move To Trash"));
	toolbox->AddSpace();
	toolbox->AddButton("Print",utils.GetBitmapResource('BBMP',"Printer"),new BMessage(M_PRINT_MESSAGE),_("Print"));
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
	utils.AddMenuItem(aMenu,_("Page Setup" B_UTF8_ELLIPSIS),M_PAGE_SETUP_MESSAGE,be_app,be_app,'P',B_SHIFT_KEY,
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
   	aMenu->AddSeparatorItem();
   	msg = new BMessage(M_SHOW_FIND_WINDOW);
 	msg->AddPointer("targetwindow",this);
   	utils.AddMenuItem(aMenu,_("Find" B_UTF8_ELLIPSIS),msg,be_app,be_app,'F',0);
   	msg = new BMessage(M_FIND_NEXT_WINDOW);
 	msg->AddPointer("targetwindow",this);
   	utils.AddMenuItem(aMenu,_("Find Next"),msg,be_app,be_app,'G',0);
   	menubar->AddItem(aMenu);
	////------------------------- Message Menu ---------------------
	aMenu = new BMenu(_("Message"));
	utils.AddMenuItem(aMenu,_("New Message" B_UTF8_ELLIPSIS),M_NEW_MSG,this,this,'N',0,
							rsrc_utils.GetBitmapResource('BBMP',"New Message"));
	aMenu->AddSeparatorItem();
	
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",false);
	utils.AddMenuItem(aMenu,_("Reply" B_UTF8_ELLIPSIS),msg,this,this,'R',0,
							rsrc_utils.GetBitmapResource('BBMP',"Reply"));
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",true);
	utils.AddMenuItem(aMenu,_("Reply To All" B_UTF8_ELLIPSIS),msg,this,this,'R',B_SHIFT_KEY,
							rsrc_utils.GetBitmapResource('BBMP',"Reply To All"));
							
	utils.AddMenuItem(aMenu,_("Forward" B_UTF8_ELLIPSIS),M_FORWARD_MESSAGE,this,this,'J',0,
							rsrc_utils.GetBitmapResource('BBMP',"Forward"));
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Move To Trash"),M_DELETE_MSG,this,this,'T',0,
							rsrc_utils.GetBitmapResource('BBMP',"Trash"));
	aMenu->AddSeparatorItem();
    
    utils.AddMenuItem(aMenu,_("Show Headers"),M_HEADER,this,this,'H',0);
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
		SiblingItem('sprv');
		break;
		
	case M_NEXT_MESSAGE:
		SiblingItem('snxt');
		break;
	// Show header 
	case M_HEADER:
		if(is_kind_of(fMailView,HMailView))
		{
			HMailView *view = cast_as(fMailView,HMailView);
			message->AddBool("header",!view->IsShowingHeader());
			PostMessage(message,fMailView);
			break;
		}
	case M_RAW:
		if(is_kind_of(fMailView,HMailView))
		{
			HMailView *view = cast_as(fMailView,HMailView);
			message->AddBool("raw",!view->IsShowingRawMessage());
			PostMessage(message,fMailView);
			break;
		}
	// Print message
	case M_PRINT_MESSAGE:
		PrintMessage(message);
		break;
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
	// Expand attributes
	case M_EXPAND_ATTRIBUTES:
		PostMessage(message,fDetailView);
		break;
	case M_OPEN_ATTACHMENT:
	case M_SAVE_ATTACHMENT:
		PostMessage(message,fMailView);
		break;
	// Delete mails
	case M_DELETE_MSG:
	{
		Hide();
		BPath path;
		::find_directory(B_USER_DIRECTORY,&path);
		path.Append("mail");
		path.Append("Trash");
		
		if(path.InitCheck() != B_OK)
		{
			(new BAlert("",_("Could not find Trash directory"),_("OK"),
							NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
			break;
		}
		entry_ref ref;
		::get_ref_for_path(path.Path(),&ref);
		
		BEntry entry(&fRef);
		if(entry.InitCheck() != B_OK)
		{
			PRINT(("%s:%s\n",__FILE__,"Cound not found this mail"));
			break;
		}
		BDirectory dir(&ref);
		
		TrackerUtils().SmartMoveFile(fRef,&dir,"_");
		PostMessage(B_QUIT_REQUESTED);
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
		PostMessage(M_SET_CONTENT,fMailView);
		return;
	}
	BString from,subject,cc,to;
	time_t when;
	ReadNodeAttrString(file,B_MAIL_ATTR_FROM,&from);
	ReadNodeAttrString(file,B_MAIL_ATTR_SUBJECT,&subject);
	file->ReadAttr(B_MAIL_ATTR_WHEN,B_TIME_TYPE,0,&when,sizeof(time_t));
	ReadNodeAttrString(file,B_MAIL_ATTR_CC,&cc);
	ReadNodeAttrString(file,B_MAIL_ATTR_TO,&to);
	
	const char* kTimeFormat = "%a, %m/%d/%Y, %r";
	char *buf = new char[64];
	struct tm* time = localtime(&when);
	 ::strftime(buf, 64,kTimeFormat, time);
	fDetailView->SetInfo(subject.String(),from.String(),buf,cc.String(),to.String());
	delete[] buf;
	BMessage msg(M_SET_CONTENT);
	msg.AddPointer("pointer",file);
	PostMessage(&msg,fMailView);
	SetTitle(subject.String());
}

/***********************************************************
 * MenusBeginning
 ***********************************************************/
void
HReadWindow::MenusBeginning()
{
	BMenuItem *item(NULL);
	if(is_kind_of(fMailView,HMailView))
	{
		HMailView *view = cast_as(fMailView,HMailView);
		MarkMenuItem(KeyMenuBar()->FindItem(M_HEADER),view->IsShowingHeader());
		MarkMenuItem(KeyMenuBar()->FindItem(M_RAW),view->IsShowingRawMessage());
	}else{
		EnableMenuItem(KeyMenuBar()->FindItem(M_HEADER),false);
		EnableMenuItem(KeyMenuBar()->FindItem(M_RAW),false);
	}
	BTextControl *ctrl(NULL);
	int32 start,end;
	// Copy
	if(CurrentFocus() == fMailView )
	{	
		BTextView *view(NULL);
		if(is_kind_of(fMailView,HMailView))
			view = cast_as(fMailView,BTextView);
		
		if(view)
		{
			view->GetSelection(&start,&end);
			if(start != end)
				EnableMenuItem(KeyMenuBar()->FindItem(B_COPY),true);
			else
				EnableMenuItem(KeyMenuBar()->FindItem(B_COPY),false);
		}
	}else if((ctrl = fDetailView->FocusedView())){
		BTextView *textview = ctrl->TextView();
		textview->GetSelection(&start,&end);
		
		if(start != end)
		{
			EnableMenuItem(KeyMenuBar()->FindItem(B_CUT),true);
			EnableMenuItem(KeyMenuBar()->FindItem(B_COPY),true);
		} else {
			EnableMenuItem(KeyMenuBar()->FindItem(B_CUT),false);
			EnableMenuItem(KeyMenuBar()->FindItem(B_COPY),false);
		}
	}
	// Disabled items
	EnableMenuItem(KeyMenuBar()->FindItem(B_UNDO),false);
	EnableMenuItem(KeyMenuBar()->FindItem(B_CUT),false);
	EnableMenuItem(KeyMenuBar()->FindItem(B_PASTE),false);
	// Select All
	BView *view = CurrentFocus();
	if(is_kind_of(view,BTextView))
		EnableMenuItem(item,true);
	else if(ctrl)
		EnableMenuItem(item,true);
	else
		EnableMenuItem(item,false);
}

/***********************************************************
 * SiblingItem
 ***********************************************************/
void
HReadWindow::SiblingItem(int32 what)
{
	// Set read message as read
	SetRead();
	// Send message
	BMessage specifier(what);
	specifier.AddString("property","Entry");
	specifier.AddRef("data",&fRef);
	BMessage msg,reply;
	msg.what =  B_GET_PROPERTY;
	msg.AddSpecifier(&specifier);
	
	fMessenger->SendMessage(&msg,&reply);
	entry_ref ref;
	if(reply.FindRef("result",&ref) == B_OK)
	{
		Select(ref);
		LoadMessage(ref);
		fRef = ref;
		reply.FindInt32("index",&fCurrentIndex);
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
		bool html = false;
		message->FindInt32("modifiers",&modifiers);
		message->FindString("bytes",&bytes);
		
		if(bytes[0] != B_SPACE)
			return BWindow::DispatchMessage(message,handler);
		BScrollBar *bar(NULL);
		if(is_kind_of(fMailView,HMailView))
			bar = fMailView->ScrollBar(B_VERTICAL);
		else{
			HTabView *tabview = cast_as(fMailView->FindView("tabview"),HTabView);
			html = true;
			if(tabview->Selection() == 0)
			{
				BView *view = FindView("__NetPositive__HTMLView");
				bar = view->ScrollBar(B_VERTICAL);
			}
		}
		if(!bar)
		{
			BWindow::DispatchMessage(message,handler);
			return;
		}
		float min,max,cur;
		bar->GetRange(&min,&max);
		cur = bar->Value();
		if(cur != max)
		{
			if(!html)
			{
				// Scroll down with space key
				char b[1];
				b[0] = (modifiers & B_SHIFT_KEY)?B_PAGE_UP:B_PAGE_DOWN;
				fMailView->KeyDown(b,1);
			}else{
				char b[1];
				b[0] = (modifiers & B_SHIFT_KEY)?B_PAGE_UP:B_PAGE_DOWN;
				BView *view = FindView("__NetPositive__HTMLView");
				view->KeyDown(b,1);
			}
		}else{
			if(modifiers&B_SHIFT_KEY)
				PostMessage(M_PREV_MESSAGE,this);
			else
				PostMessage(M_NEXT_MESSAGE,this);
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
		ReadNodeAttrString(&node,B_MAIL_ATTR_STATUS,&status);
		if(status.Compare("New") == 0)
		{
			status = "Read";
			node.WriteAttrString(B_MAIL_ATTR_STATUS,&status);
		}
	}
}

/***********************************************************
 * PrintMessage
 ***********************************************************/
void
HReadWindow::PrintMessage(BMessage *message)
{
	BMessage msg(*message);
	int32 sel = 0;
	
	if(is_kind_of(fMailView,HMailView))
	{
		// Normal mode
		msg.AddPointer("view",fMailView);
	}else{
		// HTML mode
		HTabView *tabview = cast_as(fMailView->FindView("tabview"),HTabView);
		sel = tabview->Selection();
		
		if(sel < 0)
			return;
		BView *view(NULL);
		switch(sel)
		{
		case 0:
			view = FindView("__NetPositive__HTMLView");
			break;
		case 1:
			view = FindView("headerview");
			break;
		case 2:
			view = FindView("attachmentlist");
			break;
		}
		if(!view)
			return;
		msg.AddPointer("view",view);	
	}
	msg.AddPointer("detail",fDetailView);
	msg.AddString("job_name",Title());
	be_app->PostMessage(&msg);
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
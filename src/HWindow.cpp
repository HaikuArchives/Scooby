#include "HWindow.h"
#include "MenuUtils.h"
#include "HMailView.h"
#include "HApp.h"
#include "HFolderList.h"
#include "HMailList.h"
#include "SplitPane.h"
#include "HPrefs.h"
#include "HMailCaption.h"
#include "HStatusView.h"
#include "HToolbar.h"
#include "ResourceUtils.h"
#include "HPopClientView.h"
#include "HWriteWindow.h"
#include "HDetailView.h"
#include "HPrefWindow.h"
#include "RectUtils.h"
#include "TrackerUtils.h"
#include "HSmtpClientView.h"
#include "SmtpLooper.h"
#include "HToolbarButton.h"
#include "OpenWithMenu.h"
#include "HReadWindow.h"
#include "HDeskbarView.h"
#include "HIMAP4Item.h"
#include "HIMAP4Folder.h"
#include "HHtmlMailView.h"
#include "HTabView.h"
#include "HAttachmentList.h"
#include "HMailItem.h"
#include "HCreateFolderDialog.h"
#include "Encoding.h"
#include "Utilities.h"
#include "LEDAnimation.h"

#include <Box.h>
#include <Beep.h>
#include <ClassInfo.h>
#include <Debug.h>
#include <E-mail.h>
#include <Deskbar.h>
#include <Autolock.h>
#include <Messenger.h>

#define BAR_THICKNESS 5
#define CAPTION_WIDTH 80


	
/***********************************************************
 * Constructor
 ***********************************************************/
HWindow::HWindow(BRect rect,
				const char* name)
	:BWindow(rect,name,B_DOCUMENT_WINDOW,B_ASYNCHRONOUS_CONTROLS)
	,fCheckIdleTime(time(NULL))
	,fCurrentDeskbarIcon(DESKBAR_NORMAL_ICON)
	,fOpenPanel(NULL)
{
	SetPulseRate(100000);
	AddShortcut('/',0,new BMessage(B_ZOOM));
	
	InitMenu();
	InitGUI();
	PostMessage(M_GET_FOLDERS,fFolderList);
	
	// SetWindowSizeLimit
	float min_width,min_height,max_width,max_height;
	GetSizeLimits(&min_width,&max_width,&min_height,&max_height);
	min_width = 400;
	min_height = 300;
	SetSizeLimits(min_width,max_width,min_height,max_height);
	
	// Deskbar
	bool use;
	((HApp*)be_app)->Prefs()->GetData("use_desktray",&use);
	if(use)
		InstallToDeskbar();
	fLEDAnimation = new LEDAnimation();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HWindow::~HWindow()
{
	delete fOpenPanel;
	delete fLEDAnimation;
}

/***********************************************************
 * Menuの作成
 ***********************************************************/
void
HWindow::InitMenu()
{
	BMenuBar *menubar = new BMenuBar(Bounds(),"");
    BMenu		*aMenu,*subMenu;
	MenuUtils utils;
	BMessage *msg;
	ResourceUtils rsrc_utils;
	HApp* app = (HApp*)be_app;
//// ------------------------ File Menu ----------------------    
	aMenu = new BMenu(_("File"));
	utils.AddMenuItem(aMenu,_("New Folder" B_UTF8_ELLIPSIS),M_CREATE_FOLDER_DIALOG,this,this,0,0,
						app->GetIcon("OpenFolder"),false);
	
	utils.AddMenuItem(aMenu,_("Open Query Folder" B_UTF8_ELLIPSIS),M_OPEN_QUERY,this,this,0,0,
							app->GetIcon("OpenQuery"),false);
	utils.AddMenuItem(aMenu,_("Empty Trash"),M_EMPTY_TRASH,this,this,'T',B_SHIFT_KEY,
							rsrc_utils.GetBitmapResource('BBMP',"Trash"));
	aMenu->AddSeparatorItem();
	subMenu = new BMenu(_("Import"));
	utils.AddMenuItem(subMenu,_("Plain Text Mails" B_UTF8_ELLIPSIS),M_IMPORT_PLAIN_TEXT_MAIL,this,this);
	utils.AddMenuItem(subMenu,_("mbox" B_UTF8_ELLIPSIS),M_IMPORT_MBOX,this,this);
	aMenu->AddItem(subMenu);
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Print Message"),M_PRINT_MESSAGE,this,this,'P',0,
							rsrc_utils.GetBitmapResource('BBMP',"Printer"));
	utils.AddMenuItem(aMenu,_("Page Setup" B_UTF8_ELLIPSIS),M_PAGE_SETUP_MESSAGE,
							be_app,be_app,'P',B_SHIFT_KEY,
							rsrc_utils.GetBitmapResource('BBMP',"PageSetup"));
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Preferences" B_UTF8_ELLIPSIS),M_PREF_MSG,this,this);
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("About Scooby" B_UTF8_ELLIPSIS),B_ABOUT_REQUESTED,be_app,be_app);
#ifndef USE_SPLOCALE
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Quit"),B_QUIT_REQUESTED,this,this,'Q',0);
#else
	aMenu->AddSeparatorItem();
    ((SpLocaleApp*)be_app)->AddToFileMenu(aMenu,false,true,true);
#endif
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
////------------------------- Mail Menu ---------------------
	aMenu = new BMenu(_("Mail"));
   	utils.AddMenuItem(aMenu,_("Check Mail"),M_POP_CONNECT,this,this,'M',0,
   							rsrc_utils.GetBitmapResource('BBMP',"Check Mail"));
	// account
	subMenu = new BMenu(_("Check Mail From"));

	aMenu->AddItem(subMenu);
	aMenu->AddSeparatorItem();
	//
	
	utils.AddMenuItem(aMenu,_("Send Pending Messages"),M_SEND_PENDING_MAILS,this,this,0,0,
							rsrc_utils.GetBitmapResource('BBMP',"Send"));
	menubar->AddItem( aMenu );
////------------------------- Message Menu ---------------------
	aMenu = new BMenu(_("Message"));
	utils.AddMenuItem(aMenu,_("New Message" B_UTF8_ELLIPSIS),M_NEW_MSG,this,this,'N',0,
							rsrc_utils.GetBitmapResource('BBMP',"New Message"));
	aMenu->AddSeparatorItem();
	
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",false);
	utils.AddMenuItem(aMenu,_("Reply to Sender Only" B_UTF8_ELLIPSIS),msg,this,this,'R',0,
							rsrc_utils.GetBitmapResource('BBMP',"Reply"));
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",true);
	utils.AddMenuItem(aMenu,_("Reply to All" B_UTF8_ELLIPSIS),msg,this,this,'R',B_SHIFT_KEY,
							rsrc_utils.GetBitmapResource('BBMP',"Reply To All"));
							
	utils.AddMenuItem(aMenu,_("Forward" B_UTF8_ELLIPSIS),M_FORWARD_MESSAGE,this,this,'J',0,
							rsrc_utils.GetBitmapResource('BBMP',"Forward"));
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Move to Trash"),M_DELETE_MSG,this,this,'T',0,
							rsrc_utils.GetBitmapResource('BBMP',"Trash"));
	aMenu->AddSeparatorItem();
    utils.AddMenuItem(aMenu,_("Filter"),M_FILTER_MAIL,this,this,0,0);
    utils.AddMenuItem(aMenu,_("Add To BlackList"),M_ADD_TO_BLACK_LIST,this,this,0,0,
    						rsrc_utils.GetBitmapResource('BBMP',"BlackList"));
    aMenu->AddSeparatorItem();
    // Status menu
    BMenu *statusMenu = new BMenu(_("Change Status"));
    const char* status[] = {"New","Read","Replied","Forwarded"};
    
    for(int32 i = 0;i < (int32)(sizeof(status)/sizeof(status[0]));i++)
    {
    	msg = new BMessage(M_CHANGE_MAIL_STATUS);
    	msg->AddString("status",status[i]);
    	utils.AddMenuItem(statusMenu,_(status[i]),msg,this,this,0,0);
    }
    aMenu->AddItem(statusMenu);
	aMenu->AddSeparatorItem();
    utils.AddMenuItem(aMenu,_("Show Headers"),M_HEADER,this,this,'H',0);
    utils.AddMenuItem(aMenu,_("Show Raw Message"),M_RAW,this,this,0,0);
    menubar->AddItem( aMenu );
////------------------------- Attr Menu ---------------------
	aMenu = new BMenu(_("Attributes"));
	const char* kAttr[] = {_("Subject"),_("From"),_("To"),_("When"),_("Priority"),_("Attachments"),_("Cc"),_("Size"),_("Account")};
	ColumnType attr_col[] = {COL_SUBJECT,COL_FROM,COL_TO,COL_WHEN,COL_PRIORITY,COL_ATTACHMENT,COL_CC,COL_SIZE,COL_ACCOUNT};
	for(int32 i = 0;i < 9;i++)
	{
		BMessage *msg = new BMessage(M_ATTR_MSG);
		msg->AddInt32("attr",attr_col[i]);
		utils.AddMenuItem(aMenu,kAttr[i],msg,this,this);
	}
	menubar->AddItem( aMenu ); 	 	
///// ------------------
	AddChild(menubar);
	AddCheckFromItems();
}


/***********************************************************
 * InitGUI
 ***********************************************************/
void
HWindow::InitGUI()
{
	int16 mode;
	((HApp*)be_app)->Prefs()->GetData("toolbar_mode",&mode);
	const int32 kToolbarHeight = (mode == 0)?50:30;
	
	BRect rect = Bounds();
	rect.top += (KeyMenuBar()->Bounds()).Height() + kToolbarHeight;
	BView *bg = new BView(rect,"bg",B_FOLLOW_ALL,B_WILL_DRAW);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bg);
//**** Mail List ******//
	BRect entryrect = rect;
	entryrect.right += 1 -B_V_SCROLL_BAR_WIDTH;
	entryrect.top = 1;
	entryrect.left = 202 + B_V_SCROLL_BAR_WIDTH;
	entryrect.bottom = entryrect.top + 200;
	
	BetterScrollView *eview;
	fMailList = new HMailList(entryrect,&eview,"maillist");
	BScrollBar *hbar = eview->ScrollBar(B_HORIZONTAL);

	hbar->ResizeBy(-CAPTION_WIDTH,0);
	hbar->MoveBy(CAPTION_WIDTH,0);
//**** Folder List ******//
	BRect localrect = rect;
	localrect.top = 1;
	localrect.left += 1;
	localrect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	localrect.right = 200;
	BetterScrollView *sview;
	fFolderList = new HFolderList(localrect,&sview,"locallist");
/********** Caption ***********/
	BRect captionframe = eview->Bounds();
	captionframe.bottom++; 
	captionframe.top = captionframe.bottom - B_H_SCROLL_BAR_HEIGHT-1;
	captionframe.right = CAPTION_WIDTH+1;
	BBox *bbox = new BBox(captionframe,NULL,B_FOLLOW_BOTTOM);
	captionframe.OffsetTo(B_ORIGIN);
	captionframe.top += 2;
	captionframe.bottom -= 2;
	captionframe.right -= 2;
	captionframe.left += 2;
	fMailCaption = new HMailCaption(captionframe,"caption",fMailList);
	bbox->AddChild(fMailCaption);
	eview->AddChild(bbox);
/********** TextView *********/
	BView *subview = new BView(entryrect,"subview",B_FOLLOW_ALL,B_WILL_DRAW);
	entryrect.OffsetTo(B_ORIGIN);
	entryrect.bottom = entryrect.top + DETAIL_VIEW_HEIGHT;
	fDetailView = new HDetailView(entryrect,true);
	subview->AddChild(fDetailView);
	entryrect.top = entryrect.bottom;
	entryrect.bottom = subview->Bounds().bottom;
	bool use_html;
	((HApp*)be_app)->Prefs()->GetData("use_html",&use_html);
	if(!use_html)
	{
		entryrect.right -= B_V_SCROLL_BAR_WIDTH;
		HMailView* mailView = new HMailView(entryrect,true,NULL);
		mailView->MakeEditable(false);
		BScrollView *scroll = new BScrollView("scroller",mailView,B_FOLLOW_ALL,0,false,true);
		subview->AddChild(scroll);
		fMailView = cast_as(mailView,BView);
	}else{
		entryrect.top --;
		HHtmlMailView *htmlMailView = new HHtmlMailView(entryrect,"scroller",false,B_FOLLOW_ALL);
		subview->AddChild(htmlMailView);
		fMailView = cast_as(htmlMailView,BView);
	}
	
/********** Horizontal SplitPane **********/
	BRect rightrect = bg->Bounds();
	rightrect.top++;
	rightrect.left += 202+ B_V_SCROLL_BAR_WIDTH; 
	rightrect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fHSplitter = new SplitPane(rightrect,eview,subview,B_FOLLOW_ALL);
	fHSplitter->SetAlignment(B_HORIZONTAL);
	fHSplitter->SetBarThickness(BPoint(BAR_THICKNESS,BAR_THICKNESS));
	fHSplitter->SetViewInsetBy(BPoint(0,0));
	BPoint pos;
	((HApp*)be_app)->Prefs()->GetData("horizontalsplit",&pos);
	fHSplitter->SetBarPosition(pos);
/********** Vertical SplitPane **********/	
	rightrect = bg->Bounds();
	rightrect.top ++;
	rightrect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fVSplitter = new SplitPane(rightrect,sview,fHSplitter,B_FOLLOW_ALL);
	fVSplitter->SetAlignment(B_VERTICAL);
	((HApp*)be_app)->Prefs()->GetData("verticalsplit",&pos);
	fVSplitter->SetBarPosition(pos);
	fVSplitter->SetBarThickness(BPoint(BAR_THICKNESS,BAR_THICKNESS));
	fVSplitter->SetViewInsetBy(BPoint(0,0));
/********** StatusView ***********/
	captionframe = bg->Bounds();
	captionframe.bottom+=2;
	captionframe.top = captionframe.bottom - B_H_SCROLL_BAR_HEIGHT-1;
	captionframe.right -= B_V_SCROLL_BAR_WIDTH-2;
	bbox = new BBox(captionframe,NULL,B_FOLLOW_BOTTOM|B_FOLLOW_LEFT_RIGHT);
	
	const float kStatusWidth = 250.0;
	captionframe.OffsetTo(B_ORIGIN);
	captionframe.top += 3;
	captionframe.left += 2;
	captionframe.right = captionframe.left + kStatusWidth;
	
	//fStatusView = new HStatusView(captionframe,"status",fFolderList);
	//bbox->AddChild(fStatusView);
	bg->AddChild(bbox);
	fPopClientView = new HPopClientView(captionframe,"popview");
	bbox->AddChild(fPopClientView);
	captionframe.OffsetBy(kStatusWidth,0);
	fSmtpClientView = new HSmtpClientView(captionframe,"smtpview");
	bbox->AddChild(fSmtpClientView);
	
	fHSplitter->SetBarAlignmentLocked(true);
	fVSplitter->SetBarAlignmentLocked(true);
	bg->AddChild(fVSplitter);
/********** Toolbar ***********/
	BRect toolrect = Bounds();
	toolrect.top += (KeyMenuBar()->Bounds()).Height();
	toolrect.bottom = toolrect.top + kToolbarHeight;
	toolrect.right += 1;
	toolrect.left -= 1;
	ResourceUtils utils;
	HToolbar *toolbox = new HToolbar(toolrect,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	if(kToolbarHeight == 50)
		toolbox->UseLabel(true);
	toolbox->AddButton("Check",utils.GetBitmapResource('BBMP',"Check Mail"),new BMessage(M_POP_CONNECT),_("Check Mail"));
	HToolbarButton *btn = cast_as(toolbox->FindView("Check"),HToolbarButton);
	if(btn)	btn->SetEnabled(false);
	
	toolbox->AddSpace();
	toolbox->AddButton("New",utils.GetBitmapResource('BBMP',"New Message"),new BMessage(M_NEW_MSG),_("New Message"));
	BMessage *msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",false);
	toolbox->AddButton("Reply",utils.GetBitmapResource('BBMP',"Reply"),msg,"Reply to Sender Only");
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",true);
	toolbox->AddButton("All",utils.GetBitmapResource('BBMP',"Reply To All"),msg,_("Reply to All"));
	toolbox->AddButton("Fwd",utils.GetBitmapResource('BBMP',"Forward"),new BMessage(M_FORWARD_MESSAGE),_("Forward Message"));
	toolbox->AddSpace();
	toolbox->AddButton("Print",utils.GetBitmapResource('BBMP',"Printer"),new BMessage(M_PRINT_MESSAGE),_("Print Message"));
	
	toolbox->AddSpace();
	toolbox->AddButton("Trash",utils.GetBitmapResource('BBMP',"Trash"),new BMessage(M_DELETE_MSG),_("Move Message to Trash"));
	
	toolbox->AddSpace();
	toolbox->AddButton("Next",utils.GetBitmapResource('BBMP',"Next"),new BMessage(M_SELECT_NEXT_MAIL),_("Next Message"));
	toolbox->AddButton("Prev",utils.GetBitmapResource('BBMP',"Prev"),new BMessage(M_SELECT_PREV_MAIL),_("Previous Message"));
	
	AddChild(toolbox);
	
	fFolderList->WatchQueryFolder();
	fFolderList->WatchMailFolder();
	fMailList->MakeFocus(true);	
	if(fMailView)
		KeyMenuBar()->FindItem(B_UNDO)->SetTarget(fMailView,this);
}

/***********************************************************
 * Message
 ***********************************************************/
void
HWindow::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	// Change mail status
	case M_CHANGE_MAIL_STATUS:
		be_app->PostMessage(message);	
		break;
	// Show Open File Panel
	case M_IMPORT_PLAIN_TEXT_MAIL:
		ShowOpenPanel(M_CONVERT_PLAIN_TO_MAIL);
		break;
	// Convert plain text mails to BeOS format.
	case M_CONVERT_PLAIN_TO_MAIL:
	{
		type_code type;
		int32 count;
		entry_ref ref;
		message->GetInfo("refs",&type,&count);
		for(int32 i = 0;i < count;i++)
		{
			if(message->FindRef("refs",i,&ref) != B_OK)
				continue;
			BPath path(&ref);
			Plain2BeMail(path.Path());
		}
		break;
	}
	// Show mbox open panel
	case M_IMPORT_MBOX:
		ShowOpenPanel(M_CONVERT_MBOX_TO_MAILS);
		break;
	// Convert mbox to BeOS mail format
	case M_CONVERT_MBOX_TO_MAILS:
	{
		type_code type;
		int32 count;
		entry_ref ref;
		message->GetInfo("refs",&type,&count);
		for(int32 i = 0;i < count;i++)
		{
			if(message->FindRef("refs",i,&ref) != B_OK)
				continue;
			BPath path(&ref);
			MBox2BeMail(path.Path());
		}
		break;
	}
	// Select All
	case B_COPY:
	case B_SELECT_ALL:
	{
		BView *view = CurrentFocus();
		if(view)
			PostMessage(message,view);
	}
	break;
	// Show write mail window
	case M_NEW_MSG:
		MakeWriteWindow();
		break;
	// New mail was created
	case M_CREATE_MAIL:
	{
		HMailItem *mail(NULL);
		if(message->FindPointer("pointer",(void**)&mail) == B_OK)
		{
			// send 
			bool send_now;
			message->FindBool("send",&send_now);
			if(send_now)
			{
				BMessage msg(M_SEND_MAIL);
				msg.AddPointer("pointer",mail);
				PostMessage(&msg,fSmtpClientView);
			}
		}
		break;
	}
	// Add mails
	case M_ADD_MAIL_TO_LIST:
	{	
		HMailItem *mail;
		int32 count;
		type_code type;
		message->GetInfo("mail",&type,&count);
		for(int32 i = 0;i < count;i++)
		{
			if(message->FindPointer("mail",i,(void**)&mail) == B_OK)
				fMailList->AddMail(mail);
		}
		break;
	}
	// Invoke mail
	case M_INVOKE_MAIL:
		InvokeMailItem();
		break;
	// Connect to pop server
	case M_POP_CONNECT:
		PopConnect();
		break;
	// Move mail
	case M_MOVE_MAIL:
		MoveMails(message);
		break;
	// Folder selection was changed 
	case M_LOCAL_SELECTION_CHANGED:
	{
		fDetailView->SetInfo("","","","","");
		PostMessage(M_SET_CONTENT,fMailView);
		PostMessage(message,fMailList);
		
		int32 count;
		type_code type;
		message->GetInfo("pointer",&type,&count);
		if(count>0)
		{
			fMailCaption->StopBarberPole();
		}
		break;
	}
	// delete mail
	case M_DELETE_MSG:
		DeleteMails();
		break;
	// Print message
	case M_PRINT_MESSAGE:
		PrintMessage(message);
		break;
	// Invalidate mail item
	case M_INVALIDATE_MAIL:
	{
		HMailItem *item;
		if(message->FindPointer("mail",(void**)&item) == B_OK)
		{
			int32 index = fMailList->IndexOf(item);
			if(index >=0)
				fMailList->InvalidateItem(index);
		}
		break;
	}
	// Open query folder
	case M_OPEN_QUERY:
	{
		BPath path;
		BDirectory dir;
		find_directory(B_USER_SETTINGS_DIRECTORY,&path);
		path.Append( APP_NAME );
		path.Append( QUERY_FOLDER );
		if(dir.SetTo(path.Path()) != B_OK)
			dir.CreateDirectory(path.Path(),&dir);
		entry_ref ref;
		::get_ref_for_path(path.Path(),&ref);
		
		BMessenger tracker("application/x-vnd.Be-TRAK" );
		BMessage msg(B_REFS_RECEIVED);
		msg.AddRef("refs",&ref);
		BMessage reply ; 
		tracker.SendMessage( &msg, &reply ); 
	
		break;
	}
	// Add sender as people file
	case M_ADD_TO_PEOPLE:
		AddToPeople();
		break;
	// Open Mail folder
	case M_OPEN_FOLDER:
	{
		int32 sel = fFolderList->CurrentSelection();
	
		if(sel < 0)
			break;
		HFolderItem *item = cast_as(fFolderList->ItemAt(sel),HFolderItem);
		if(item && item->FolderType() != IMAP4_TYPE)
			item->Launch();
		break;
	}
	// Set mail content
	case M_SET_CONTENT:
	{
		entry_ref ref;

		if(message->FindRef("refs",&ref) != B_OK)
		{
			PostMessage(M_SET_CONTENT,fMailView);
			fDetailView->SetInfo("","","","","");
			break;
		}
		// this file pointer will be deleted at MailView
		BFile *file = new BFile(&ref,B_READ_ONLY);
		if(file->InitCheck() != B_OK)
		{
			delete file;
			PostMessage(M_SET_CONTENT,fMailView);
			break;
		}
		BMessage msg(M_SET_CONTENT);
		msg.AddPointer("pointer",file);
		PostMessage(&msg,fMailView);
		// set header view
		const char* kSubject,*kFrom,*kDate,*kCc,*kTo;
	
		if(message->FindString("subject",&kSubject) == B_OK &&
			message->FindString("from",&kFrom) == B_OK &&
			message->FindString("when",&kDate) == B_OK &&
			message->FindString("cc",&kCc) == B_OK &&
			message->FindString("to",&kTo) == B_OK)
			
			fDetailView->SetInfo(kSubject,kFrom,kDate,kCc,kTo);
		else
			fDetailView->SetInfo("","","","","");
		ChangeDeskbarIcon(DESKBAR_NORMAL_ICON);
		break;
	}
	case M_PREF_MSG:
	{
		HPrefWindow *win = new HPrefWindow(RectUtils().CenterRect(600,380));
		BMessage msg(M_ADD_FOLDERS);
		fFolderList->GenarateFolderPathes(msg);
		win->PostMessage(&msg);
		win->Show();
		break;
	}
	case M_ATTR_MSG:
	{
		const char* kAttr[] = {_("Subject"),_("From"),_("To"),_("When"),_("Priority"),_("Attachments"),_("Cc"),_("Size"),_("Account")};

		int32 col;
		if(message->FindInt32("attr",&col) == B_OK)
		{
			BMenuItem *item = KeyMenuBar()->FindItem(kAttr[col]);
			
			if(!item) 
				break;
			item->SetMarked( !item->IsMarked() );
			fMailList->SetColumnShown((ColumnType)col,item->IsMarked());
		}
		break;
	}
	// Start barberpole
	case M_START_MAIL_BARBER_POLE:
		fMailCaption->StartBarberPole();
		break;
	// Stop barberpole
	case M_STOP_MAIL_BARBER_POLE:
		fMailCaption->StopBarberPole();
		break;
	// Check from
	case M_CHECK_FROM:
	{
		entry_ref ref;
		if(message->FindRef("refs",&ref) == B_OK)
		{
			CheckFrom(ref);
		}
		break;
	}
	// Send pending mails
	case M_SEND_PENDING_MAILS:
	{
		SendPendingMails();
		break;
	}
	// Delete Folders
	case M_DELETE_FOLDER:
	{
		int32 i,k = 0;
		while((i = fFolderList->CurrentSelection(k++)) >= 0)
			DeleteFolder(i);
		break;
	}
	// Open with e-mail
	case M_OPEN_WITH_MSG:
	{
		entry_ref app_ref,ref;
		const char* app_sig;
		
		BMessage msg(B_REFS_RECEIVED);
		if(message->FindString("app_sig",&app_sig) != B_OK)
			break;
		int32 sel;
		int32 i = 0;
		while((sel = fMailList->CurrentSelection(i++)) >= 0)
		{
			HMailItem *item = fMailList->MailAt(sel);
			if(item)
			{
				ref = item->Ref();
				msg.AddRef("refs",&ref);
			}
		}
		if(!msg.IsEmpty())
		{
			BMessenger messenger(fMailList,this);
			msg.AddMessenger("TrackerViewToken",messenger);	
			be_roster->Launch(app_sig,&msg);
		}
		break;
	}
	// Filter mails
	case M_FILTER_MAIL:
	{
		int32 sel;
		int32 i = 0;
		BList itemList;
		// Store item poniters to be filtered
		while((sel = fMailList->CurrentSelection(i++)) >= 0)
		{
			HMailItem *item = fMailList->MailAt(sel);
			if(item )
				itemList.AddItem(item);
		}
		// Filter items
		int32 count = itemList.CountItems();
		for(int32 i = 0;i < count;i++)
		{
			HMailItem *item = (HMailItem*)itemList.ItemAt(i);
			FilterMails(item);
		}
		break;
	}
	// Empty trashcan
	case M_EMPTY_TRASH:
	{
		EmptyTrash();
		break;
	}
	// Reply message
	case M_REPLY_MESSAGE:
	{
		entry_ref ref;
		HMailItem *item(NULL);
		bool reply_all;
		message->FindBool("reply_all",&reply_all);
		if(message->FindRef("refs",&ref) == B_OK)
		{
			item = new HMailItem(ref);
			item->fDeleteMe = true;
		}else{
		
			int32 sel = fMailList->CurrentSelection();
			if(sel < 0)
				break;	
			item = fMailList->MailAt(sel);
		}
		ReplyMail(item,reply_all);
		break;
	}
	// Forward message
	case M_FORWARD_MESSAGE:
	{
		HMailItem *item(NULL);
		entry_ref ref;
		if(message->FindRef("refs",&ref) == B_OK)
		{
			item = new HMailItem(ref);
			item->fDeleteMe = true;
		}else{
			int32 sel = fMailList->CurrentSelection();
			if(sel < 0)
				break;
			item = fMailList->MailAt(sel);
		}
		ForwardMail(item);
		break;
	}
	// Show header 
	case M_HEADER:
		if(is_kind_of(fMailView,HMailView))
		{
			HMailView *view = cast_as(fMailView,HMailView);
			message->AddBool("header",!view->IsShowingHeader());
			PostMessage(message,view);
			break;
		}
	case M_RAW:
		if(is_kind_of(fMailView,HMailView))
		{
			HMailView *view = cast_as(fMailView,HMailView);
		
			message->AddBool("raw",!view->IsShowingRawMessage());
			PostMessage(message,view);
			break;
		}
	// Refresh Folder Cache
	case M_REFRESH_CACHE:
	{
		fMailList->MakeEmpty();
		
		int32 sel = fFolderList->CurrentSelection();
		if(sel<0)
			break;
		//HFolderItem *item = cast_as(fFolderList->ItemAt(sel),HFolderItem);
		HFolderItem *item = (HFolderItem*)fFolderList->ItemAt(sel);
		if(item)
		{
			fFolderList->SetWatching( true );
			item->StartRefreshCache();
			fFolderList->InvalidateItem(fFolderList->IndexOf(item));
			fFolderList->SetWatching(true);
		}
		break;
	}
	// Add item by nodemonitor
	case M_ADD_FROM_NODEMONITOR:
	{
		HMailItem *item;
		if(message->FindPointer("mail",(void**)&item) == B_OK)
			fMailList->AddItem(item);
		break;
	}
	// Remove item by nodemonitor
	case M_REMOVE_FROM_NODEMONITOR:
	{
		HMailItem *item(NULL);
		bool is_delete;
		
		if(message->FindPointer("mail",(void**)&item) == B_OK)
		{
			message->FindBool("delete",&is_delete);
			fMailList->RemoveItem(item);
			if(is_delete)
				delete item;
		}
		break;
	}
	// Update preferences
	case M_PREFS_CHANGED:
	{
		if(is_kind_of(fMailView,HMailView))
		{
			cast_as(fMailView,HMailView)->ResetFont();
		}
		break;
	}
	// Show create folder dialog
	case M_CREATE_FOLDER_DIALOG:
	{
		BRect rect;
		rect = RectUtils().CenterRect(250,70);
		int32 index = fFolderList->CurrentSelection();
		HFolderItem *item=NULL;
		if(index>=0)
			item = (HFolderItem*)fFolderList->ItemAt(index);
		HCreateFolderDialog *dialog = new HCreateFolderDialog(rect,_("New Folder"),item);
		dialog->Show();
		break;
	}
	// Add to blacklist
	case M_ADD_TO_BLACK_LIST:
	{
		int32 index=0;
		int32 sel;
		while((sel = fMailList->CurrentSelection(index++)) >= 0)
			AddToBlackList(sel);
		break;
	}
	// Select sibling mails
	case M_SELECT_NEXT_MAIL:
	case M_SELECT_PREV_MAIL:
		PostMessage(message,fMailList);
		break;
	// Expand attributes
	case M_EXPAND_ATTRIBUTES:
		PostMessage(message,fDetailView);
		break;
	case M_OPEN_ATTACHMENT:
	case M_SAVE_ATTACHMENT:
		PostMessage(message,fMailView);
		break;
	// Dropped to tracker
	case 'MNOT':
	{
		BPoint drop_point = message->FindPoint("_old_drop_point_");
		BRect rect = this->Frame();
		if(rect.Contains(drop_point) == true)
			break;
		BMessage msg,reply;
		
		msg.what = B_GET_PROPERTY;
		msg.AddSpecifier("Path");
		if(message->SendReply(&msg,&reply) != B_OK)
			break;
		entry_ref ref;
		if(reply.FindRef("result",&ref) == B_OK)
		{
			BView *view = fMailView->FindView("tabview");
			/// Attachment file
			if(CurrentFocus() == view)
			{
				HAttachmentList *list = cast_as(view->FindView("attachmentlist"),HAttachmentList);
				if(!list)
					break;
				entry_ref dirRef;
				BMessage saveMsg(B_REFS_RECEIVED);
				BPath path(&ref);
				::get_ref_for_path(path.Path(),&dirRef);
				saveMsg.AddRef("directory",&dirRef);
				PostMessage(&saveMsg,view->Parent());
			}
		}	
		break;
	}
	// Update toolbar buttons
	case M_UPDATE_TOOLBUTTON:
	{
		const char* name = message->FindString("name");
		void *pointer;
		int32 sel = fMailList->CurrentSelection();
		message->FindPointer("pointer",&pointer);
		HToolbarButton *btn = static_cast<HToolbarButton*>(pointer);
		if(btn == NULL)
			break;
		if( ::strcmp(name,"Check") == 0)
		{
			if(fPopClientView->IsRunning() || !fFolderList->IsGatheredLocalFolders())
				btn->SetEnabled(false);
			else
				btn->SetEnabled(true);
		}else if( ::strcmp(name,"Send") == 0){
			if(fSmtpClientView->IsRunning())
				btn->SetEnabled(false);
			else
				btn->SetEnabled(true);
		}else if( ::strcmp(name,"Reply") == 0 ||
				::strcmp(name,"All") == 0 ||
				::strcmp(name,"Fwd") == 0||
				::strcmp(name,"Next") == 0||
				::strcmp(name,"Prev") == 0||
				::strcmp(name,"Trash") == 0 ||
				::strcmp(name,"Print") == 0){
			if(sel < 0)
				btn->SetEnabled(false);
			else
				btn->SetEnabled(true);
		}
		break;
	}
	default:
		BWindow::MessageReceived(message);
	}	
}

/***********************************************************
 * MenusBeginning
 ***********************************************************/
void
HWindow::MenusBeginning()
{
	const char* kAttr[] = {_("Subject"),_("From"),_("To"),_("When"),_("Priority"),_("Attachments"),_("Cc"),_("Size"),_("Account")};
	ColumnType attr_col[] = {COL_SUBJECT,COL_FROM,COL_TO,COL_WHEN,COL_PRIORITY,COL_ATTACHMENT,COL_CC,COL_SIZE,COL_ACCOUNT};
	BMenuItem *item;
	for(int32 i = 0;i < 9;i++)
	{
		item = KeyMenuBar()->FindItem(kAttr[i]);
		MarkMenuItem(item,fMailList->IsColumnShown(attr_col[i]));
	}
	
	bool mailSelected = (fMailList->CurrentSelection() <0)?false:true;
	
	if(is_kind_of(fMailView,HMailView))
	{
		HMailView *view = cast_as(fMailView,HMailView);

		MarkMenuItem(KeyMenuBar()->FindItem(M_HEADER),view->IsShowingHeader());
		EnableMenuItem(KeyMenuBar()->FindItem(M_HEADER),mailSelected);
		MarkMenuItem(KeyMenuBar()->FindItem(M_RAW),view->IsShowingRawMessage());
		EnableMenuItem(KeyMenuBar()->FindItem(M_RAW),mailSelected);
	}else{
		EnableMenuItem(KeyMenuBar()->FindItem(M_HEADER),false);
		MarkMenuItem(KeyMenuBar()->FindItem(M_HEADER),false);		
		EnableMenuItem(KeyMenuBar()->FindItem(M_RAW),false);
		MarkMenuItem(KeyMenuBar()->FindItem(M_RAW),false);
	}
	EnableMenuItem(KeyMenuBar()->FindItem(M_FILTER_MAIL),mailSelected);
	EnableMenuItem(KeyMenuBar()->FindItem(M_PRINT_MESSAGE),mailSelected);
	EnableMenuItem(KeyMenuBar()->FindItem(M_ADD_TO_BLACK_LIST),mailSelected);
	// Copy	
	int32 start,end;
	BTextControl *ctrl(NULL);
	if(CurrentFocus() == fMailView )
	{	
		BTextView *view(NULL);
		if(is_kind_of(fMailView,HMailView))
			view = cast_as(fMailView,BTextView);
		if(view)
		{
			view->GetSelection(&start,&end);
	
			if(start != end)
				EnableMenuItem(KeyMenuBar()->FindItem(B_COPY ),true);
			else
				EnableMenuItem(KeyMenuBar()->FindItem(B_COPY ),false);
		}
	}else if((ctrl = fDetailView->FocusedView())){
		BTextView *textview = ctrl->TextView();
		textview->GetSelection(&start,&end);
		
		if(start != end)
		{
			EnableMenuItem(KeyMenuBar()->FindItem(B_CUT ),true);
			EnableMenuItem(KeyMenuBar()->FindItem(B_COPY ),true);
		} else {
			EnableMenuItem(KeyMenuBar()->FindItem(B_CUT ),false);
			EnableMenuItem(KeyMenuBar()->FindItem(B_COPY ),false);
		}
	}
	// Disabled items
	EnableMenuItem(KeyMenuBar()->FindItem(B_UNDO ),false);
	EnableMenuItem(KeyMenuBar()->FindItem(B_CUT ),false);
	EnableMenuItem(KeyMenuBar()->FindItem(B_PASTE ),false);
	EnableMenuItem(KeyMenuBar()->FindItem(M_POP_CONNECT ),fFolderList->IsGatheredLocalFolders());
	EnableMenuItem(KeyMenuBar()->FindItem(M_CHECK_FROM ),fFolderList->IsGatheredLocalFolders());
	// Select All
	item = KeyMenuBar()->FindItem(B_SELECT_ALL);
	BView *view = CurrentFocus();
	
	if(is_kind_of(view,HMailList))
		EnableMenuItem(item,true);
	else if(is_kind_of(view,BTextView))
		EnableMenuItem(item,true);
	else if(ctrl)
		EnableMenuItem(item,true);
	else
		EnableMenuItem(item,false);
	// Rebuild check from items
	AddCheckFromItems();
}

/***********************************************************
 * CheckFrom
 ***********************************************************/
void
HWindow::CheckFrom(entry_ref &ref)
{
	// reset idle time
	fCheckIdleTime = time(NULL);
	
	if(fPopClientView->IsRunning())
		return;

	BMessage msg,sendMsg(M_POP_CONNECT);
	if(AddPopServer(ref,sendMsg) != B_OK)
		goto err;

	if(!sendMsg.IsEmpty())
		PostMessage(&sendMsg,fPopClientView);
	return;
err:
	(new BAlert("",_("Account file is corrupted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
}
/***********************************************************
 * PopConnect
 ***********************************************************/
void
HWindow::PopConnect()
{
	// reset idle time
	fCheckIdleTime = time(NULL);
	
	if(fPopClientView->IsRunning())
		return;
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	if( path.InitCheck() != B_OK)
		return;
		
	BDirectory dir(path.Path());
	status_t err = B_OK;
	BEntry entry;
	BMessage sendMsg(M_POP_CONNECT);
	
	sendMsg.MakeEmpty();
	
	while( err == B_OK )
	{
		err = dir.GetNextEntry(&entry,false);	
		if( entry.InitCheck() != B_NO_ERROR )
			break;
		if( entry.GetPath(&path) != B_NO_ERROR )
			break;
		else if(entry.IsDirectory())
			continue;
		else{
			entry_ref ref;
			entry.GetRef(&ref);
			AddPopServer(ref,sendMsg);
		}
	}
	if(!sendMsg.IsEmpty())
		PostMessage(&sendMsg,fPopClientView);
	return;
}

/***********************************************************
 * AddPopServer
 ***********************************************************/
status_t
HWindow::AddPopServer(entry_ref& ref,BMessage &sendMsg)
{
	BFile file;
	BMessage msg;
	BEntry entry(&ref);
	
	char name[B_FILE_NAME_LENGTH+1];
	entry.GetName(name);
	PRINT(("Name:%s\n",name));
	if(file.SetTo(&entry,B_READ_ONLY) != B_OK)
		return B_ERROR;
	msg.Unflatten(&file);
	const char* host,*login,*password,*port;
	int32 iValue;
		
	if(msg.FindString("pop_host",&host) != B_OK)
		return B_ERROR;
			
	if(msg.FindString("pop_port",&port) != B_OK)
		return B_ERROR;
			
	if(msg.FindString("pop_user",&login) != B_OK)
		return B_ERROR;
			
	int16 proto;
	if(msg.FindInt16("protocol_type",&proto) != B_OK)
		return B_ERROR;
	if(proto == 2) // if protocol type is IMAP4, we need to skip.
		return B_NOT_ALLOWED;
		
	int16 retrieve;
	if(msg.FindInt16("retrieve",&retrieve) != B_OK)
		retrieve = 0;
			
	iValue = 0;
	if(retrieve == 2)
		msg.FindInt32("delete_day",&iValue);
	
	BString uidl("");
	if(msg.FindString("uidl",&uidl) != B_OK)	
		uidl= "";
		
	PRINT(("SAVED UIDL:%s\n",uidl.String()));
		
	if(msg.FindString("pop_password",&password) != B_OK)
		return B_ERROR;
	int32 len = strlen(password);
	char* pass = new char[len+1];
	for(int32 k = 0;k < len;k++)
		pass[k] =(char)(255-password[k]);
	pass[len] = '\0';
	sendMsg.AddString("name",name);
	sendMsg.AddString("address",host);
	sendMsg.AddInt16("port",atoi(port));
	sendMsg.AddString("login",login);
	sendMsg.AddInt16("retrieve",retrieve);
	sendMsg.AddInt16("protocol_type",proto);		
	sendMsg.AddInt32("delete_day",iValue);
	sendMsg.AddString("uidl",uidl);
	
	sendMsg.AddString("password",pass);
	delete[] pass;
	return B_OK;
}

/***********************************************************
 * FolderSelection
 ***********************************************************/
int32
HWindow::FolderSelection()
{
	return fFolderList->CurrentSelection();
}

/***********************************************************
 * InvokeMailItem
 ***********************************************************/
void
HWindow::InvokeMailItem()
{
	int32 sel = fMailList->CurrentSelection();
	
	if(sel < 0)
		return;
	HMailItem *item = fMailList->MailAt(sel);
	entry_ref ref = item->Ref();

	MakeReadWindow(ref,new BMessenger(fMailList,this));
}

/***********************************************************
 * DeleteMails
 ***********************************************************/
void
HWindow::DeleteMails()
{
	int32 sel = fFolderList->CurrentSelection();
	if(sel < 0)
		return;
	HFolderItem *from = cast_as(fFolderList->ItemAt(sel),HFolderItem);
	// Local mails
	if(from->FolderType() != IMAP4_TYPE)
	{
	
		BPath path;
		::find_directory(B_USER_DIRECTORY, &path, true);
		path.Append("mail");
		path.Append( TRASH_FOLDER );
	
		entry_ref ref;
		::get_ref_for_path(path.Path(),&ref);
	
		HFolderItem *to(NULL);
	
		int32 folder = fFolderList->FindFolder(ref);
		
		if(folder < 0) // Could not find trash folder
			fFolderList->AddItem((to = new HFolderItem(ref,fFolderList)));
		else
			to = cast_as(fFolderList->ItemAt(folder),HFolderItem);
		
		// If from and to folder are same, skip moving
		if(from == to) 
			return;
		
		BMessage msg(M_MOVE_MAIL);
		msg.AddPointer("from",from);
		msg.AddPointer("to",to);
	
		int32 selected; 
		int32 last_selected = -1;
		int32 sel_index = 0;
		
		HMailItem *mail(NULL);
		while((selected = fMailList->CurrentSelection(sel_index++)) >= 0)
		{
			mail=fMailList->MailAt(selected);
			if(!mail)
				continue;
			msg.AddPointer("mail",mail);
			msg.AddRef("refs",&mail->fRef);
			last_selected = selected;
		}
		// Select the next mail
		//if(last_selected >= 0)
		//	fMailList->Select(last_selected+1);
		
		PostMessage(&msg);
	}else{
		// IMAP4 mails
		int32 selected; 
		int32 unread_mails = 0;
		HIMAP4Item *mail(NULL);
		
		while((selected = fMailList->CurrentSelection(0)) >= 0)
		{
			mail=cast_as(fMailList->RemoveItem(selected),HIMAP4Item);
			if(!mail)
				continue;
			mail->Delete();
			if(!mail->IsRead())
				unread_mails++;
			from->RemoveMail( mail );
			delete mail;
		}
		
	}
}


/***********************************************************
 * AddToPeople
 ***********************************************************/
void
HWindow::AddToPeople()
{
	BString from("");	
	int32 sel;
	int32 i = 0;
	entry_ref ref,people_ref;
	if(be_roster->FindApp("application/x-vnd.Be-PEPL",&people_ref) != B_OK)
		return;
	
	while((sel = fMailList->CurrentSelection(i++)) >= 0)
	{
		HMailItem *item = fMailList->MailAt(sel);
		if(item)
		{
			ref = item->Ref();
			BNode node(&ref);
			if(node.InitCheck() != B_OK)
				continue;
			ReadNodeAttrString(&node,B_MAIL_ATTR_FROM,&from);
			
			int32 index = from.FindFirst("<");
			BString name("");
			if(index != B_ERROR)
			{
				if(from[0] == '\"')
				{
					int32 j = 1;
					while(from[j] != '\"')
						name << (char)from[j++];
				}else
					from.CopyInto(name,0,index);
				
				name.Insert("META:name ",0);
				
				index++;
				BString tmp("");
				while(from[index] != '>')
					tmp << from[index++];
				from = tmp;
			}
			from.Insert("META:email ",0);
			PRINT(("Name:%s\n",name.String() ));
			PRINT(("From:%s\n",from.String() ));
			
			int32 argc = 0;
			char *argv[3];
			argv[argc] = new char[from.Length()+1];
			::strcpy(argv[argc++],from.String() );
			if(name.Length() != 0)
			{
				argv[argc] = new char[name.Length()+1];	
				::strcpy(argv[argc++],name.String() );
			}
			argv[argc++] = NULL;
			
			be_roster->Launch(&people_ref,argc-1,argv);
			
			for(int32 k = 0;k < argc;k++)
			{
				delete[] argv[k];
			}
		}
	}
}

/***********************************************************
 * ForwardMail
 ***********************************************************/
void
HWindow::ForwardMail(HMailItem *item)
{
	entry_ref ref = item->Ref();
	BFile file(&ref,B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	BString subject("");
	ReadNodeAttrString(&file,B_MAIL_ATTR_SUBJECT,&subject);
	
	subject.Insert("Fwd:",0);
	BRect rect;
	((HApp*)be_app)->Prefs()->GetData("write_window_rect",&rect);
	MakeWriteWindow(subject.String()
					,NULL
					,NULL
					,NULL
					,NULL
					,NULL
					,item
					,false
					,true);
}

/***********************************************************
 * Send pending mails
 ***********************************************************/
void
HWindow::SendPendingMails()
{
	// get out directory
	BPath path;
	::find_directory(B_USER_DIRECTORY,&path);
	path.Append("mail");
	path.Append("out");
	
	if(path.InitCheck() != B_OK)
		return;
	
	entry_ref ref;
	::get_ref_for_path(path.Path(),&ref);
	
	int32 folder_index = fFolderList->FindFolder(ref);
	if(folder_index < 0)
		return;
	HFolderItem *folder = cast_as(fFolderList->ItemAt(folder_index),HFolderItem);
	if(!folder)
		return;
	BList *maillist = folder->MailList();
	
	int32 count = maillist->CountItems();
	BMessage msg(M_SEND_MAIL);
	for(int32 i = 0;i < count;i++)
	{
		HMailItem *item = static_cast<HMailItem*>(maillist->ItemAt(i));
		if(!item)
			continue;
		item->RefreshStatus();
		if(item->fStatus.Compare("Pending") == 0)
			msg.AddPointer("pointer",item);
	}
	if(!msg.IsEmpty())
		PostMessage(&msg,fSmtpClientView);
}

/***********************************************************
 * ReplyMail
 ***********************************************************/
void
HWindow::ReplyMail(HMailItem *item,bool reply_all)
{
	entry_ref ref = item->Ref();
	BFile file(&ref,B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	BString to(""),subject(""),reply(""),cc("");
	ReadNodeAttrString(&file,B_MAIL_ATTR_FROM,&to);
	ReadNodeAttrString(&file,B_MAIL_ATTR_CC,&cc);
	ReadNodeAttrString(&file,B_MAIL_ATTR_SUBJECT,&subject);
	ReadNodeAttrString(&file,B_MAIL_ATTR_REPLY,&reply);
	
	if(reply.Length() > 0)
		to = reply;
	
	if(reply_all)
	{
		to << "," <<cc;	
	}
	// Eliminate mail list title
	if(subject.Length() > 2 && subject[0] == '[')
	{
		int32 index = subject.FindFirst(']',1);
		if(index != B_ERROR)
		{
			if(subject[index+1] == ' ')
				index++;
			subject = &subject[index+1];
		}
	}
	//
	if( strncmp(subject.String(),"Re:",3) != 0)
	{
		if(subject != "" && subject[0] == ' ')
			subject.Insert("Re:",0);
		else
			subject.Insert("Re: ",0);
	}
	MakeWriteWindow(subject.String()
					,to.String()
					,NULL
					,NULL
					,NULL
					,NULL
					,item
					,true);					
}

/***********************************************************
 * DispatchMessage
 ***********************************************************/
void
HWindow::DispatchMessage(BMessage *message,BHandler *handler)
{
	HPrefs *prefs = ((HApp*)be_app)->Prefs();
	bool auto_check;
	int32 minutes;
	prefs->GetData("auto_check",&auto_check);
	prefs->GetData("check_minutes",&minutes);
	
	if(message->what == B_PULSE && auto_check)
	{
		time_t now = time(NULL);
		float diff = difftime(now,fCheckIdleTime);
		//PRINT(("Diff:%f\n",diff));
		if(diff > minutes*60)
			PostMessage(M_POP_CONNECT);
	}
	
	if(message->what == B_KEY_DOWN&& (handler == fMailList || handler == fMailView))
	{
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
				PostMessage(M_SELECT_PREV_MAIL,fMailList);
			else
				PostMessage(M_SELECT_NEXT_MAIL,fMailList);
		}
	}
	/// Stop LED Animation
	if(fLEDAnimation->IsRunning() && 
		(message->what == B_KEY_DOWN||message->what == B_MOUSE_DOWN))
	{
		fLEDAnimation->Stop();
	}
	
	BWindow::DispatchMessage(message,handler);
}

/***********************************************************
 * InstallToDeskbar
 ***********************************************************/
void
HWindow::InstallToDeskbar()
{
	BDeskbar deskbar;

	if(deskbar.HasItem( APP_NAME ) == false)
	{
		BRoster roster;
		entry_ref ref;
		roster.FindApp( APP_SIG , &ref);
		int32 id;
		deskbar.AddItem(&ref, &id);
	}
}

/***********************************************************
 * MakeWriteWindow
 ***********************************************************/
void
HWindow::MakeWriteWindow(const char* subject,
						const char* to,
						const char* cc,
						const char* bcc,
						const char* body,
						const char* enclosure,
						HMailItem *replyItem,
						bool reply,
						bool forward)
{
	BRect rect;
	((HApp*)be_app)->Prefs()->GetData("write_window_rect",&rect);
	HWriteWindow *win = new HWriteWindow(rect,_("New Message")
							,subject
							,to
							,cc
							,bcc
							,body
							,enclosure
							,replyItem
							,reply
							,forward);
	win->Show();
}

/***********************************************************
 * RefsReceived
 ***********************************************************/
void
HWindow::RefsReceived(BMessage *message)
{
	BAutolock lock(this);	
	entry_ref ref,folder_ref;
	BMessenger messenger,*newMessenger;
	if(message->FindRef("refs",&ref) != B_OK)
		return;

	message->FindMessenger("TrackerViewToken", &messenger);
	newMessenger= new BMessenger(messenger);
	MakeReadWindow(ref,newMessenger);
}

/***********************************************************
 * MakeReadWindow
 ***********************************************************/
void
HWindow::MakeReadWindow(entry_ref ref,BMessenger *messenger)
{
	// Check mail status
	BString status;
	BNode node(&ref);
	BRect rect;

	ReadNodeAttrString(&node,B_MAIL_ATTR_STATUS,&status);
	
	if(status.Compare("Sent")==0 ||
		 status.Compare("Pending") == 0||
		  status.Compare("Forwarded") == 0||
		   status.Compare("Error") == 0)
	{
		int32 index = (new BAlert("",_("Would you like to (re)edit this message?"),_("Yes"),_("No"),NULL,B_WIDTH_AS_USUAL,B_IDEA_ALERT))->Go();
		// Re edit mail
		if(index == 0)
		{
			((HApp*)be_app)->Prefs()->GetData("write_window_rect",&rect);
			HWriteWindow *win = new HWriteWindow(rect,_("New Message"),ref);
			win->Show();
			return;
		}
	}
	
	((HApp*)be_app)->Prefs()->GetData("read_win_rect",&rect);
	HReadWindow *win = new HReadWindow(rect,ref,messenger);
	win->Show();
}

/***********************************************************
 * RemoveFromDeskbar
 ***********************************************************/
void
HWindow::RemoveFromDeskbar()
{
	BDeskbar deskbar;
	if( deskbar.HasItem( APP_NAME ))
		deskbar.RemoveItem( APP_NAME );
}

/***********************************************************
 * EmptyTrash
 ***********************************************************/
void
HWindow::EmptyTrash()
{
	int32 count = fFolderList->CountItems();
	HFolderItem *trash(NULL);
		
	for(int32 i = 0;i < count;i++)
	{
		HFolderItem *item = cast_as(fFolderList->ItemAt(i),HFolderItem);
		if(item && ::strcmp(item->FolderName(), TRASH_FOLDER ) == 0)
			trash = item;
	}
	if(!trash)
	{
		(new BAlert("",_("Could not find Trash folder"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}
		
	entry_ref ref = trash->Ref();
	BDirectory dir( &ref );
   	status_t err = B_NO_ERROR;
	BEntry entry;
	BPath path(&ref);
	
	BMessage msg(M_MOVE_FILE);
	
	BPath desktopTrash;
	::find_directory(B_USER_DIRECTORY,&desktopTrash);
	desktopTrash.Append("Desktop");
	desktopTrash.Append("Trash");
	
	if(!desktopTrash.Path())
	{
		(new BAlert("",_("Could not find Trash folder."),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}
	
	while( err == B_OK )
	{
		if( (err = dir.GetNextRef( &ref )) == B_OK)
		{
			if(entry.SetTo(&ref) != B_OK)
				continue;
			if(entry.Exists())
			{
				msg.AddRef("refs",&ref);
				msg.AddString("path",desktopTrash.Path());
			}
		}
	}
	
	if(!msg.IsEmpty())
		be_app->PostMessage(&msg);
}

/***********************************************************
 * AddCheckFromItems
 ***********************************************************/
void
HWindow::AddCheckFromItems()
{
	BMenu *subMenu = KeyMenuBar()->SubmenuAt(2);
	if(!subMenu) return;
	subMenu = subMenu->SubmenuAt(1);
	if(!subMenu) return;
	// Delete all items	
	int32 count = subMenu->CountItems();
	while(count>0)
		delete subMenu->RemoveItem(--count);
	// Add items
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	BDirectory dir(path.Path());
		
	BEntry entry;
	status_t err = B_OK;
	
	while(err == B_OK)
	{
		if((err = dir.GetNextEntry(&entry)) == B_OK )
		{
			if(entry.IsDirectory())
				continue;
			char name[B_FILE_NAME_LENGTH+1];
			entry.GetName(name);
			entry_ref ref;
			entry.GetRef(&ref);
			if(!fFolderList->IsIMAP4Account(ref))
			{
				BMessage *msg = new BMessage(M_CHECK_FROM);
				msg->AddRef("refs",&ref);
				subMenu->AddItem(new BMenuItem(name,msg));
			}
		}
	}
}

/***********************************************************
 * DeleteFolder
 ***********************************************************/
void
HWindow::DeleteFolder(int32 sel)
{
	HFolderItem *item = cast_as(fFolderList->RemoveFolder(sel),HFolderItem);
	if(!item)
		return;
	item->DeleteMe();
	delete item;
}

/***********************************************************
 * MoveMails
 ***********************************************************/
void
HWindow::MoveMails(BMessage *message)
{
	entry_ref ref;
	int32 count;
	type_code type;
	HMailItem *mail;
	int32 i;
		
	message->GetInfo("mail",&type,&count);
	if(count == 0)
		return;
	HFolderItem *to;
	HFolderItem *from;
	message->FindPointer("to",(void**)&to);
	message->FindPointer("from",(void**)&from);
	
	ref = to->Ref();
	BPath path(&ref);
	
	int32 old_selection=-1;
	
	fMailList->DeselectAll();
	
	BMessage msg(M_MOVE_FILE);
	
	for(i = 0;i < count;i++)
	{			
		message->FindPointer("mail",i,(void**)&mail);
		old_selection = fMailList->IndexOf(mail);
		ref = mail->Ref();
		msg.AddRef("refs",&ref);
		msg.AddString("path",path.Path());
		BPath item_path(path.Path());
		item_path.Append(BPath(&ref).Leaf());
		::get_ref_for_path(item_path.Path(),&ref);
	}
	
	if(old_selection >= 0)
		fMailList->Select( old_selection + 1 );
	be_app->PostMessage(&msg);
}

/***********************************************************
 * FilterMail
 ***********************************************************/
void
HWindow::FilterMails(HMailItem *item)
{
	if(!item)
		return;
	BString out_path;
	fPopClientView->FilterMail(item->fSubject.String(),
								item->fFrom.String(),
								item->fTo.String(),
								item->fCC.String(),
								item->fReply.String(),
								item->fAccount.String(),
								out_path);
	PRINT(("To:%s\n",out_path.String()));
	if(BPath(out_path.String()).InitCheck() != B_OK)
		return;

	BPath default_path,current_path;
	::find_directory(B_USER_DIRECTORY,&default_path);
	default_path.Append("mail");
	default_path.Append("in");
	entry_ref ref = item->Ref();
	current_path.SetTo(&ref);
	current_path.GetParent(&current_path);
	
	if(out_path.Compare(current_path.Path()) == 0 ||
		out_path.Compare(default_path.Path()) == 0)
		return;
	
	BMessage msg(M_MOVE_FILE);
	msg.AddRef("refs",&ref);
	msg.AddString("path",out_path.String());
	be_app->PostMessage(&msg);
}

/***********************************************************
 * PrintMessage
 ***********************************************************/
void
HWindow::PrintMessage(BMessage *message)
{
	int32 sel = fMailList->CurrentSelection();
	if(sel < 0)
		return;
	HMailItem *item = fMailList->MailAt(sel);
	BMessage msg(*message);
	
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
	msg.AddString("job_name",item->fSubject.String());
	be_app->PostMessage(&msg);
}

/***********************************************************
 * Plain2BeMail
 *	Convert plain text mails to BeOS format ones
 *	and filter them.
 ***********************************************************/
void
HWindow::Plain2BeMail(const char* path)
{
	BFile input(path,B_READ_ONLY);
	if(input.InitCheck() != B_OK)
		return;
	BString str;
	str << input;
	entry_ref folder_ref,file_ref;
	bool del;
	fPopClientView->SaveMail(str.String(),&folder_ref,&del);
}

/***********************************************************
 * MBox2BeMail
 ***********************************************************/
void
HWindow::MBox2BeMail(const char* path)
{
	BFile file(path,B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	
	off_t size;
	file.GetSize(&size);
	
	entry_ref folder_ref,file_ref;
	bool del;
	BString msgStr;
	BString line;
	while(1)
	{
		if(size == file.Position() )
			break;
		ReadLine(&file,&line);
		if(line.Compare("From ",5) == 0)
		{
			msgStr = line;
			ReadLine(&file,&line);
			if(IsHeaderLine(line.String()))
			{
				if(line.Compare("Return-Path: ",13) == 0)
					msgStr = line;
				else{
					msgStr = &msgStr[5];
					msgStr.Insert("Return-Path: ",0);
					msgStr += line;
				}
				
				while(1)
				{
					ReadLine(&file,&line);
					msgStr += line;
					if(line.Compare("From ",5) == 0)
					{
						ReadLine(&file,&line);
						if(IsHeaderLine(line.String()))
						{
							file.Seek(-line.Length(),SEEK_CUR);
							break;
						}else
							msgStr += line;
					}
					if(size == file.Position() )
						break;	
				}
				UpdateIfNeeded();
				fPopClientView->SaveMail(msgStr.String(),&folder_ref,&del);
			}
		}
	}
}

/***********************************************************
 * ShowOpenPanel
 ***********************************************************/
void
HWindow::ShowOpenPanel(int32 what)
{
	if(!fOpenPanel)
	{
		fOpenPanel = new BFilePanel();
		fOpenPanel->SetTarget(this);
	}
	BMessage msg(what);
	fOpenPanel->SetMessage(&msg);
	
	fOpenPanel->Show();
}

/***********************************************************
 * AddToBlackList
 ***********************************************************/
void
HWindow::AddToBlackList(int32 index)
{
	HMailItem *item = fMailList->MailAt(index);
	if(!item)
		return;
	BString from;
	const char* p = item->fFrom.String();
	int32 len = ::strlen(p);
	
	for(int32 i = 0;i < len;i++)
	{
		if(*p == '<')
		{
			p++;
			if(*p != ' ')
				from.SetTo(*p,1);
		}else{
			if(*p != ' ')
				from += *p;
		}
		p++;
		if(*p == '>')
			break;
		if(*p == ' ' && from.FindFirst("@") != B_ERROR)
				break;
	}
	if(from.FindFirst("@") == B_ERROR)
		return;
	// Load list
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append( APP_NAME );
	path.Append("BlackList");
	
	
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_OPEN_AT_END);
	if(file.InitCheck() != B_OK)
		return;
	file.Write("\n",1);
	file.Write(from.String(),from.Length());
}

/***********************************************************
 * PlayLEDAnimaiton()
 ***********************************************************/
void
HWindow::PlayLEDAnimaiton()
{
	bool enabled;
	((HApp*)be_app)->Prefs()->GetData("led_blink",&enabled);
	if(enabled)
		fLEDAnimation->Start();
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HWindow::QuitRequested()
{	
	// To avoid corrupting folder structure caches, wait for gathering folders.
	if(!fFolderList->IsGatheredLocalFolders())
		return false;
		
	if(fPopClientView->IsRunning())
	{
		int32 btn = (new BAlert("",_("POP3 session is running"),_("Force Quit"),_("Wait"),NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		if(btn == 0)
			fPopClientView->Cancel();
		else
			return false;
	}
	if(fSmtpClientView->IsRunning())
	{
		int32 btn = (new BAlert("",_("SMTP session is running"),_("Force Quit"),_("Wait"),NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		if(btn == 0)
			fSmtpClientView->Cancel();
		else
			return false;
	}
	Hide();
	if(is_kind_of(fMailView,HMailView))
	{
		HMailView *view = cast_as(fMailView,HMailView);
		Unlock();
		view->StopLoad();
		Lock();
	}
	fMailList->MarkOldSelectionAsRead();
	RemoveFromDeskbar();
	// Save Window Rect
	((HApp*)be_app)->Prefs()->SetData("window_rect",Frame());
	// Save Splitter pos
	BPoint pos;
	pos = fHSplitter->GetBarPosition();
	((HApp*)be_app)->Prefs()->SetData("horizontalsplit",pos);
	pos = fVSplitter->GetBarPosition();
	((HApp*)be_app)->Prefs()->SetData("verticalsplit",pos);
	// Save col size
	fMailList->SaveColumnsAndPos();
	fMailList->MakeEmpty();
	// Delete all folder items
	fFolderList->SaveFolderStructure();
	fFolderList->DeleteAll();
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}

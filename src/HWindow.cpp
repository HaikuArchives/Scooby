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
#include "SmtpClient.h"
#include "HToolbarButton.h"
#include "OpenWithMenu.h"
#include "HReadWindow.h"

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
				const char* name,
				const char* mail_addr)
	:BWindow(rect,name,B_DOCUMENT_WINDOW,B_ASYNCHRONOUS_CONTROLS)
	,fCheckIdleTime(time(NULL))
{
	SetPulseRate(100000);
	
	if(mail_addr)
		MakeWriteWindow(NULL,mail_addr);
	InitMenu();
	InitGUI();
	PostMessage(M_GET_VOLUMES,fFolderList);
	
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
}

/***********************************************************
 * Destructor
 ***********************************************************/
HWindow::~HWindow()
{

}

/***********************************************************
 * Menuの作成
 ***********************************************************/
void
HWindow::InitMenu()
{
	BMenuBar *menubar = new BMenuBar(Bounds(),"");
    BMenu		*aMenu;
	MenuUtils utils;
	BMessage *msg;
	ResourceUtils rsrc_utils;
	BString label;
//// ------------------------ File Menu ----------------------    
	aMenu = new BMenu(_("File"));
	utils.AddMenuItem(aMenu,_("Open Query Folder"),M_OPEN_QUERY,this,this,0,0,
							rsrc_utils.GetBitmapResource('BBMP',"OpenQuery"));
	utils.AddMenuItem(aMenu,_("Empty Trash"),M_EMPTY_TRASH,this,this,'T',B_SHIFT_KEY,
							rsrc_utils.GetBitmapResource('BBMP',"Trash"));
	aMenu->AddSeparatorItem();
	
	utils.AddMenuItem(aMenu,_("Print Message"),M_PRINT_MESSAGE,this,this,'P',0,
							rsrc_utils.GetBitmapResource('BBMP',"Printer"));
	utils.AddMenuItem(aMenu,_("Page Setup…"),M_PAGE_SETUP_MESSAGE,be_app,be_app,0,0,
							rsrc_utils.GetBitmapResource('BBMP',"PageSetup"));
	aMenu->AddSeparatorItem();
	label = _("Preferences");
	label << "…";
	utils.AddMenuItem(aMenu,label.String(),M_PREF_MSG,this,this);
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("About Scooby…"),B_ABOUT_REQUESTED,be_app,be_app);
	aMenu->AddSeparatorItem();
	utils.AddMenuItem(aMenu,_("Quit"),B_QUIT_REQUESTED,this,this,'Q',0);
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
////------------------------- Mail Menu ---------------------
	aMenu = new BMenu(_("Mail"));
   	utils.AddMenuItem(aMenu,_("Check Mail"),M_POP_CONNECT,this,this,'M',0,
   							rsrc_utils.GetBitmapResource('BBMP',"Check Mail"));
	// account
	BMenu *subMenu = new BMenu(_("Check Mail From"));

	aMenu->AddItem(subMenu);
	aMenu->AddSeparatorItem();
	//
	
	utils.AddMenuItem(aMenu,_("Send Peding Messages"),M_SEND_PENDING_MAILS,this,this,0,0,
							rsrc_utils.GetBitmapResource('BBMP',"Send"));
	menubar->AddItem( aMenu );
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
	utils.AddMenuItem(aMenu,_("Move To Trash"),M_DELETE_MSG,this,this,'T',0,
							rsrc_utils.GetBitmapResource('BBMP',"Trash"));
    aMenu->AddSeparatorItem();
    utils.AddMenuItem(aMenu,_("Show Header"),M_HEADER,this,this,'H',0);
    utils.AddMenuItem(aMenu,_("Show Raw Message"),M_RAW,this,this,0,0);
    menubar->AddItem( aMenu );
////------------------------- Attr Menu ---------------------
	aMenu = new BMenu(_("Attributes"));
	const char* kAttr[] = {_("Subject"),_("From"),_("To"),_("When"),_("Priority"),_("Attachment")};
	ColumnType attr_col[] = {COL_SUBJECT,COL_FROM,COL_TO,COL_WHEN,COL_PRIORITY,COL_ATTACHMENT};
	for(int32 i = 0;i < 6;i++)
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
/********** Captionの追加 ***********/
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
	entryrect.top = entryrect.bottom+1;
	entryrect.bottom = subview->Bounds().bottom;
	entryrect.right -= B_V_SCROLL_BAR_WIDTH;
	fMailView = new HMailView(entryrect,true,NULL);
	fMailView->MakeEditable(false);
	BScrollView *scroll = new BScrollView("scroller",fMailView,B_FOLLOW_ALL,0,false,true);
	subview->AddChild(scroll);
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
	//toolbox->AddButton("Send",utils.GetBitmapResource('BBMP',"Send"),new BMessage(M_SEND_PENDING_MAILS),"Send Pending Mails");
	toolbox->AddSpace();
	toolbox->AddButton("New",utils.GetBitmapResource('BBMP',"New Message"),new BMessage(M_NEW_MSG),_("New Message"));
	BMessage *msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",false);
	toolbox->AddButton("Reply",utils.GetBitmapResource('BBMP',"Reply"),msg,"Reply Message");
	msg = new BMessage(M_REPLY_MESSAGE);
	msg->AddBool("reply_all",true);
	toolbox->AddButton("All",utils.GetBitmapResource('BBMP',"Reply To All"),msg,_("Reply To All"));
	toolbox->AddButton("Fwd",utils.GetBitmapResource('BBMP',"Forward"),new BMessage(M_FORWARD_MESSAGE),_("Forward Message"));
	toolbox->AddSpace();
	toolbox->AddButton("Trash",utils.GetBitmapResource('BBMP',"Trash"),new BMessage(M_DELETE_MSG),_("Move To Trash"));
	
	toolbox->AddSpace();
	toolbox->AddButton("Next",utils.GetBitmapResource('BBMP',"Next"),new BMessage(M_SELECT_NEXT_MAIL),_("Next Message"));
	toolbox->AddButton("Prev",utils.GetBitmapResource('BBMP',"Prev"),new BMessage(M_SELECT_PREV_MAIL),_("Prev Message"));
	
	AddChild(toolbox);
	
	fFolderList->WatchQueryFolder();
	fMailList->MakeFocus(true);	
	/*
	KeyMenuBar()->FindItem(B_CUT)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_COPY)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_PASTE)->SetTarget(fMailView,this);
	KeyMenuBar()->FindItem(B_SELECT_ALL)->SetTarget(this);
	*/
	
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
	// Select All
	case B_COPY:
	case B_SELECT_ALL:
	{
		BView *view = CurrentFocus();
		if(view)
			PostMessage(message,view);
		break;
	}
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
			entry_ref ref = mail->Ref();
			BPath path(&ref);
			path.GetParent(&path);
			entry_ref folder_ref;
			::get_ref_for_path(path.Path(),&folder_ref);
			int32 fol = fFolderList->FindFolder(folder_ref);
			
			HFolderItem *item(NULL);
			if(fol < 0)
				goto send;
			item = cast_as(fFolderList->ItemAt(fol),HFolderItem);
			if(!item)
				goto send;
		
			item->AddMail(mail);
			if(fol == fFolderList->CurrentSelection())
			{
				fMailList->AddItem(mail);
				fMailList->SortItems();
			}
send:
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
	// Receive new  mail
	case M_RECEIVE_MAIL:
	{
		entry_ref folder_ref,file_ref;
		if(message->FindRef("folder_ref",&folder_ref) == B_OK &&
			message->FindRef("file_ref",&file_ref) == B_OK)
		{
			int32 index = fFolderList->FindFolder(folder_ref);
			if(index <0)
				break;
	
			HFolderItem *item = cast_as(fFolderList->ItemAt(index),HFolderItem);
			if(!item)
				break;
			
			HMailItem *mail = new HMailItem(file_ref);
			item->AddMail( mail );
			item->SetName(item->Unread()+1);
			if(index == fFolderList->CurrentSelection())
			{
				fMailList->AddMail(mail);
				//fMailList->AddItem(mail);
				//fMailList->SortItems();
			}
			fFolderList->InvalidateItem(index);
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
		fMailList->MakeEmpty();
		fDetailView->SetInfo("","","");
		PostMessage(M_SET_CONTENT,fMailView);
		PostMessage(message,fMailList);
		fMailCaption->StopBarberPole();
		break;
	}
	// delete mail
	case M_DELETE_MSG:
		DeleteMails();
		break;
	case M_PRINT_MESSAGE:
	{
		int32 sel = fMailList->CurrentSelection();
		if(sel < 0)
			break;
		HMailItem *item = cast_as(fMailList->ItemAt(sel),HMailItem);
		BMessage msg(*message);
		msg.AddPointer("view",fMailView);
		msg.AddString("job_name",item->fSubject.String());
		be_app->PostMessage(&msg);
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
		if(item)
			item->Launch();
		break;
	}
	// Set mail content
	case M_SET_CONTENT:
	{
		//PostMessage(message,fMailView);
		entry_ref ref;
		if(message->FindRef("refs",&ref) != B_OK)
		{
			fMailView->SetContent(NULL);
			break;
		}
		// this file pointer will be deleted at MailView
		BFile *file = new BFile(&ref,B_READ_ONLY);
		if(file->InitCheck() != B_OK)
		{
			delete file;
			fMailView->SetContent(NULL);
			break;
		}
		fMailView->SetContent(file);
		// set header view
		const char* kSubject,*kFrom,*kDate;
	
		if(message->FindString("subject",&kSubject) == B_OK &&
			message->FindString("from",&kFrom) == B_OK &&
			message->FindString("when",&kDate) == B_OK)
			fDetailView->SetInfo(kSubject,kFrom,kDate);
		else
			fDetailView->SetInfo("","","");
		
		// recalc unread mails
		bool read;
		if(message->FindBool("read",&read) == B_OK)
		{
			if(read)
				break;
			int32 sel = fFolderList->CurrentSelection();
			if(sel <0)
				break;
			HFolderItem *item = cast_as(fFolderList->ItemAt(sel),HFolderItem);
			if(item)
			{
				item->SetName(item->Unread()-1);
				fFolderList->InvalidateItem(sel);
			}
		}
		break;
	}
	case M_PREF_MSG:
	{
		HPrefWindow *win = new HPrefWindow(RectUtils().CenterRect(600,340));
		win->Show();
		break;
	}
	case M_ATTR_MSG:
	{
		const char* kAttr[] = {_("Subject"),_("From"),_("To"),_("When"),_("Priority"),_("Attachment")};

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
			HMailItem *item = cast_as(fMailList->ItemAt(sel),HMailItem);
			if(item)
			{
				ref = item->Ref();
				msg.AddRef("refs",&ref);
			}
		}
		if(!msg.IsEmpty())
			be_roster->Launch(app_sig,&msg);
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
			item = cast_as(fMailList->ItemAt(sel),HMailItem);
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
			item = cast_as(fMailList->ItemAt(sel),HMailItem);
		}
		ForwardMail(item);
		break;
	}
	// Show header 
	case M_HEADER:
		message->AddBool("header",!fMailView->IsShowingHeader());
		PostMessage(message,fMailView);
		break;
	case M_RAW:
		message->AddBool("raw",!fMailView->IsShowingRawMessage());
		PostMessage(message,fMailView);
		break;
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
		PRINT(("dafd\n"));
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
		fMailView->ResetFont();
		break;
	}
	
	case M_SELECT_NEXT_MAIL:
	case M_SELECT_PREV_MAIL:
		PostMessage(message,fMailList);
		break;
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
			if(fPopClientView->IsRunning())
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
				::strcmp(name,"Trash") == 0){
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
	const char* kAttr[] = {_("Subject"),_("From"),_("To"),_("When"),_("Priority"),_("Attachment")};
	ColumnType attr_col[] = {COL_SUBJECT,COL_FROM,COL_TO,COL_WHEN,COL_PRIORITY,COL_ATTACHMENT};
	BMenuItem *item;
	for(int32 i = 0;i < 6;i++)
	{
		item = KeyMenuBar()->FindItem(kAttr[i]);
		if(item) 
			item->SetMarked(fMailList->IsColumnShown(attr_col[i]));
	}
	
	bool mailSelected = (fMailList->CurrentSelection() <0)?false:true;
	
	item = KeyMenuBar()->FindItem(M_HEADER);
	item->SetMarked(fMailView->IsShowingHeader());
	item->SetEnabled(mailSelected);
	item = KeyMenuBar()->FindItem(M_RAW);
	item->SetMarked(fMailView->IsShowingRawMessage());
	item->SetEnabled(mailSelected);
	
	// Copy	
	int32 start,end;
	BTextControl *ctrl(NULL);
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
	item = KeyMenuBar()->FindItem(B_SELECT_ALL);
	BView *view = CurrentFocus();
	
	if(is_kind_of(view,HMailList))
		item->SetEnabled(true);
	else if(is_kind_of(view,BTextView))
		item->SetEnabled(true);
	else if(ctrl)
		item->SetEnabled(true);
	else
		item->SetEnabled(false);
	// Recreate check from items
	// Delete all items
	BMenu *subMenu = KeyMenuBar()->SubmenuAt(2);
	subMenu = subMenu->SubmenuAt(1);
	int32 count = subMenu->CountItems();
	while(count>0)
		delete subMenu->RemoveItem(--count);
	AddCheckFromItems();
	//
}

/***********************************************************
 * CheckFrom
 ***********************************************************/
void
HWindow::CheckFrom(entry_ref ref)
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
		else{
			entry_ref ref;
			entry.GetRef(&ref);
			if(AddPopServer(ref,sendMsg) != B_OK)
				goto err;
		}
	}
	if(!sendMsg.IsEmpty())
		PostMessage(&sendMsg,fPopClientView);
	return;
err:
	(new BAlert("","Account file is corrupted","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
}

/***********************************************************
 * AddPopServer
 ***********************************************************/
status_t
HWindow::AddPopServer(entry_ref ref,BMessage &sendMsg)
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
		
	sendMsg.AddString("name",name);
	if(msg.FindString("pop_host",&host) != B_OK)
		return B_ERROR;
	sendMsg.AddString("address",host);
			
	if(msg.FindString("pop_port",&port) != B_OK)
		return B_ERROR;
	sendMsg.AddInt16("port",atoi(port));
			
	if(msg.FindString("pop_user",&login) != B_OK)
		return B_ERROR;
	sendMsg.AddString("login",login);
			
	int16 proto;
	if(msg.FindInt16("protocol_type",&proto) != B_OK)
		proto = 0;
	sendMsg.AddInt16("protocol_type",proto);		
	
	int16 retrieve;
	if(msg.FindInt16("retrieve",&retrieve) != B_OK)
		return B_ERROR;
	sendMsg.AddBool("delete",(retrieve==0)?false:true);
			
	iValue = 0;
	if(retrieve == 2)
		msg.FindInt32("delete_day",&iValue);
	sendMsg.AddInt32("delete_day",iValue);
	
	const char* uidl;
	if(msg.FindString("uidl",&uidl) != B_OK)	
		sendMsg.AddString("uidl","");
	else
		sendMsg.AddString("uidl",uidl);
	PRINT(("SAVED UIDL:%s\n",uidl));
		
	if(msg.FindString("pop_password",&password) != B_OK)
		return B_ERROR;
	BString pass("");
	int32 len = strlen(password);
	for(int32 k = 0;k < len;k++)
		pass << (char)(255-password[k]);
	
	sendMsg.AddString("password",pass);
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
	BList AddList,DelList;
		
	message->GetInfo("mail",&type,&count);
	if(count == 0)
		return;
	HFolderItem *to;
	HFolderItem *from;
	message->FindPointer("to",(void**)&to);
	message->FindPointer("from",(void**)&from);
	
	ref = to->Ref();
	BPath path(&ref);
	
	for(i = 0;i < count;i++)
	{			
		message->FindPointer("mail",i,(void**)&mail);
		DelList.AddItem(mail);
		
		ref = mail->Ref();
		MoveFile(ref,path.Path());
		BPath item_path(path.Path());
		item_path.Append(BPath(&ref).Leaf());
		::get_ref_for_path(item_path.Path(),&ref);
		
		AddList.AddItem(new HMailItem(ref,
										mail->fStatus.String(),
										mail->fSubject.String(),
										mail->fFrom.String(),
										mail->fTo.String(),
										mail->fCC.String(),
										mail->fWhen,
										mail->fPriority.String(),
										mail->fEnclosure
										));
	}
	
	
	to->AddMails(&AddList);
	from->RemoveMails(&DelList);
	int32 old_selection = fMailList->CurrentSelection();
	fMailList->DeselectAll();
	fMailList->RemoveMails(&DelList);
	fMailList->Select( old_selection );
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
	HMailItem *item = cast_as(fMailList->ItemAt(sel),HMailItem);
	
	entry_ref ref = item->Ref();
	MakeReadWindow(ref);
}

/***********************************************************
 * DeleteMails
 ***********************************************************/
void
HWindow::DeleteMails()
{
	BPath path;
	::find_directory(B_USER_DIRECTORY, &path, true);
	path.Append("mail");
	path.Append( TRASH_FOLDER );
	
	entry_ref ref;
	::get_ref_for_path(path.Path(),&ref);
	
	HFolderItem *item(NULL);
	
	int32 folder = fFolderList->FindFolder(ref);
	
	if(folder < 0)
	{
		fFolderList->AddItem((item = new HFolderItem(ref,fFolderList)));
		//fFolderList->SortItems();
	}else
		item = cast_as(fFolderList->ItemAt(folder),HFolderItem);
		
	int32 sel = fFolderList->CurrentSelection();
	if(sel < 0)
		return;
	HFolderItem *from = cast_as(fFolderList->ItemAt(sel),HFolderItem);
	if(from == item) 
		return;
	BMessage msg(M_MOVE_MAIL);
	msg.AddPointer("from",from);
	msg.AddPointer("to",item);
	
	int32 selected; 
	int32 count_selected = 0;
	int32 sel_index = 0;
	int32 unread_mails = 0;
	HMailItem *mail(NULL);
	while((selected = fMailList->CurrentSelection(sel_index++)) >= 0)
	{
		mail=cast_as(fMailList->ItemAt(selected),HMailItem);
		if(!item)
			continue;
		count_selected++;
		msg.AddPointer("mail",mail);
		msg.AddRef("refs",&mail->fRef);
		if(!mail->IsRead())
			unread_mails++;
	}
	if(unread_mails>0)
	{
		from->SetName(from->Unread()-unread_mails);
		fFolderList->InvalidateItem(sel);
	}
	PostMessage(&msg);
}

/***********************************************************
 * MoveFile
 ***********************************************************/
void
HWindow::MoveFile(entry_ref file_ref,const char *kDir_Path)
{
	if(BNode(&file_ref).InitCheck() != B_OK)
		return;
	int32 i = 0;
	BPath filePath(&file_ref);
	BDirectory destDir(kDir_Path);
	BString name = filePath.Leaf();
	
	BString toName = name;
	
	//PRINT(("Mail:%s\n",filePath.Path() ));
	//PRINT(("DEST:%s\n",dir_path ));
	
	BEntry entry(&file_ref);
	while(entry.MoveTo(&destDir,toName.String(),false) != B_OK)
		toName << "_" << i;
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
		HMailItem *item = cast_as(fMailList->ItemAt(sel),HMailItem);
		if(item)
		{
			ref = item->Ref();
			BNode node(&ref);
			if(node.InitCheck() != B_OK)
				continue;
			node.ReadAttrString(B_MAIL_ATTR_FROM,&from);
			
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
	file.ReadAttrString(B_MAIL_ATTR_SUBJECT,&subject);
	
	subject.Insert("Fwd:",0);
	off_t size;
	file.GetSize(&size);
	char *buf = new char[size+1];
	size = file.Read(buf,size);
	buf[size] = '\0';
	
	BRect rect;
	((HApp*)be_app)->Prefs()->GetData("write_window_rect",&rect);
	MakeWriteWindow(subject.String()
					,NULL
					,item
					,false
					,true);
	delete[] buf;
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
	file.ReadAttrString(B_MAIL_ATTR_FROM,&to);
	file.ReadAttrString(B_MAIL_ATTR_CC,&cc);
	file.ReadAttrString(B_MAIL_ATTR_SUBJECT,&subject);
	file.ReadAttrString(B_MAIL_ATTR_REPLY,&reply);
	
	if(reply.Length() > 0)
		to = reply;
	
	if(reply_all)
	{
		to << "," <<cc;	
	}
	
	if( strncmp(subject.String(),"Re:",3) != 0)
		subject.Insert("Re: ",0);
	off_t size;
	file.GetSize(&size);
	char *buf = new char[size+1];
	size = file.Read(buf,size);
	buf[size] = '\0';
	
	MakeWriteWindow(subject.String()
					,to.String()
					,item
					,true);
					
	delete[] buf;
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
		
		message->FindInt32("modifiers",&modifiers);
		message->FindString("bytes",&bytes);
		if(bytes[0] != B_SPACE)
			return BWindow::DispatchMessage(message,handler);
		BScrollBar *bar = fMailView->ScrollBar(B_VERTICAL);
		float min,max,cur;
		bar->GetRange(&min,&max);
		cur = bar->Value();
		if(cur != max)
		{
			// Scroll down with space key
			char b[1];
			b[0] = (modifiers && B_SHIFT_KEY)?B_PAGE_UP:B_PAGE_DOWN;
			fMailView->KeyDown(b,1);
		}else{
			if(modifiers&B_SHIFT_KEY)
				PostMessage(M_SELECT_PREV_MAIL,fMailList);
			else
				PostMessage(M_SELECT_NEXT_MAIL,fMailList);
		}
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
						HMailItem *replyItem,
						bool reply,
						bool forward)
{
	BRect rect;
	((HApp*)be_app)->Prefs()->GetData("write_window_rect",&rect);
	HWriteWindow *win = new HWriteWindow(rect,_("New Message")
							,subject
							,to
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
	BRect rect;
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
	TrackerUtils utils;
	int32 count = fFolderList->CountItems();
	HFolderItem *trash(NULL);
		
	for(int32 i = 0;i < count;i++)
	{
		HFolderItem *item = cast_as(fFolderList->ItemAt(i),HFolderItem);
		if(::strcmp(item->Name(), TRASH_FOLDER ) == 0)
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
	
	while( err == B_OK )
	{
		if( (err = dir.GetNextRef( &ref )) == B_OK)
		{
			if(entry.SetTo(&ref) != B_OK)
				continue;
			if(entry.Exists())
				utils.MoveToTrash(ref);
		}
	}
	if(fFolderList->CurrentSelection() == fFolderList->IndexOf(trash))
		fMailList->MakeEmpty();
	
	trash->EmptyMailList();
	trash->Gather();
	fFolderList->InvalidateItem(fFolderList->IndexOf(trash));	
}

/***********************************************************
 * AddCheckFromItems
 ***********************************************************/
void
HWindow::AddCheckFromItems()
{
	// Delete all items
	BMenu *subMenu = KeyMenuBar()->SubmenuAt(2);
	subMenu = subMenu->SubmenuAt(1);
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
			char name[B_FILE_NAME_LENGTH+1];
			entry.GetName(name);
			entry_ref ref;
			entry.GetRef(&ref);
			BMessage *msg = new BMessage(M_CHECK_FROM);
			msg->AddRef("refs",&ref);
			subMenu->AddItem(new BMenuItem(name,msg));
		}
	}
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HWindow::QuitRequested()
{	
	if(fPopClientView->IsRunning())
	{
		int32 btn = (new BAlert("","POP3 session is running",_("Force Quit"),_("Wait"),NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		if(btn == 0)
			fPopClientView->Cancel();
		else
			return false;
	}
	if(fSmtpClientView->IsRunning())
	{
		int32 btn = (new BAlert("","SMTP session is running",_("Force Quit"),_("Wait"),NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		if(btn == 0)
			fSmtpClientView->Cancel();
		else
			return false;
	}
	Hide();
	Unlock();
	fMailView->StopLoad();
	Lock();
	
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
	fMailList->SaveColumns();
	fMailList->MakeEmpty();
	// Delete all folder items
	fFolderList->DeleteAll();
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}

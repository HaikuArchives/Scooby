#include "HApp.h"
#include "RectUtils.h"
#include "HWindow.h"
#include "HPrefs.h"
#include "HAboutWindow.h"
#include "HPrefWindow.h"
#include "HDeskbarView.h"
#include "HFindWindow.h"
#include "HReadWindow.h"
#include "HWriteWindow.h"
#include "ResourceUtils.h"

#include <String.h>
#include <Debug.h>
#include <PrintJob.h>
#include <TextView.h>
#include <Beep.h>
#include <Roster.h>
#include <ClassInfo.h>
#include <ScrollBar.h>
#include <Autolock.h>


/***********************************************************
 * Constructor
 ***********************************************************/
HApp::HApp() :LocaleApp(APP_SIG)
	,fPrintSettings(NULL)
	,fWindow(NULL)
	,fWatchNetPositive(false)
{
	BPath path;
	::find_directory(B_USER_DIRECTORY,&path);
	path.Append("mail");
	path.Append( TRASH_FOLDER );
	::create_directory(path.Path(),0777);
	char *pref_name = new char[strlen(APP_NAME) + 12];
	::sprintf(pref_name,"%s %s",APP_NAME,"preference");
	fPref = new HPrefs(pref_name,APP_NAME);
	delete[] pref_name;
	fPref->LoadPrefs();
	SetPulseRate(100000);
	AddSoundEvent("New E-mail");
	
	BRect rect(100,100,300,170);
	fFindWindow = new HFindWindow(rect,_("Find"));
	fFindWindow->Hide();
	fFindWindow->Show();
	// Load all mail icons
	InitIcons();
	//
#ifdef CHECK_NETPOSITIVE
	fFilterAdded(false)
	
	fMessageFilter = new BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE,MessageFilter);
#endif
}

/***********************************************************
 * Destructor
 ***********************************************************/
HApp::~HApp()
{
	delete fPref;
	delete fPrintSettings;
	// delete all icons
	delete fReadMailIcon;
	delete fNewMailIcon;
	delete fEnclosureIcon;
	delete fForwardedMailIcon;
	delete fSentMailIcon;
	delete fRepliedMailIcon;
	delete fPriority1;
	delete fPriority2;
	delete fPriority4;
	delete fPriority5;
	delete fOpenFolderIcon;
	delete fCloseFolderIcon;
	delete fOpenQueryIcon;
	delete fCloseQueryIcon;
	delete fOpenIMAPIcon;
	delete fCloseIMAPIcon;
	//
#ifdef CHECK_NETPOSITIVE
	delete fMessageFilter;
#endif
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HApp::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_POP_CONNECT:
	case M_NEW_MSG:
		fWindow->PostMessage(message);
		break;
	case M_PAGE_SETUP_MESSAGE:
		PageSetup();
		break;
	case M_PRINT_MESSAGE:
	{
		BView *view;
		BView *detail;
		if(message->FindPointer("view",(void**)&view) == B_OK)
		{
			message->FindPointer("detail",(void**)&detail);
			const char* job_name;
			message->FindString("job_name",&job_name);
			Print(view,detail,job_name);
		}
		break;
	}
	// Check whether new mails have received from Deskbar replicant
	case M_CHECK_SCOOBY_STATE:
	{
		int32 icon;
		if(message->FindInt32("icon",&icon) != B_OK)
			break;
		BMessage msg(M_CHECK_SCOOBY_STATE);
		msg.AddInt32("icon",fWindow->CurrentDeskbarIcon());
		message->SendReply(&msg,(BHandler*)NULL,1000000);
		break;
	}
	case M_SHOW_FIND_WINDOW:
	{
		ShowFindWindow();
		BWindow *window;
		BView *target(NULL);
		if(message->FindPointer("targetwindow",(void**)&window) == B_OK)
		{
			bool html_mode;
			fPref->GetData("use_html",&html_mode);
			if(is_kind_of(window,HWindow))
			{
				if(!html_mode)
					target = window->FindView("HMailView");
				else
					target = window->FindView("__NetPositive__HTMLView");
			}else if(is_kind_of(window,HReadWindow)){
				if(!html_mode)
					target = window->FindView("HMailView");
				else
					target = window->FindView("__NetPositive__HTMLView");
			}else if(is_kind_of(window,HWriteWindow)){
				target = window->FindView("HMailView");
			}
		}	
		
		fFindWindow->SetTarget(target);
		break;
	}
	case M_FIND_NEXT_WINDOW:
	{
		BWindow *window;
		BView *target(NULL);
		if(message->FindPointer("targetwindow",(void**)&window) == B_OK)
		{
			bool html_mode;
			fPref->GetData("use_html",&html_mode);
			if(is_kind_of(window,HWindow))
			{
				if(!html_mode)
					target = window->FindView("HMailView");
				else
					target = window->FindView("__NetPositive__HTMLView");
			}else if(is_kind_of(window,HReadWindow)){
				if(!html_mode)
					target = window->FindView("HMailView");
				else
					target = window->FindView("__NetPositive__HTMLView");
			}else if(is_kind_of(window,HWriteWindow)){
				target = window->FindView("HMailView");
			}
		}
		fFindWindow->SetTarget(target);
		fFindWindow->PostMessage('mFnd');
		break;
	}
	default:
		_inherited::MessageReceived(message);
	}	
}

/***********************************************************
 * ArgvReceived
 ***********************************************************/
void
HApp::ArgvReceived(int32 argc,char** argv)
{
	bool send = false;
	BString subject,to,cc,bcc,body,enclosure_path;
	
	for(int32 i = 0;i < argc;i++)
	{
		char *p;
		PRINT(("%d %s\n",i,argv[i]));
		if((p= strstr(argv[i],"mailto:")) )
		{
			p+=7;
			to = p;
			send = true;
			break;
		}else if(strcmp(argv[i],"-subject") == 0){
			subject = argv[++i];
			send = true;
		}else if(strcmp(argv[i],"-body") == 0){
			body = argv[++i];
			send = true;
		}else if(strcmp(argv[i],"ccto:") == 0){
			p+=5;
			cc = p;
			send = true;
		}else if(strcmp(argv[i],"enclosure:") == 0){
			p+=10;
			enclosure_path = p;
			send = true;
		}else if(strcmp(argv[i],"bccto:") == 0){
			p+=6;
			bcc = p;
			send = true;
		}else if( strcmp(argv[i],"--help") == 0){
			printf(" usage: Scooby [ mailto:<address> ] [ -subject \"<text>\" ] [ ccto:<address> ] [ bccto:<address> ] [ -body \"<body text>\" ] [ enclosure:<path> ] [ <message to read> ...]\n");
			exit(0);
		}
	}
	
	if(send)
	{
		MakeMainWindow(true);
		
		if(fWindow->Lock())
		{
			fWindow->MakeWriteWindow(subject.String() // subject to cc bcc body	 enclosure_path
									,to.String()
									,cc.String()
									,bcc.String()
									,body.String()
									,enclosure_path.String());
			fWindow->Unlock();
		}
	}
	_inherited::ArgvReceived(argc,argv);
}

/***********************************************************
 * RefsReceived
 ***********************************************************/
void
HApp::RefsReceived(BMessage *message)
{
	message->PrintToStream();
	fWindow->RefsReceived(message);
	MakeMainWindow(true);
}

/***********************************************************
 * ReadyToRun
 ***********************************************************/
void
HApp::ReadyToRun()
{
	MakeMainWindow();
}

/***********************************************************
 * MakeMainWindow
 ***********************************************************/
bool
HApp::MakeMainWindow(bool hidden)
{
	if(fWindow)
		return false;
	
	BRect rect;
	fPref->GetData("window_rect",&rect);
	fWindow = new HWindow(rect,APP_NAME);
	
	if(hidden)
		fWindow->Minimize(true);
	fWindow->Show();
	return true;
}

/***********************************************************
 * PageSetup
 ***********************************************************/
void
HApp::PageSetup()
{
	status_t result;
    BPrintJob print("PageSetup");
	if(fPrintSettings)
		print.SetSettings(new BMessage(*fPrintSettings));
	if ((result = print.ConfigPage()) == B_OK) {
		delete fPrintSettings;
		fPrintSettings = print.Settings();
	}
}

/***********************************************************
 * Print
 ***********************************************************/
void
HApp::Print(BView *view ,BView *detailView,const char* job_name)
{
    status_t		err;
    BTextView		*textView = cast_as(view,BTextView);
    BPrintJob print( job_name ); 
    
	if(!fPrintSettings)
	{
		err = print.ConfigPage();
		if(err == B_OK)
		{
			fPrintSettings = print.Settings();
		}else
			return;
	}
	print.SetSettings(new BMessage(*fPrintSettings));
	
	int32 lineCount = 0;
	float lineHeight = 0;
	float bodyHeight = 0;
  	if(textView)
  	{
   		lineCount = textView->CountLines();
   		lineHeight = textView->LineHeight();
   		bodyHeight = lineHeight * lineCount;
  	}else{
  		view->LockLooper();
  		bodyHeight = view->Frame().Height();
  		BScrollBar *bar;
  		if( (bar = view->ScrollBar(B_VERTICAL)) )
  		{
  			float min,max;
  			bar->GetRange(&min,&max);
  			bodyHeight += (max == min)?0:max;
  		}
  		view->UnlockLooper();
  	}
	// Printing
	if(textView && lineCount <=0)
		goto out;
	if (print.ConfigJob() == B_NO_ERROR) {
		BRect printRect(print.PrintableRect());
		printRect.OffsetTo(0, 0);
		
		if(textView)
			printRect.bottom = lineHeight * floor( printRect.Height() / lineHeight );
	
		print.BeginJob();
		if (!print.CanContinue())
			goto out;
			
		int32 firstPage=print.FirstPage();
		int32 lastPage=print.LastPage();
		
		//print the first page
		BRect headerRect;
		
	    if (detailView && detailView->LockLooper())
	    {
			BRect origDetailBounds=detailView->Bounds();
	    	headerRect=detailView->Bounds();
	    	if (firstPage<=1)
	    	{
		    	detailView->ResizeTo(printRect.Width(), origDetailBounds.Height());
		    	headerRect=detailView->Bounds();
				print.DrawView(detailView, headerRect, BPoint(0, 0));
		    	detailView->ResizeTo(origDetailBounds.Width(), origDetailBounds.Height());
		    }
		    detailView->UnlockLooper();
	    }
	    
	    float headerHeight;
	    if(textView)
	    	headerHeight=ceil(headerRect.Height()/lineHeight)*lineHeight;
		else
			headerHeight=ceil(headerRect.Height());
		BRect firstPageBodyRect(printRect);
		firstPageBodyRect.bottom -= headerHeight;
		if (firstPage<=1 )
		{
			print.DrawView((textView)?textView:view, firstPageBodyRect, BPoint(0, headerRect.Height()));
			print.SpoolPage();
			firstPage++;
		}
		printRect.OffsetBy(0, firstPageBodyRect.Height());
		if (!print.CanContinue())
			goto out;
		
		//print the other pages
		float fullHeight = bodyHeight + headerHeight;
		int32 pageCount = (int32) ceil(fullHeight / printRect.Height());
		if (lastPage > pageCount) lastPage=pageCount;
		
		for (int32 loop = firstPage; loop <= lastPage; loop++) {
			
			print.DrawView((textView)?textView:view, printRect, BPoint(0, 0));
			print.SpoolPage();
			printRect.OffsetBy(0, printRect.Height());
			if (!print.CanContinue())
				goto out;
		}
		print.CommitJob();
	}
out:
	return;
}

/***********************************************************
 * AddSoundEvent
 ***********************************************************/
void
HApp::AddSoundEvent(const char* name)
{
	::add_system_beep_event(name);
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
HApp::QuitRequested()
{
	// Wait for closing all NetPositive windows
	if(IsNetPositiveRunning())
	{
		fWatchNetPositive = true;
		return false;
	}
	// Quit FindWindow
	fFindWindow->SetQuit(true);
	fFindWindow->PostMessage(B_QUIT_REQUESTED);
	// Remove all IMAP4 mails
	RemoveTmpImapMails();
	return _inherited::QuitRequested();
}

/***********************************************************
 * AboutRequested
 ***********************************************************/
void
HApp::AboutRequested()
{
	(new HAboutWindow(APP_NAME,
			__DATE__,
			"Created by Atsushi Takamatsu @ Sapporo,Japan.\n\nIf you want to send me bugs, include stack crawl info :)",
			"http://scooby.sourceforge.net/",
			"E-Mail: atsushi@io.ocn.ne.jp"))->Show();
}	

/***********************************************************
 * RemoveTmpImapMails
 ***********************************************************/
void
HApp::RemoveTmpImapMails()
{
	BPath path;
	::find_directory(B_COMMON_TEMP_DIRECTORY,&path);
	path.Append("*.imap");
	BString cmd("rm ");
	cmd += path.Path();
	cmd += " 2> /dev/null";
	
	::system(cmd.String());
}

/***********************************************************
 * IsNetPositiveRunning
 *	Check whether NetPositive child windows is running
 ***********************************************************/
bool
HApp::IsNetPositiveRunning()
{
	int32 count = CountWindows();
	BWindow *window(NULL);
	
	for(int32 i = 0;i < count;i++)
	{
		window = WindowAt(i);
		if(window && ::strcmp(window->Name(),"NetPositive") == 0)
		{
#ifdef CHECK_NETPOSITIVE
			if(!fFilterAdded)
			{
				window->Lock();
				window->AddCommonFilter(fMessageFilter);
				window->Unlock();
				fFilterAdded = true;
			}
#endif
			return true;
		}
	}
	return false;
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HApp::Pulse()
{
	if(fWatchNetPositive && !IsNetPositiveRunning())
	{
		PostMessage(B_QUIT_REQUESTED);
	}
}

#ifdef CHECK_NETPOSITIVE
/***********************************************************
 * MessageFilter
 ***********************************************************/
filter_result
HApp::MessageFilter(BMessage *message,BHandler **target,BMessageFilter *messageFilter)
{
	if(message->what != '_PUL' && 
		message->what != 'prog'&& 
		message->what != '_MCH' &&
		message->what != '_MMV' && 
		message->what != '_UPD' && 
		message->what != '_UKD' && 
		message->what != '_KYD' && 
		message->what != '_EVP')
	{
		if(::strcmp((*target)->Name(),"__NetPositive__HTMLView") == 0)
		message->PrintToStream();
	}
	return B_DISPATCH_MESSAGE;
}
#endif

/***********************************************************
 * ShowFindWindow
 ***********************************************************/
void
HApp::ShowFindWindow()
{
	if(fFindWindow->Lock())
	{
		if(fFindWindow->IsMinimized())
			fFindWindow->Minimize(false);
		if(fFindWindow->IsHidden())
			fFindWindow->Show();
		fFindWindow->Activate();
		fFindWindow->Unlock();
	}
}

/***********************************************************
 * InitIconCache
 ***********************************************************/
void
HApp::InitIcons()
{
	ResourceUtils utils;
	fNewMailIcon = utils.GetBitmapResource('BBMP',"New");
	fReadMailIcon = utils.GetBitmapResource('BBMP',"Read");
	fSentMailIcon = utils.GetBitmapResource('BBMP',"Sent");
	fRepliedMailIcon = utils.GetBitmapResource('BBMP',"Replied");
	fForwardedMailIcon = utils.GetBitmapResource('BBMP',"Forwarded");
	fEnclosureIcon = utils.GetBitmapResource('BBMP',"Enclosure");
	fPriority1 = utils.GetBitmapResource('BBMP',"1");
	fPriority2 = utils.GetBitmapResource('BBMP',"2");
	fPriority4 = utils.GetBitmapResource('BBMP',"4");
	fPriority5 = utils.GetBitmapResource('BBMP',"5");
	fOpenFolderIcon = utils.GetBitmapResource('BBMP',"OpenFolder");
	fCloseFolderIcon = utils.GetBitmapResource('BBMP',"CloseFolder");
	fOpenQueryIcon = utils.GetBitmapResource('BBMP',"OpenQuery");
	fCloseQueryIcon = utils.GetBitmapResource('BBMP',"CloseQuery");
	fOpenIMAPIcon = utils.GetBitmapResource('BBMP',"OpenIMAP");
	fCloseIMAPIcon = utils.GetBitmapResource('BBMP',"CloseIMAP");
}

/***********************************************************
 * GetIcon
 ***********************************************************/
BBitmap*
HApp::GetIcon(const char* icon_name)
{
	BBitmap *bitmap(NULL);
	
	if(strcmp(icon_name,"New") == 0)
		bitmap = fNewMailIcon;
	else if(strcmp(icon_name,"Read") == 0)
		bitmap = fReadMailIcon;
	else if(strcmp(icon_name,"Sent") == 0)
		bitmap = fSentMailIcon;
	else if(strcmp(icon_name,"Replied") == 0)
		bitmap = fRepliedMailIcon;
	else if(strcmp(icon_name,"Forwarded") == 0)
		bitmap = fForwardedMailIcon;
	else if(strcmp(icon_name,"Enclosure")== 0)
		bitmap = fEnclosureIcon;
	else if(strcmp(icon_name,"1")== 0)
		bitmap = fPriority1;
	else if(strcmp(icon_name,"2")== 0)
		bitmap = fPriority2;
	else if(strcmp(icon_name,"4")== 0)
		bitmap = fPriority4;
	else if(strcmp(icon_name,"5")== 0)
		bitmap = fPriority5;
	else if(strcmp(icon_name,"OpenFolder")== 0)
		bitmap = fOpenFolderIcon;
	else if(strcmp(icon_name,"CloseFolder")== 0)
		bitmap = fCloseFolderIcon;
	else if(strcmp(icon_name,"OpenQuery")== 0)
		bitmap = fOpenQueryIcon;
	else if(strcmp(icon_name,"CloseQuery")== 0)
		bitmap = fCloseQueryIcon;
	else if(strcmp(icon_name,"OpenIMAP")== 0)
		bitmap = fOpenIMAPIcon;
	else if(strcmp(icon_name,"CloseIMAP")== 0)
		bitmap = fCloseIMAPIcon;	
	return bitmap;
}

/***********************************************************
 * Main
 ***********************************************************/
int main()
{
	HApp app;
	app.Run();
	return 0;
}

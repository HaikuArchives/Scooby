#include "HApp.h"
#include "RectUtils.h"
#include "HWindow.h"
#include "HPrefs.h"
#include "HAboutWindow.h"
#include "HPrefWindow.h"
#include "HDeskbarView.h"

#include <String.h>
#include <Debug.h>
#include <PrintJob.h>
#include <TextView.h>
#include <Beep.h>
#include <Roster.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HApp::HApp() :LocaleApp(APP_SIG)
	,fPrintSettings(NULL)
	,fWindow(NULL)
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
	
	AddSoundEvent("New E-mail");
}

/***********************************************************
 * Destructor
 ***********************************************************/
HApp::~HApp()
{
	delete fPref;
	delete fPrintSettings;
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
		BTextView *view;
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
	for(int32 i = 0;i < argc;i++)
	{
		char *p;
		if((p= strstr(argv[i],"mailto:")) )
		{
			p+=7;
			if( !MakeMainWindow(p,true) )
			{
				fWindow->MakeWriteWindow(NULL,p);		
			}
			break;
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
	MakeMainWindow(NULL,true);
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
HApp::MakeMainWindow(const char* mail_addr,bool hidden)
{
	if(fWindow)
		return false;
	
	BRect rect;
	fPref->GetData("window_rect",&rect);
	fWindow = new HWindow(rect,APP_NAME,mail_addr);
	
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
HApp::Print(BTextView *textview ,BView *detail,const char* job_name)
{
    status_t		err;
    BView			*detailView(detail);
    BTextView		*bodyView(textview);
    /*
    if (view->LockLooper())
    {
    	//detailView=view->FindView("WriteView");
    	bodyView=dynamic_cast<BTextView*>(view->FindView("HMailView"));
    	view->UnlockLooper();
    }
	*/
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
	
	int32 lineCount = bodyView->CountLines();
	float lineHeight = bodyView->LineHeight();
	float bodyHeight = lineHeight * lineCount;
   
   	if ((lineCount) && (print.ConfigJob() == B_NO_ERROR)) {
		BRect printRect(print.PrintableRect());
		printRect.OffsetTo(0, 0);
		printRect.bottom = lineHeight * floor( printRect.Height() / lineHeight );
		
		print.BeginJob();
		if (!print.CanContinue())
			goto out;
			
		int32 firstPage=print.FirstPage();
		int32 lastPage=print.LastPage();
		
		//print the first page
		BRect headerRect;
	    if (detailView && textview->LockLooper())
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
	    	textview->UnlockLooper();
	    }
	    float headerHeight=ceil(headerRect.Height()/lineHeight)*lineHeight;
		BRect firstPageBodyRect(printRect);
		firstPageBodyRect.bottom -= headerHeight;
		if (firstPage<=1)
		{
			print.DrawView(bodyView, firstPageBodyRect, BPoint(0, headerRect.Height()));
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
			print.DrawView(bodyView, printRect, BPoint(0, 0));
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
			"Created by Atsushi Takamatsu @ Sapporo,Japan.",
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
 * Main
 ***********************************************************/
int main()
{
	HApp app;
	app.Run();
	return 0;
}

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
	case M_DESKBAR_INSTALLED:
		fWindow->PostMessage(message);
		break;
	case M_PAGE_SETUP_MESSAGE:
		PageSetup();
		break;
	case M_PRINT_MESSAGE:
	{
		BTextView *view;
		if(message->FindPointer("view",(void**)&view) == B_OK)
		{
			const char* job_name;
			message->FindString("job_name",&job_name);
			Print(view,job_name);
		}
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
HApp::Print(BTextView *view ,const char* job_name)
{
	int32			lines;
	int32			lines_page;
	int32			loop;
	int32			pages;
	float			line_height;
    BRect			r;
    status_t		err;
	
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
	
	lines = view->CountLines();
	line_height = view->LineHeight();
   
   	if ((lines) && ((int)line_height) && (print.ConfigJob() == B_NO_ERROR)) {
		r = print.PrintableRect();
		lines_page = static_cast<int32>( r.Height() / line_height );
		pages = lines / lines_page;
		r.top = 0;
		r.bottom = (line_height * lines_page);
		r.right -= r.left;
		r.left = 0;
		print.BeginJob();
		if (!print.CanContinue())
			goto out;
		for (loop = 0; loop <= pages; loop++) {
			print.DrawView(view, r, BPoint(0, 0));
			print.SpoolPage();
			r.top += (line_height * lines_page);
			r.bottom += (line_height * lines_page);
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
 * Main
 ***********************************************************/
int main()
{
	HApp app;
	app.Run();
	return 0;
}

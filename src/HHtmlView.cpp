#include "HHtmlView.h"

#include <Debug.h>
#include <Message.h>
#include <Window.h>
#include <FindDirectory.h>
#include <Node.h>
#include <Path.h>
#include <Roster.h>
#include <Application.h>
#include <string.h>
#include <Alert.h>

#define STARTUP_PAGE "netpositive:Startup.html"

/***********************************************************
 * Constructor
 ***********************************************************/
HHtmlView::HHtmlView(BRect rect,
					const char* name,
					bool use_toolbar,
					uint32 resize,
					uint32 flags)
	:BView(rect,name,resize,B_WILL_DRAW|B_FRAME_EVENTS|flags)
	,fNetPositiveView(NULL)
{
	rect = Bounds();
	// Get startup file path
	app_info info;
    be_app->GetAppInfo(&info); 
    BPath path(&info.ref);
    path.GetParent(&path);
    path.Append("html/startup.html");
    char *buf = new char[::strlen(path.Path())+8];
    ::sprintf(buf,"file://%s",path.Path());
	// Add Netpositive replicant
    BMessage message_replicant(B_ARCHIVED_OBJECT); 
    message_replicant.AddString("add_on",B_NETPOSITIVE_APP_SIGNATURE); 
   	message_replicant.AddString("url",buf);
   	delete[] buf; 
    message_replicant.AddBool("openAsText",0); 
    message_replicant.AddInt32("encoding",GetDefaultEncoding()); 
    message_replicant.AddString("class","NPBaseView"); 
    message_replicant.AddString("_name","NetPositive"); 
    message_replicant.AddRect("_frame",rect); 
    message_replicant.AddInt32("_flags",B_WILL_DRAW | B_FRAME_EVENTS |B_FULL_UPDATE_ON_RESIZE); 
    message_replicant.AddInt32("_resize_mode",B_FOLLOW_ALL); 
    message_replicant.AddBool("showToolbars",use_toolbar); 

    fShelf = new BShelf(this,false,"NetShelf"); 
    fShelf->SetDisplaysZombies(true);
    if(fShelf->AddReplicant(&message_replicant,BPoint(0,0)) == B_OK)
    {
   		fNetPositiveView = FindView("NetPositive");
   		if(fNetPositiveView)
   			fNetPositiveView->SetResizingMode(B_FOLLOW_ALL);
	}else{
		(new BAlert("","Could not create NetPositive replicant","OK"))->Go();
	}
}


/***********************************************************
 * Destructor
 ***********************************************************/
HHtmlView::~HHtmlView()
{
	delete fShelf;	
}

/***********************************************************
 * ShowURL
 ***********************************************************/
void
HHtmlView::ShowURL(const char* url)
{
	BMessage msg(B_NETPOSITIVE_OPEN_URL);
	msg.AddString("be:url",url);
	Window()->PostMessage(&msg,fNetPositiveView);	
}

/***********************************************************
 * Forward
 ***********************************************************/
void
HHtmlView::Forward()
{
	Window()->PostMessage(B_NETPOSITIVE_FORWARD,fNetPositiveView);	
}

/***********************************************************
 * Home
 ***********************************************************/
void
HHtmlView::Home()
{
	Window()->PostMessage(B_NETPOSITIVE_HOME,fNetPositiveView);	
}

/***********************************************************
 * Back
 ***********************************************************/
void
HHtmlView::Back()
{
	Window()->PostMessage(B_NETPOSITIVE_BACK,fNetPositiveView);	
}

/***********************************************************
 * Reload
 ***********************************************************/
void
HHtmlView::Reload()
{
	Window()->PostMessage(B_NETPOSITIVE_RELOAD,fNetPositiveView);	
}


/***********************************************************
 * Stop
 ***********************************************************/
void
HHtmlView::Stop()
{
	Window()->PostMessage(B_NETPOSITIVE_STOP,fNetPositiveView);	
}


/***********************************************************
 * SetEncoding
 ***********************************************************/
void
HHtmlView::SetEncoding(int32 encoding_index)
{
	// not implemented yet
	// I have no clue to set encoding :(
/*	BMessage msg('NPCP');
	msg.AddInt32("encoding",encoding_index);
	Window()->PostMessage(&msg,fNetPositiveView);	
*/
}

/***********************************************************
 * GetDefaultEncoding
 ***********************************************************/
int32
HHtmlView::GetDefaultEncoding()
{
	int32 encoding = 0;
	// Load NetPositive setting file
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append("NetPositive");
	path.Append("settings");
	
	BNode node(path.Path());
	if(node.InitCheck() != B_OK)
		return 0;
	node.ReadAttr("DefaultEncoding",B_INT32_TYPE,0,&encoding,sizeof(int32));
	return encoding;
}
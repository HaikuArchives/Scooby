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
#include <UTF8.h>

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
    message_replicant.AddBool("openAsText",false); 
    message_replicant.AddInt32("encoding",GetDefaultEncoding(fDefaultEncoding)); 
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
	SetEncoding((EncodingMessages)fDefaultEncoding);
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
HHtmlView::SetEncoding(EncodingMessages encoding)
{
	BMessage msg(encoding);
	if(!fNetPositiveView)
		return;
	BView *view = fNetPositiveView->FindView("__NetPositive__HTMLView");
	if(view)
		Window()->PostMessage(&msg,view);	
}

/***********************************************************
 * GetDefaultEncoding
 ***********************************************************/
int32
HHtmlView::GetDefaultEncoding(int32 &outEncoding)
{
	int32 encoding = 0;
	int32 result = 0;
	// Load NetPositive setting file
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append("NetPositive");
	path.Append("settings");
	
	BNode node(path.Path());
	if(node.InitCheck() != B_OK)
		return 0;
	node.ReadAttr("DefaultEncoding",B_INT32_TYPE,0,&encoding,sizeof(int32));
	
	switch(encoding)
	{
	case B_ISO1_CONVERSION:
		result = NETPOSITIVE_ISO1;
		break;
	case B_ISO2_CONVERSION:
		result = NETPOSITIVE_ISO2;
		break;
	case B_ISO5_CONVERSION:
		result = NETPOSITIVE_ISO5;
		break;
	case B_ISO7_CONVERSION:
		result = NETPOSITIVE_ISO7;
		break;
	case B_SJIS_CONVERSION:
		result = NETPOSITIVE_SJIS;
		break;
	case B_EUC_CONVERSION:
		result = NETPOSITIVE_EUC;
		break;
	case B_UNICODE_CONVERSION:
		result = NETPOSITIVE_UNICODE;
		break;
	case 256:
		result = NETPOSITIVE_JAPANESE_AUTO;
		break;
	case 258:
		result = NETPOSITIVE_UTF8;
		break;
	case B_MAC_ROMAN_CONVERSION:
		result = NETPOSITIVE_MACROMAN;
		break;
	case B_KOI8R_CONVERSION:
		result = NETPOSITIVE_KOI8R;
		break;
	case B_MS_WINDOWS_1251_CONVERSION:
		result = NETPOSITIVE_WINDOWS_1251;
		break;
	case B_MS_DOS_866_CONVERSION:
		result = NETPOSITIVE_MSDOS_886;
		break;
		
	}
	PRINT(("Encoding:%d -> %d\n",encoding,result));
	outEncoding = result;
	return encoding;
}
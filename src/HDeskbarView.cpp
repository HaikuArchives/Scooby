#include "HDeskbarView.h"
#include <Message.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <PopUpMenu.h>
#include <Window.h>
#include <MenuItem.h>
#include <Entry.h>
#include <Roster.h>
#include <Alert.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <Resources.h>
#include <stdio.h>
#include <Debug.h>
#include <Beep.h>

#include "HWindow.h"
#include "MenuUtils.h"
#include "HApp.h"


/***********************************************************
 * This is the exported function that will be used by Deskbar
 * to create and add the replicant
 ***********************************************************/
extern "C" _EXPORT BView* instantiate_deskbar_item();


/***********************************************************
 * Constructor.
 ***********************************************************/
HDeskbarView::HDeskbarView(BRect frame)
		:BView(frame,APP_NAME,B_FOLLOW_NONE,B_WILL_DRAW)
		,fCurrentIconState(DESKBAR_NEW_ICON)
{
	fIcon = NULL;
}

/***********************************************************
 * Deskbar item installing function
 ***********************************************************/
BView* instantiate_deskbar_item(void)
{
	return new HDeskbarView(BRect(0, 0, 15, 15));	
}

/***********************************************************
 * Constructor for achiving.
 ***********************************************************/
HDeskbarView::HDeskbarView(BMessage *message)
	:BView(message)
	,fCurrentIconState(DESKBAR_NEW_ICON)
{
	fIcon = NULL;
	ChangeIcon(DESKBAR_NORMAL_ICON);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HDeskbarView::~HDeskbarView()
{
	delete fIcon;
}


/***********************************************************
 * Instantiate
 ***********************************************************/
HDeskbarView*
HDeskbarView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "HDeskbarView"))
		return NULL;	
	HDeskbarView *view = new HDeskbarView(data);
	
	// Send this pointer to Scooby
	BMessenger scooby(APP_SIG);
	if(scooby.IsValid())
	{
		BMessage msg(M_DESKBAR_INSTALLED);
		msg.AddPointer("deskbar_view",view);
		scooby.SendMessage(&msg);
	}
	return view;
}

/***********************************************************
 * Archive
 ***********************************************************/
status_t
HDeskbarView::Archive(BMessage *data,bool deep) const
{
	uint32 winresizingmode = ResizingMode();
	((BView*)this)->BView::SetResizingMode(B_FOLLOW_LEFT|B_FOLLOW_TOP);
	BView::Archive(data, deep);
	((BView*)this)->SetResizingMode(winresizingmode);

	data->AddString("add_on",APP_SIG);
	return B_NO_ERROR;
}

/***********************************************************
 * Draw
 ***********************************************************/
void
HDeskbarView::Draw(BRect /*updateRect*/)
{	
	rgb_color oldColor = HighColor();
	SetHighColor(Parent()->ViewColor());
	FillRect(BRect(0.0,0.0,15.0,15.0));
	SetHighColor(oldColor);
	SetDrawingMode(B_OP_OVER);
	if(fIcon)
		DrawBitmap(fIcon,BRect(0.0,0.0,15.0,15.0));
	SetDrawingMode(B_OP_COPY);			
	Sync();
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HDeskbarView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_CHANGE_DESKBAR_ICON:
	{
		int32 icon;
		if(message->FindInt32("icon",&icon) == B_OK)
			ChangeIcon(icon);	
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * ChangeIcon
 ***********************************************************/
void
HDeskbarView::ChangeIcon(int32 icon)
{
	if(fCurrentIconState == icon)
		return;
		
	BBitmap *new_icon(NULL);
	
	switch(icon)
	{
	case DESKBAR_NORMAL_ICON:
	{
		entry_ref ref;
		if(be_roster->FindApp(APP_SIG,&ref) == B_OK)
		{
			new_icon = new BBitmap(BRect(0.0,0.0,15.0,15.0),B_COLOR_8_BIT,true);
			BNodeInfo::GetTrackerIcon(&ref,new_icon,B_MINI_ICON);
		}
		break;
	}
	case DESKBAR_NEW_ICON:
	{
		entry_ref ref;
		if(be_roster->FindApp(APP_SIG,&ref) == B_OK)
		{
			// Load icon from Scooby's resource
			BFile file(&ref,B_READ_ONLY);
			if(file.InitCheck() != B_OK)
				break;
			BResources rsrc(&file);
			size_t len;
			const void *data = rsrc.LoadResource('BBMP',"Got mails", &len);
			if(len == 0)
				break;
			BMemoryIO stream(data, len);
			stream.Seek(0, SEEK_SET);
			BMessage archive;
			if (archive.Unflatten(&stream) != B_OK)
				break;
			new_icon = new BBitmap(&archive);
		}
		break;
	}
	}
	
	fCurrentIconState = icon;
	delete fIcon;
	fIcon = new_icon;
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HDeskbarView::MouseDown(BPoint pos)
{
	entry_ref app;
	BMessage msg(M_SHOW_MSG);
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons); 
	
	
  	BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
  	BFont font(be_plain_font);
  	font.SetSize(10);
  	theMenu->SetFont(&font);
  	
  	MenuUtils utils;
  	utils.AddMenuItem(theMenu,"New Message",M_NEW_MSG,NULL,NULL,0,0);
  	theMenu->AddSeparatorItem();
  	utils.AddMenuItem(theMenu,"Check Now",M_POP_CONNECT,NULL,NULL,0,0);
  	theMenu->AddSeparatorItem();
	utils.AddMenuItem(theMenu,"Quit",B_QUIT_REQUESTED,NULL,NULL,0,0);
  
	BRect r ;
   	ConvertToScreen(&pos);
   	r.top = pos.y - 5;
   	r.bottom = pos.y + 5;
   	r.left = pos.x -5;
   	r.right = pos.x +5;
         
	BMenuItem *bItem = theMenu->Go(pos, false,true,r);  
    if(bItem)
    {
    	BMessage*	aMessage = bItem->Message();
		if(aMessage)
		{
			if(be_roster->IsRunning(APP_SIG))
			{
				team_id id = be_roster->TeamFor(APP_SIG);
				BMessenger messenger(APP_SIG,id);
				messenger.SendMessage(aMessage);
			}	
		}
	}
	delete theMenu;
	
	BView::MouseDown(pos);
}
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
#include <stdlib.h>
#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <Deskbar.h>

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
		:BView(frame,APP_NAME,B_FOLLOW_NONE,B_WILL_DRAW|B_PULSE_NEEDED)
		,fIcon(NULL)
		,fCurrentIconState(DESKBAR_NEW_ICON)
{
	const char* kLabels[] = {"New Message","Check Now","Quit"};
	for(int32 i = 0;i < 3;i++)
		fLabels[i] = strdup( kLabels[i] );
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
	,fIcon(NULL)
	,fCurrentIconState(DESKBAR_NEW_ICON)
{
	//LocaleUtils utils(APP_SIG);
	const char* kLabels[] = {"New Message","Check Now","Quit"};
	for(int32 i = 0;i < 3;i++)
		fLabels[i] = strdup( _(kLabels[i]));
	ChangeIcon(DESKBAR_NORMAL_ICON);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HDeskbarView::~HDeskbarView()
{
	delete fIcon;
	for(int32 i = 0;i < 3;i++)
		free( fLabels[i] );
}

/***********************************************************
 * Instantiate
 ***********************************************************/
HDeskbarView*
HDeskbarView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "HDeskbarView"))
		return NULL;
	return new HDeskbarView(data);
}

/***********************************************************
 * Archive
 ***********************************************************/
status_t
HDeskbarView::Archive(BMessage *data,bool deep) const
{
	BView::Archive(data, deep);

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
	char icon_name[10];
	
	switch(icon)
	{
	case DESKBAR_NORMAL_ICON:
	{
		strcpy(icon_name ,"Read");
		break;
	}
	case DESKBAR_NEW_ICON:
	{
		strcpy(icon_name , "New");
		break;
	}
	}
	
	entry_ref ref;
	if(be_roster->FindApp(APP_SIG,&ref) == B_OK)
	{
		// Load icon from Scooby's resource
		BFile file(&ref,B_READ_ONLY);
		if(file.InitCheck() != B_OK)
			goto err;
		BResources rsrc(&file);
		size_t len;
		const void *data = rsrc.LoadResource('BBMP',icon_name, &len);
		if(len == 0)
			goto err;
		BMemoryIO stream(data, len);
		stream.Seek(0, SEEK_SET);
		BMessage archive;
		if (archive.Unflatten(&stream) != B_OK)
			goto err;
		new_icon = new BBitmap(&archive);
	}
err:
	fCurrentIconState = icon;
	delete fIcon;
	fIcon = new_icon;
	Invalidate();
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HDeskbarView::Pulse()
{
	BMessenger scooby(APP_SIG);
	if(scooby.IsValid())
	{
		BMessage reply;
		BMessage msg(M_CHECK_SCOOBY_STATE);
		msg.AddInt32("icon",fCurrentIconState);
		if( scooby.SendMessage(&msg,&reply,1000000,1000000) == B_OK)
		{
			int32 icon;
			if(reply.what == B_NO_REPLY)
				goto err;
			if(reply.FindInt32("icon",&icon) != B_OK)
				return;
			if(icon == fCurrentIconState)
				return;
		
			ChangeIcon(icon);
		}else{
			goto err;
		}
	}else
		goto err;
	return;
err:
	// probably Scooby had crached
	BDeskbar().RemoveItem(APP_NAME);

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
  	utils.AddMenuItem(theMenu,fLabels[0],M_NEW_MSG,NULL,NULL,0,0);
  	theMenu->AddSeparatorItem();
  	utils.AddMenuItem(theMenu,fLabels[1],M_POP_CONNECT,NULL,NULL,0,0);
  	theMenu->AddSeparatorItem();
	utils.AddMenuItem(theMenu,fLabels[2],B_QUIT_REQUESTED,NULL,NULL,0,0);
  
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

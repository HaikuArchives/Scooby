#include "HIMAP4Window.h"
#include "HApp.h"
#include "NumberControl.h"
#include "HIMAP4Folder.h"
#include "HFolderList.h"

#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <stdlib.h>
#include <ClassInfo.h>
#include <ListView.h>
#include <Handler.h>
#include <Debug.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HIMAP4Window::HIMAP4Window(BRect rect,
							BHandler *handler,
							HIMAP4Folder *item)
	:BWindow(rect,_("IMAP4 Property"),B_TITLED_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
	,fHandler(handler)
	,fItem(item)
{
	fAddMode = (item)?false:true;
	InitGUI();
	if(item)
	{
		SetText("address",item->Server());
		SetText("login",item->Login());
		SetText("password",item->Password());
		SetText("folder",item->FolderName());
		SetText("name",item->Name() );
		BString port;
		port << (int32)item->Port();
		SetText("port",port.String());
		
		BTextControl *ctrl = cast_as(FindView("name"),BTextControl);
		if(ctrl)
			ctrl->SetEnabled(false);
	}
}

/***********************************************************
 * Destructor
 ***********************************************************/
HIMAP4Window::~HIMAP4Window()
{
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HIMAP4Window::InitGUI()
{
	BView *bg = new BView(Bounds(),"bg",B_FOLLOW_ALL,B_WILL_DRAW);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	const char* kLabels[] = {_("Name:"),_("Address:"),_("Port:"),
							_("Login:"),_("Password:"),_("Folder:")};
	const char* kNames[]  = {"name","address","port","login","password",
							"folder"};
	const float kDivider = bg->StringWidth(_("Password:"))+5;	
	BRect rect = Bounds();
	rect.InsetBy(10,10);
	rect.bottom = rect.top + 20;
	
	BTextControl *ctrl;
	NumberControl *nctrl;
	for(int32 i = 0;i < 6;i++)
	{
		if(i == 2)
		{
			nctrl = new NumberControl(rect,kNames[i],kLabels[i],143,NULL);
			nctrl->SetDivider(kDivider);
			bg->AddChild(nctrl);
		}else{
			ctrl = new BTextControl(rect,kNames[i],kLabels[i],"",NULL);
			ctrl->SetDivider(kDivider);
			if(i == 5)
				ctrl->SetText("INBOX");
			if(i == 0)
			{
				// Disallow charactors that could not use filename
				ctrl->TextView()->DisallowChar('/');
				ctrl->TextView()->DisallowChar(':');
			}
			bg->AddChild(ctrl);
			if(i == 4)
				ctrl->TextView()->HideTyping(true);
		}
		rect.OffsetBy(0,25);
	}
	
	rect.OffsetBy(0,10);
	rect.left = rect.right - 80;
	const char* kTitle = (fAddMode)?_("Add"):_("Apply");
	BButton *button = new BButton(rect,"ok",kTitle,new BMessage('mOK '));
	bg->AddChild(button);
	AddChild(bg);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HIMAP4Window::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case 'mOK ':
	{
		if(fAddMode)
		{
			BMessage msg(M_ADD_FOLDER);
			BListView *view = cast_as(fHandler,BListView);
			if(!view)
			{
				PRINT(("NULL:%s %s\n",__FILE__,__LINE__));
				break;
			}
			msg.AddPointer("item",new HIMAP4Folder(GetText("name"),
											GetText("folder"),
											GetText("address"),
											atoi(GetText("port")),
											GetText("login"),
											GetText("password"),
											view));
			view->Window()->PostMessage(&msg,view);
						
		}else{
			if(!fItem)
				break;
			fItem->SetServer(GetText("address"));
			fItem->SetLogin(GetText("login"));
			fItem->SetPassword(GetText("password"));
			fItem->SetFolderName(GetText("folder"));
			int port = atoi(GetText("address"));
			fItem->SetPort(port);
		}
		this->PostMessage(B_QUIT_REQUESTED);
		break;
	}
	default:
		BWindow::MessageReceived(message);
	}
}

/***********************************************************
 * SetText
 ***********************************************************/
void
HIMAP4Window::SetText(const char* name,const char* text)
{
	BTextControl *ctrl = cast_as(FindView(name),BTextControl);
	if(ctrl)
		ctrl->SetText(text);
}

/***********************************************************
 * GetText
 ***********************************************************/
const char*
HIMAP4Window::GetText(const char* name)
{
	BTextControl *ctrl = cast_as(FindView(name),BTextControl);
	if(ctrl)
		return ctrl->Text();
	return NULL;
}

#include "HIMAP4Window.h"
#include "HApp.h"
#include "NumberControl.h"
#include "HIMAP4Folder.h"
#include "HFolderList.h"
#include "PassControl.h"

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
							const char* name,
							const char* folder,
							const char* server,
							int			port,
							const char* login,
							const char* password)
	:BWindow(rect,_("IMAP4 Settings"),B_TITLED_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
	,fHandler(handler)
{
	fAddMode = (name)?false:true;
	InitGUI();
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
	
	const char* kLabels[] = {_("Name:"),_("Address:"),_("Port"),
							_("Login:"),_("Password:"),_("Folder:")};
	const char* kNames[]  = {"name","address","port","login","password",
							"folder"};
	const float kDivider = bg->StringWidth(_("Password:"))+5;	
	BRect rect = Bounds();
	rect.InsetBy(10,10);
	rect.bottom = rect.top + 20;
	
	BTextControl *ctrl;
	NumberControl *nctrl;
	PassControl	*pctrl;
	for(int32 i = 0;i < 6;i++)
	{
		if(i == 2)
		{
			nctrl = new NumberControl(rect,kNames[i],kLabels[i],143,NULL);
			nctrl->SetDivider(kDivider);
			bg->AddChild(nctrl);
		}else if(i == 4){
			pctrl = new PassControl(rect,kNames[i],kLabels[i],"",NULL);
			pctrl->SetDivider(kDivider);
			bg->AddChild(pctrl);	
		}else{
			ctrl = new BTextControl(rect,kNames[i],kLabels[i],"",NULL);
			ctrl->SetDivider(kDivider);
			if(i == 5)
				ctrl->SetText("INBOX");
			bg->AddChild(ctrl);
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
	if(strcmp(name,"password") == 0)
	{
		PassControl *ctrl = cast_as(FindView(name),PassControl);
		if(ctrl)
			return ctrl->actualText();
	}else{
		BTextControl *ctrl = cast_as(FindView(name),BTextControl);
		if(ctrl)
			return ctrl->Text();
	}
	return NULL;
}

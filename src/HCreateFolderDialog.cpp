#include "HCreateFolderDialog.h"
#include "Utilities.h"
#include "HApp.h"

#include <View.h>
#include <stdlib.h>
#include <Application.h>
#include <Button.h>
#include <String.h>
#include <Alert.h>
#include <Beep.h>
#include <Directory.h>
#include <Path.h>


enum{
	M_OK_MSG = 'mOKM'
};


/***********************************************************
 * Constructor
 ***********************************************************/
HCreateFolderDialog::HCreateFolderDialog(BRect rect,
									const char *title,
									const char* path)
	:BWindow(rect
			,title
			,B_FLOATING_WINDOW_LOOK
			,B_MODAL_APP_WINDOW_FEEL
			,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
	,fParentPath(NULL)
{
	InitGUI();
	fParentPath = ::strdup(path);
	fNameControl->MakeFocus(true);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HCreateFolderDialog::~HCreateFolderDialog()
{
	free(fParentPath);
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HCreateFolderDialog::InitGUI()
{
	BRect rect = Bounds();
	BView *bgview = new BView(rect,"bgview",B_FOLLOW_ALL,B_WILL_DRAW);
	bgview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	rect.InsetBy(10,10);
	rect.bottom = rect.top + 20;
	fNameControl = new BTextControl(rect,"name",_("Name:"),NULL,NULL);
	fNameControl->SetDivider(fNameControl->StringWidth(_("Name:"))+3);
	DisallowMetaKeys(fNameControl->TextView());
	DisallowFilenameKeys(fNameControl->TextView());
	bgview->AddChild(fNameControl);
	
	rect.OffsetBy(0,rect.Height() + 5);
	BButton *button;
	button = new BButton(rect,"ok",_("OK"),new BMessage(M_OK_MSG));
	button->ResizeToPreferred();
	rect = button->Frame();
	button->MoveBy(bgview->Bounds().Width() - rect.Width()-20,0);
	bgview->AddChild(button);
	
	rect = button->Frame();
	rect.OffsetBy(-(rect.Width()+5),0);
	button = new BButton(rect,"cancel",_("Cancel"),new BMessage(B_QUIT_REQUESTED));
	bgview->AddChild(button);
	
	
	AddChild(bgview);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HCreateFolderDialog::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_OK_MSG:
	{
		BPath path(fParentPath);
		path.Append(fNameControl->Text());
		if(path.InitCheck() != B_OK)
		{
			BString string(_("Invalid path"));
			string += ": ";
			string += fParentPath;
			string += "/";
			string += fNameControl->Text();
			beep();
			(new BAlert("",string.String(),_("OK")))->Go();
			break;
		}
		::create_directory(path.Path(),0777);
		this->PostMessage(B_QUIT_REQUESTED);
		break;
	}
	default:
		BWindow::MessageReceived(message);
	}
}
#include "HSmtpClientView.h"
#include "HMailItem.h"
#include "SmtpLooper.h"
#include "HApp.h"

#include <Font.h>
#include <Window.h>
#include <Debug.h>
#include <Beep.h>
#include <Alert.h>
#include <PopUpMenu.h>
#include <MenuItem.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HSmtpClientView::HSmtpClientView(BRect frame,const char* name)
	:_inherited(frame,name)
	,fIsRunning(false)
	,fSmtpLooper(NULL)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
HSmtpClientView::~HSmtpClientView()
{
	if(fSmtpLooper)
		fSmtpLooper->PostMessage(B_QUIT_REQUESTED);
}


/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HSmtpClientView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_SMTP_ABORT:
		Cancel();
		break;
	// Send mails
	case M_SEND_MAIL:
	{
		if(!fSmtpLooper)
			fSmtpLooper = new SmtpLooper(this,Window());
		fIsRunning = true;
		//int32 count;
		//type_code type;
		//message->GetInfo("pointer",&type,&count);
		message->what = M_SMTP_CONNECT;
		fSmtpLooper->PostMessage(message);
		StopProgress();
		StartBarberPole();
		SetText(_("Sending Mail" B_UTF8_ELLIPSIS));
		break;
	}
	// End of smtp session
	case M_SMTP_END:
	{
		fIsRunning = false;
		StopBarberPole();
		StopProgress();
		SetText("");
		if(fSmtpLooper)
			fSmtpLooper->PostMessage(B_QUIT_REQUESTED);
		fSmtpLooper = NULL;
		PRINT(("SMTP END\n"));
		break;
	}
	// Smtp session error
	case M_SMTP_ERROR:
	{
		BString err(_("SMTP ERROR"));
		err << "\n" << message->FindString("log");
		beep();
		(new BAlert("",err.String(),"OK",NULL,NULL,B_WIDTH_AS_USUAL
											,B_STOP_ALERT))->Go();
		break;
	}
	// Set Max Size
	case M_SET_MAX_SIZE:
	{
		StopBarberPole();
		int32 max_size;
		if(message->FindInt32("max_size",&max_size) != B_OK)
			break;
		SetMaxValue(max_size);
		SetValue(0);
		StartProgress();
		break;
	}
	// Send size
	case M_SEND_MAIL_SIZE:
	{
		int32 size;
		if(message->FindInt32("size",&size) != B_OK)
			break;
		Update(size);
		break;
	}
	default:
		_inherited::MessageReceived(message);
	}
}

/***********************************************************
 * Cancel
 ***********************************************************/
void
HSmtpClientView::Cancel()
{
	fSmtpLooper->ForceQuit();
	
	StopBarberPole();
	StopProgress();
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HSmtpClientView::MouseDown(BPoint pos)
{
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons); 
	if(fIsRunning)
    {	 
    	BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
    	BFont font(be_plain_font);
    	font.SetSize(10);
    	theMenu->SetFont(&font);
    	
    	theMenu->AddItem(new BMenuItem(_("Abort"),new BMessage(M_SMTP_ABORT)));
    	
    	
    	BRect r;
        ConvertToScreen(&pos);
        r.top = pos.y - 5;
        r.bottom = pos.y + 5;
        r.left = pos.x - 5;
        r.right = pos.x + 5;
    	
    	BMenuItem *theItem = theMenu->Go(pos, false,true,r);  
    	if(theItem)
    	{
    	 	BMessage*	aMessage = theItem->Message();
			if(aMessage)
				this->Window()->PostMessage(aMessage,this);
	 	} 
	 	delete theMenu;
    }else
    	_inherited::MouseDown(pos);
}
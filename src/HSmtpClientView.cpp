#include "HSmtpClientView.h"
#include "ResourceUtils.h"
#include "HMailItem.h"
#include "SmtpClient.h"

#include <Font.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Region.h>
#include <StringView.h>
#include <Window.h>
#include <Debug.h>
#include <Beep.h>
#include <Alert.h>

#define DIVIDER 120

const rgb_color kLightGray = {150, 150, 150, 255};
const rgb_color kGray = {100, 100, 100, 255};
const rgb_color kBlack = {0,0,0,255};
const rgb_color kWhite = {255,255,255,255};

/***********************************************************
 * Constructor
 ***********************************************************/
HSmtpClientView::HSmtpClientView(BRect frame,const char* name)
	:BView(frame,name,B_FOLLOW_ALL,B_WILL_DRAW|B_PULSE_NEEDED)
	,fBarberPoleBits(NULL)
	,fLastBarberPoleOffset(0)
	,fShowingBarberPole(false)
	,fShowingProgress(false)
	,fMaxValue(0)
	,fCurrentValue(0)
	,fIsRunning(false)
	,fSmtpClient(NULL)
{
	BFont font(be_fixed_font);
	font.SetSize(10);
	SetFont(&font);
	
	BRect rect(Bounds());
	rect.left += DIVIDER;
	
	fStringView = new BStringView(rect,"","",B_FOLLOW_BOTTOM|B_FOLLOW_LEFT_RIGHT);
	AddChild(fStringView);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	//StartBarberPole();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HSmtpClientView::~HSmtpClientView()
{
	delete fBarberPoleBits;
	if(fSmtpClient)
		fSmtpClient->PostMessage(B_QUIT_REQUESTED);
}


/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HSmtpClientView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	// Send mails
	case M_SEND_MAIL:
	{
		if(!fSmtpClient)
			fSmtpClient = new SmtpClient(this,Window());
		fIsRunning = true;
		int32 count;
		type_code type;
		message->GetInfo("pointer",&type,&count);
		message->what = M_SMTP_CONNECT;
		fSmtpClient->PostMessage(message);
		StopProgress();
		StartBarberPole();
		fStringView->SetText("Sending Mails…");
		break;
	}
	// End of smtp session
	case M_SMTP_END:
	{
		fIsRunning = false;
		StopBarberPole();
		StopProgress();
		fStringView->SetText("");
		if(fSmtpClient)
			fSmtpClient->PostMessage(B_QUIT_REQUESTED);
		fSmtpClient = NULL;
		PRINT(("SMTP END\n"));
		break;
	}
	// Smtp session error
	case M_SMTP_ERROR:
	{
		BString err("SMTP ERROR\n");
		err << message->FindString("log");
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
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * Draw
 ***********************************************************/
void
HSmtpClientView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
	// show barber pole
	if(fShowingBarberPole || fShowingProgress)
	{
		BRect barberPoleRect = BarberPoleOuterRect();
		
		
		BeginLineArray(4);
		AddLine(barberPoleRect.LeftTop(), barberPoleRect.RightTop(), kLightGray);
		AddLine(barberPoleRect.LeftTop(), barberPoleRect.LeftBottom(), kLightGray);
		AddLine(barberPoleRect.LeftBottom(), barberPoleRect.RightBottom(), kWhite);
		AddLine(barberPoleRect.RightBottom(), barberPoleRect.RightTop(), kWhite);
		EndLineArray();
		
		barberPoleRect.InsetBy(1, 1);
	
		if(!fBarberPoleBits)
			fBarberPoleBits= ResourceUtils().GetBitmapResource('BBMP',"RedLongBarberPole");
		BRect destRect(fBarberPoleBits ? fBarberPoleBits->Bounds() : BRect(0, 0, 0, 0));
		destRect.OffsetTo(barberPoleRect.LeftTop() - BPoint(0, fLastBarberPoleOffset));
		fLastBarberPoleOffset -= 1;
		if (fLastBarberPoleOffset < 0)
			fLastBarberPoleOffset = 5;
		BRegion region;
		region.Set(BarberPoleInnerRect());
		ConstrainClippingRegion(&region);	

		if (fBarberPoleBits && fShowingBarberPole)
			DrawBitmap(fBarberPoleBits, destRect);
		if (fShowingProgress)
		{
			SetHighColor(255,0,0);
			float width = destRect.Width();
			if(fMaxValue)
				destRect.right = destRect.left + width * (fCurrentValue/fMaxValue);
			FillRect( destRect);
		}
	}
}

/***********************************************************
 * Return barber pole inner rect
 ***********************************************************/
BRect 
HSmtpClientView::BarberPoleInnerRect() const
{
	BRect result = Bounds();
	result.InsetBy(3, 3);
	result.right = result.left+ DIVIDER-20;
	result.bottom = result.top + 4;
	return result;
}

/***********************************************************
 * Return barber pole outer rect
 ***********************************************************/
BRect 
HSmtpClientView::BarberPoleOuterRect() const
{
	BRect result(BarberPoleInnerRect());
	result.InsetBy(-1, -1);
	return result;
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HSmtpClientView::Pulse()
{
	if (!fShowingBarberPole)
		return;
	Invalidate(BarberPoleOuterRect());
}

/***********************************************************
 * Start BarberPole Animation
 ***********************************************************/
void
HSmtpClientView::StartBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = true;
	Invalidate();
}

/***********************************************************
 * Stop BarberPole Animation
 ***********************************************************/
void
HSmtpClientView::StopBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = false;
	Invalidate();
}

/***********************************************************
 * Update
 ***********************************************************/
void
HSmtpClientView::Update(float delta)
{
	fCurrentValue+=delta;
	Invalidate();
}

/***********************************************************
 * SetValue
 ***********************************************************/
void
HSmtpClientView::SetValue(float value)
{
	fCurrentValue = value;
	Invalidate();
}


/***********************************************************
 * Cancel
 ***********************************************************/
void
HSmtpClientView::Cancel()
{
	fSmtpClient->ForceQuit();
}
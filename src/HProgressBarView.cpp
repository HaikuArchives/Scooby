#include "HProgressBarView.h"
#include "ResourceUtils.h"

#include <Autolock.h>
#include <Region.h>
#include <Window.h>

#define DIVIDER 120

const rgb_color kLightGray = {150, 150, 150, 255};
const rgb_color kGray = {100, 100, 100, 255};
const rgb_color kBlack = {0,0,0,255};
const rgb_color kWhite = {255,255,255,255};

/***********************************************************
 * Constructor
 ***********************************************************/
HProgressBarView::HProgressBarView(BRect frame,const char* name)
	:BView(frame,name,B_FOLLOW_ALL,B_WILL_DRAW|B_PULSE_NEEDED)
	,fLastBarberPoleOffset(0)
	,fShowingBarberPole(false)
	,fShowingProgress(false)
	,fBarberPoleBits(NULL)
	,fMaxValue(0)
	,fCurrentValue(0)
{
	BFont font(be_fixed_font);
	font.SetSize(10);
	SetFont(&font);
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect rect(Bounds());
	rect.left += DIVIDER;
	
	fStringView = new BStringView(rect,"","",B_FOLLOW_BOTTOM|B_FOLLOW_LEFT_RIGHT);
	AddChild(fStringView);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HProgressBarView::~HProgressBarView()
{
	delete fBarberPoleBits;
}

/***********************************************************
 * Draw
 ***********************************************************/
void
HProgressBarView::Draw(BRect updateRect)
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
			fBarberPoleBits= ResourceUtils().GetBitmapResource('BBMP',"LongBarberPole");
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
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
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
HProgressBarView::BarberPoleInnerRect() const
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
HProgressBarView::BarberPoleOuterRect() const
{
	BRect result(BarberPoleInnerRect());
	result.InsetBy(-1, -1);
	return result;
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HProgressBarView::Pulse()
{
	if (!fShowingBarberPole)
		return;
	Invalidate(BarberPoleOuterRect());
}

/***********************************************************
 * Start BarberPole Animation
 ***********************************************************/
void
HProgressBarView::StartBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = true;
	Invalidate();
}

/***********************************************************
 * Stop BarberPole Animation
 ***********************************************************/
void
HProgressBarView::StopBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = false;
	Invalidate();
}

/***********************************************************
 * Update
 ***********************************************************/
void
HProgressBarView::Update(float delta)
{
	fCurrentValue+=delta;
	Invalidate();
}

/***********************************************************
 * SetValue
 ***********************************************************/
void
HProgressBarView::SetValue(float value)
{
	fCurrentValue = value;
	Invalidate();
}

/***********************************************************
 * SetText
 ***********************************************************/
void
HProgressBarView::SetText(const char *text)
{
	fStringView->SetText(text);
}
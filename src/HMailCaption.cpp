#include "HMailCaption.h"
#include <Autolock.h>
#include <Window.h>
#include <String.h>
#include <Region.h>
#include <iostream>
#include "HApp.h"
#include "ResourceUtils.h"

/***********************************************************
 * Constructor.
 ***********************************************************/
HMailCaption::HMailCaption(BRect rect,const char* name,BListView *target)
	:BView(rect,name,B_FOLLOW_LEFT|B_FOLLOW_BOTTOM,B_WILL_DRAW|B_PULSE_NEEDED)
	,fTarget(target)
	,fLastBarberPoleOffset(0)
	,fShowingBarberPole(false)
	,fBarberPoleBits(NULL)
{
	this->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect rect1 = rect;
	rect1.OffsetTo(B_ORIGIN);
	
	//
	rect1.top += 2;
	rect1.bottom = rect1.top + 11;
	rect1.left = rect.left + 1;
	rect1.right = Bounds().right - BarberPoleOuterRect().Width() - 5;
	
	view = new BStringView(rect1,"",_("no items"));
	view->SetAlignment(B_ALIGN_LEFT);
	this->AddChild(view);
	//this->Draw(this->Bounds());
	fOld = 0;
	
	BFont font;
	view->GetFont(&font);
	font.SetSize(10);
	view->SetFont(&font);
}

/***********************************************************
 * Destructor.
 ***********************************************************/
HMailCaption::~HMailCaption()
{
	delete fBarberPoleBits;
}

const rgb_color kLightGray = {150, 150, 150, 255};
const rgb_color kGray = {100, 100, 100, 255};
const rgb_color kBlack = {0,0,0,255};
const rgb_color kWhite = {255,255,255,255};

/***********************************************************
 * Draw
 ***********************************************************/
void
HMailCaption::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
	// show barber pole
	if(fShowingBarberPole)
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
			fBarberPoleBits= ResourceUtils().GetBitmapResource('BBMP',"BarberPole");
		BRect destRect(fBarberPoleBits ? fBarberPoleBits->Bounds() : BRect(0, 0, 0, 0));
		destRect.OffsetTo(barberPoleRect.LeftTop() - BPoint(0, fLastBarberPoleOffset));
		fLastBarberPoleOffset -= 1;
		if (fLastBarberPoleOffset < 0)
			fLastBarberPoleOffset = 5;
		BRegion region;
		region.Set(BarberPoleInnerRect());
		ConstrainClippingRegion(&region);	

		if (fBarberPoleBits)
			DrawBitmap(fBarberPoleBits, destRect);
	}
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HMailCaption::Pulse()
{
	if(fTarget!= NULL)
	{
		int32 num = fTarget->CountItems();
		if(num != fOld){
			this->SetNumber(num);
			fOld = num;
		}
	}
	if (!fShowingBarberPole)
		return;
	Invalidate(BarberPoleOuterRect());
}

/***********************************************************
 * Set number.
 ***********************************************************/
void
HMailCaption::SetNumber(int32 num)
{
	BAutolock lock(Window());

	BString str("");
	if(num == 1)
		str << num << " "<< _("item");
	else if(num == 0)
		str = _("no items");
	else
		str << num << " "<< _("items");
	view->SetText(str.String());
}

/***********************************************************
 * Start BarberPole Animation
 ***********************************************************/
void
HMailCaption::StartBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = true;
	Invalidate();
}

/***********************************************************
 * Stop BarberPole Animation
 ***********************************************************/
void
HMailCaption::StopBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = false;
	Invalidate();
}

/***********************************************************
 * Return barber pole inner rect
 ***********************************************************/
BRect 
HMailCaption::BarberPoleInnerRect() const
{
	BRect result = Bounds();
	result.InsetBy(3, 3);
	result.left = result.right - 7;
	result.bottom = result.top + 6;
	return result;
}

/***********************************************************
 * Return barber pole outer rect
 ***********************************************************/
BRect 
HMailCaption::BarberPoleOuterRect() const
{
	BRect result(BarberPoleInnerRect());
	result.InsetBy(-1, -1);
	return result;
}

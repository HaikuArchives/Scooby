#include "HStatusView.h"
#include <Autolock.h>
#include <Window.h>
#include <String.h>
#include <Region.h>
#include <ClassInfo.h>
#include "ResourceUtils.h"

#include "HFolderList.h"

/***********************************************************
 * Constructor.
 ***********************************************************/
HStatusView::HStatusView(BRect rect,const char* name,BListView *target)
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
	rect1.top += 1;
	rect1.bottom = rect1.top + 10;
	rect1.left = rect.left + 1;
	rect1.right = Bounds().right - BarberPoleOuterRect().Width() - 5;
	
	view = new BStringView(rect1,"","0 items");
	view->SetAlignment(B_ALIGN_RIGHT);
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
HStatusView::~HStatusView()
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
HStatusView::Draw(BRect updateRect)
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
HStatusView::Pulse()
{
	if(fTarget!= NULL)
	{
		int32 num = fTarget->CountItems();
		bool dirty = false;
		if(num != fOld)
			dirty = true;
		
		BString str("");
		/*	
		if(dirty)
			str = fStatus;
		HFolderList *list = cast_as(fTarget,HFolderList);
		
		int32 sel = list->CurrentSelection();
		if(sel>=0 && fOldSel != sel)
		{
			HFolderItem *item = cast_as(list->ItemAt(sel),HFolderItem);
			if(item)
			{
				int32 unread = item->Unread();
				int32 all = item->CountMails();
				str = "Total:";
				str << all << "  Unread:" << unread;
				fOldSel = sel;
				fStatus = str;
				dirty = true;
			}
		}
		*/
		if(dirty)
			SetCaption(num,str.String());
	}
	if (!fShowingBarberPole)
		return;
	Invalidate(BarberPoleOuterRect());
}

/***********************************************************
 * Set number.
 ***********************************************************/
void
HStatusView::SetCaption(int32 num,const char* text)
{
	BString str("");
	str <<text << "     ";	
	if(num == 1)
		str << num << " folder";
	else if(num == 0)
		str = "no folders";
	else
		str << num << " folders";
		
	
	view->SetText(str.String());
}

/***********************************************************
 * Start BarberPole Animation
 ***********************************************************/
void
HStatusView::StartBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = true;
	Invalidate();
}

/***********************************************************
 * Stop BarberPole Animation
 ***********************************************************/
void
HStatusView::StopBarberPole()
{
	BAutolock lock(Window());
	fShowingBarberPole = false;
	Invalidate();
}

/***********************************************************
 * Return barber pole inner rect
 ***********************************************************/
BRect 
HStatusView::BarberPoleInnerRect() const
{
	BRect result = Bounds();
	result.InsetBy(3, 2);
	result.left = result.right - 7;
	result.bottom = result.top + 6;
	return result;
}

/***********************************************************
 * Return barber pole outer rect
 ***********************************************************/
BRect 
HStatusView::BarberPoleOuterRect() const
{
	BRect result(BarberPoleInnerRect());
	result.InsetBy(-1, -1);
	return result;
}
#include "StatusItem.h"
#include "StatusBar.h"

#include <Font.h>
#include <Debug.h>

/***********************************************************
 * Constructor
 ***********************************************************/
StatusItem::StatusItem(BRect rect,
					const char* name,
					const char* initialText,
					void	(*func)(StatusItem* item))
	:BView(rect,NULL,B_FOLLOW_LEFT|B_FOLLOW_BOTTOM,B_WILL_DRAW|B_PULSE_NEEDED)
	,fLabel(initialText)
{
	fBackgroundColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	fDark_1_color = tint_color(fBackgroundColor,B_DARKEN_1_TINT);
	fDark_2_color = tint_color(fBackgroundColor,B_DARKEN_2_TINT);
	fCachedBounds = Bounds();

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	pulseFunc = func;
	BFont font(be_plain_font);
	font.SetSize(9);
	font_height fheight;
	font.GetHeight(&fheight);
	fFontHeight = ceil(fheight.ascent)+ceil(fheight.descent);
	SetFont(&font);
}

/***********************************************************
 * Draw
 ***********************************************************/
void
StatusItem::Draw(BRect updateRect)
{
	// Draw bevel
	SetHighColor(fDark_1_color);
	StrokeLine(BPoint(fCachedBounds.left,fCachedBounds.top),BPoint(fCachedBounds.right,
		fCachedBounds.top));
	StrokeLine(BPoint(fCachedBounds.left,fCachedBounds.top+1),BPoint(fCachedBounds.left,
		fCachedBounds.bottom));
	SetHighColor(255,255,255,255);
	StrokeLine(BPoint(fCachedBounds.right,fCachedBounds.top+1),BPoint(fCachedBounds.right,
		fCachedBounds.bottom-1));
	StrokeLine(BPoint(fCachedBounds.left+1,fCachedBounds.bottom),BPoint(fCachedBounds.right,
		fCachedBounds.bottom));
	SetHighColor(fDark_2_color);
	StrokeLine(BPoint(fCachedBounds.left+1,fCachedBounds.top+1),BPoint(fCachedBounds.right-1,
		fCachedBounds.top+1));
	StrokeLine(BPoint(fCachedBounds.left+1,fCachedBounds.top+2),BPoint(fCachedBounds.left+1,
		fCachedBounds.bottom-1));
	SetHighColor(fBackgroundColor);
	StrokeLine(BPoint(fCachedBounds.left+2,fCachedBounds.bottom-1),BPoint(fCachedBounds.right-1,
		fCachedBounds.bottom-1));
	StrokeLine(BPoint(fCachedBounds.right-1,fCachedBounds.top+2),BPoint(fCachedBounds.right-1,
		fCachedBounds.bottom-2));
	// Draw label	
	SetDrawingMode(B_OP_COPY);
	SetHighColor(0,0,0);
	SetLowColor(ViewColor());
	DrawString(fLabel.String(),BPoint(5,fFontHeight-3));	
}

/***********************************************************
 * ResizeToPreferred
 ***********************************************************/
void
StatusItem::ResizeToPreferred()
{
	float width = StringWidth(fLabel.String())+10;
	
	float oldWidth = Bounds().Width();
	
	if(oldWidth != width)
	{
		ResizeBy(width-oldWidth,0);
		StatusBar *bar = (StatusBar*)Parent();
		bar->RearrangeItems(this,width-oldWidth);
	}
}

/***********************************************************
 * SetLabel
 ***********************************************************/
void
StatusItem::SetLabel(const char* label)
{
	if(::strcmp(label,fLabel.String()) != 0)
	{
		fLabel = label;
		Invalidate();
	}
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
StatusItem::Pulse()
{
	if(pulseFunc)
		pulseFunc(this);
}

void
StatusItem::FrameResized(float width, float height)
{
	BRect new_bounds = Bounds();
	float min_x = new_bounds.right;
	if(min_x > fCachedBounds.right)
		min_x = fCachedBounds.right;
	float max_x = new_bounds.right;
	if(max_x < fCachedBounds.right)
		max_x = fCachedBounds.right;
	float min_y = new_bounds.bottom;
	if(min_y > fCachedBounds.bottom)
		min_y = fCachedBounds.bottom;
	float max_y = new_bounds.bottom;
	if(max_y < fCachedBounds.bottom)
		max_y = fCachedBounds.bottom;
	if(min_x != max_x)
		Invalidate(BRect(min_x-1,new_bounds.top,max_x,max_y));
	if(min_y != max_y)
		Invalidate(BRect(new_bounds.left,min_y-1,max_x,max_y));
}

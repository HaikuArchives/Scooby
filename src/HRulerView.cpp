#include "HRulerView.h"

#include <Debug.h>
#include <Polygon.h>

const rgb_color kRulerBorderColor = ui_color(B_PANEL_BACKGROUND_COLOR);
const rgb_color kRed={255,0,0,255};

/***********************************************************
 *
 ***********************************************************/
HRulerView::HRulerView(BRect rect,const char* name,HWrapTextView *view)
	:BView(rect,name,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,B_WILL_DRAW|B_PULSE_NEEDED)
	,fTextView(view)
	,fCaretPosition(4.0F)
	,fDragging(false)
{
	SetLowColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));
	SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));

	BFont font;
	font.SetFamilyAndStyle("Courier 10 BT","Roman");
	font.SetSize(9);
	SetFont(&font);
	FontReseted();
	fOldCaretRect.Set(3.0F,Bounds().top,5.0F,Bounds().bottom);
}

/***********************************************************
 *
 ***********************************************************/
void
HRulerView::Draw(BRect /*rect*/)
{
	//if(fFont.IsFixed())
		DrawRuler();
	float rightLimit = fTextView->RightLimit();
	// Draw caret
	SetHighColor(kRed);
	//BRect caretRect(fCaretPosition-1,Bounds().top+9,fCaretPosition,Bounds().bottom);
	//FillRect(caretRect);
	StrokeLine(BPoint(fCaretPosition,Bounds().top+7),BPoint(fCaretPosition,Bounds().bottom));
	// Draw max pos mark
	BPoint points[3];
	points[0].Set(rightLimit-4,Bounds().top+9);
	points[1].Set(rightLimit+4,Bounds().top+9);
	points[2].Set(rightLimit,Bounds().bottom-1);
	BPolygon polygon(points,3);
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_MAX_TINT));
	FillPolygon(&polygon);
	// Draw border
	SetHighColor(kRulerBorderColor);
	StrokeLine(BPoint(0,Bounds().bottom),BPoint(Bounds().right,Bounds().bottom));
}

/***********************************************************
 * DrawRuler
 ***********************************************************/
void
HRulerView::DrawRuler()
{
	rgb_color black = {0,0,0,255};
	BRect bounds(Bounds());
	float width = (bounds.Width()>fTextView->RightLimit())?bounds.Width():fTextView->RightLimit();
	width -= -4.0F;
	int32 count = (int32)floor(width/fFontWidth);
	
	BeginLineArray(count);
	
	float start = 4.0F;
	BPoint pos;
	char buf[10];
	pos.y = bounds.top + 7;
	for(int32 i = 0;i < count;i++)
	{
		AddLine(BPoint(start,(i%10)?bounds.top+13:bounds.top+8),BPoint(start,bounds.bottom),black);
		if(!(i%10))
		{
			pos.x = start-3;
#ifdef __INTEL__			
			::snprintf(buf,9,"%ld",i);
#else
			::sprintf(buf,"%ld",i);
#endif
			DrawString(buf,pos);
		}
		start += fFontWidth;
	}
	
	EndLineArray();
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HRulerView::MouseDown(BPoint pos)
{
	float rightLimit = fTextView->RightLimit();
	
	BRect arrowRect;
	arrowRect.Set(rightLimit-4,Bounds().top+8,rightLimit+4,Bounds().bottom);
	if(arrowRect.Contains(pos))
	{
		fDragging = true;
		SetMouseEventMask(B_POINTER_EVENTS,B_NO_POINTER_HISTORY);
	}
}

/***********************************************************
 * MouseUp
 ***********************************************************/
void
HRulerView::MouseUp(BPoint pos)
{
	fDragging = false;
}

/***********************************************************
 * MouseMoved
 ***********************************************************/
void
HRulerView::MouseMoved(BPoint point, uint32 code, const BMessage* message)
{
	if(fDragging)
	{
		if(code == B_INSIDE_VIEW)
		{
			fTextView->SetRightLimit(point.x);
			Invalidate();
		}
	}
}

/***********************************************************
 * FontReseted
 ***********************************************************/
void
HRulerView::FontReseted()
{
	fTextView->GetFontAndColor(0,&fFont);
	int c = 'A';
	fFontWidth = fFont.StringWidth((char*)&c);
	Invalidate();
}

/***********************************************************
 * Pulse
 ***********************************************************/
void
HRulerView::Pulse()
{
	int32 line_offset = fTextView->OffsetAt(fTextView->CurrentLine());
	int32 start,end;
	fTextView->GetSelection(&start,&end);

	int32 size= start-line_offset;
	if(size <= 0)
		return;
		
	char *buf = new char[size+1];
	if(!buf)
		return;
	fTextView->GetText(line_offset,size,buf);// GetText adds a NULL.
	float pos = fFont.StringWidth(buf)+4.0;
	if(pos != fCaretPosition)
	{
		fCaretPosition = pos;
		//Invalidate old caret rect.
		Invalidate(fOldCaretRect);
		//Invalidate new caret rect. 
		fOldCaretRect.left = fCaretPosition-1;
		fOldCaretRect.right = fCaretPosition+1;
		Invalidate(fOldCaretRect);
	}
	delete[] buf;
}
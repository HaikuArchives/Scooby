#include "HWrapTextView.h"
#include "HRulerView.h"

#include <Debug.h>

#define RULER_HEIGHT 16

/***********************************************************
 * Constructor
 ***********************************************************/
HWrapTextView::HWrapTextView(BRect rect,const char* name,int32 resize,int32 flag)
		:_inherited(rect,name,BRect(4.0,4.0,rect.right-rect.left-4.0,rect.bottom-rect.top-4.0),resize,flag)
		,fUseRuler(false)
		,fRulerView(NULL)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
HWrapTextView::~HWrapTextView()
{

}

/***********************************************************
 * AddRuler
 ***********************************************************/
void
HWrapTextView::AddRuler()
{
	if(!fRulerView)
	{
		BRect rect = TextRect();
		rect.top += RULER_HEIGHT;
		rect = Bounds();
		rect.bottom = rect.top + RULER_HEIGHT;
		fRulerView = new HRulerView(rect,"ruler",this);
		AddChild(fRulerView);
		ResetTextRect();
	}
}

/***********************************************************
 * RemoveRuler
 ***********************************************************/
void
HWrapTextView::RemoveRuler()
{
	if(fRulerView)
	{
		fRulerView->RemoveSelf();
		delete fRulerView;
		fRulerView=NULL;
		ResetTextRect();
	}
}

/***********************************************************
 * UseRuler
 ***********************************************************/
void
HWrapTextView::UseRuler(bool use)
{
	if(use == fUseRuler)
		return;
	fUseRuler = use;
	if(fUseRuler)
		AddRuler();
	else
		RemoveRuler();
}

/***********************************************************
 * FrameResized
 ***********************************************************/
void
HWrapTextView::FrameResized(float width, float height)
{
	ResetTextRect();
	if(fUseRuler)
	{
		BRect rect= fRulerView->Bounds();
		fRulerView->ResizeTo(width,rect.Height());
		
		rect.left= rect.right - 50;
		rect.right = width;
		fRulerView->Invalidate(rect);
	}
	_inherited::FrameResized(width,height);
}

/***********************************************************
 * SetRightLimit
 ***********************************************************/
void
HWrapTextView::SetRightLimit(float limit)
{
	if(fRightLimit != limit)
	{
		fRightLimit = limit;
		ResetTextRect();
	}
}
/***********************************************************
 * ResetTextRect
 ***********************************************************/
void
HWrapTextView::ResetTextRect()
{
	BRect textRect = Bounds();
	textRect.left = 4.0;
	textRect.top = 4.0;
	textRect.right -= 4.0;
	textRect.bottom -= 4.0;
	if(fUseRuler)
	{
		textRect.top += RULER_HEIGHT;
		textRect.right = fRightLimit;
	}
	SetTextRect(textRect);
}

/***********************************************************
 * SetFontAndColor
 ***********************************************************/
void
HWrapTextView::SetFontAndColor(const BFont		*inFont, 
								uint32			inMode,
								const rgb_color	*inColor)
{
	if(fRulerView)
		fRulerView->FontReseted();
	_inherited::SetFontAndColor(inFont,inMode,inColor);
}

/***********************************************************
 * SetFontAndColor
 ***********************************************************/
void
HWrapTextView::SetFontAndColor(int32			startOffset, 
								int32			endOffset, 
								const BFont		*inFont,
								uint32			inMode ,
								const rgb_color	*inColor)
{
	if(fRulerView)
		fRulerView->FontReseted();
	_inherited::SetFontAndColor(startOffset,endOffset,inFont,inMode,inColor);
}
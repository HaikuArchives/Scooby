//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <string.h>
#include <Region.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "HTabView.h"
#include "Colors.h"


//******************************************************************************************************
//**** TabView CLASS DEFINITION
//******************************************************************************************************
HTabView::HTabView(BRect frame, const char *name, const char** names, int32 number_of_views,
	uint32 resizingMode, uint32 flags, const BFont *labelFont)
: BView(frame, name, resizingMode, flags)
{
	fNames = new char*[number_of_views];
	fViews = new BView*[number_of_views];
	fEnabled = new bool[number_of_views];
	int32 Counter;
	
	for(Counter = 0; Counter < number_of_views; Counter++)
		fEnabled[Counter] = true;
	
	for(Counter = 0; Counter < number_of_views; Counter++)
	{
		fNames[Counter] = new char[strlen(names[Counter])+1];
		strcpy(fNames[Counter],names[Counter]);
		fViews[Counter] = NULL;
	}
	fNumberOfViews = number_of_views;
	SetFont(labelFont);
	SetViewColor(BeBackgroundGrey);
	font_height FontAttributes;
	labelFont->GetHeight(&FontAttributes);
	float LabelFontAscent = ceil(FontAttributes.ascent);
	float LabelFontHeight = LabelFontAscent + ceil(FontAttributes.descent);
	float TabViewBottom = 5.0 +LabelFontHeight+ 5.0;
	fTabRect = new BRect[fNumberOfViews];
	fLabelRect = new BRect[fNumberOfViews];
	fLabelPosition = new BPoint[fNumberOfViews];
	
	CalcTabsWidth();
	
	fCurrentlyShowing = 0;
	fBounds = Bounds();
	fContentArea.Set(fBounds.left+1.0,TabViewBottom+1.0,fBounds.right-1.0,fBounds.bottom-1.0);
}


float HTabView::GetTabsWidth()
{
	return fTabRect[fNumberOfViews-1].right + 3.0;
}


BRect HTabView::GetContentArea()
{
	return fContentArea;
}


void HTabView::AddViews(BView** tabbed_views)
{
	for(int32 Counter = 0; Counter < fNumberOfViews; Counter++)
	{
		fViews[Counter] = tabbed_views[Counter];
		if(Counter != fCurrentlyShowing)
			fViews[Counter]->Hide();
		AddChild(fViews[Counter]);
	}
}


HTabView::~HTabView()
{
	for(int32 Counter = 0; Counter < fNumberOfViews; Counter++)
		delete[] fNames[Counter];
	delete[] fNames;
	delete[] fViews;
	delete[] fTabRect;
	delete[] fLabelRect;
	delete[] fLabelPosition;
	delete[] fEnabled;
}


void HTabView::Draw(BRect updateRect)
{
	BRegion ClippingRegion;
	GetClippingRegion(&ClippingRegion);
	//Redraw the tabs
	float TabsBottom = fTabRect[0].bottom;
	int32 Counter;
	for(Counter = 0; Counter < fNumberOfViews; Counter++)
	{
		if(ClippingRegion.Intersects(fTabRect[Counter]))
		{
			rgb_color BackgroundColor = ViewColor();
	
			SetHighColor(ViewColor());
			SetLowColor(ViewColor());
		
			FillRect(BRect(fTabRect[Counter].left+1.0,fTabRect[Counter].top+1.0,
				fTabRect[Counter].right-2.0,TabsBottom - (Counter!=fCurrentlyShowing?1.0:0.0)));
			if(ClippingRegion.Intersects(fLabelRect[Counter]))
			{
				if(!fEnabled[Counter])
					SetHighColor(tint_color(Black,B_LIGHTEN_1_TINT));
				else
					SetHighColor(Black);
				DrawString(fNames[Counter],fLabelPosition[Counter]);
			}
			SetHighColor(BeHighlight);
			MovePenTo(BPoint(fTabRect[Counter].left,TabsBottom-(Counter!=0?1.0:0.0)));
			StrokeLine(BPoint(fTabRect[Counter].left,fTabRect[Counter].top+2.0));
			StrokeLine(BPoint(fTabRect[Counter].left+2.0,fTabRect[Counter].top));
			StrokeLine(BPoint(fTabRect[Counter].right-4.0,fTabRect[Counter].top));
			const rgb_color MiddleColor = {190,190,190,255};
			SetHighColor(MiddleColor);
			MovePenTo(BPoint(fTabRect[Counter].right-3.0,fTabRect[Counter].top));
			StrokeLine(BPoint(fTabRect[Counter].right-2.0,fTabRect[Counter].top+1.0));
			SetHighColor(BeShadow);
			MovePenTo(BPoint(fTabRect[Counter].right-1.0,fTabRect[Counter].top+2.0));
			StrokeLine(BPoint(fTabRect[Counter].right-1.0,TabsBottom-1.0));
		}
	}

	//Draw the main view area left border
	SetHighColor(BeHighlight);
	if(updateRect.left <= fBounds.left)
	{
		MovePenTo(fBounds.LeftBottom());
		StrokeLine(BPoint(fBounds.left,TabsBottom+1.0));
	}

	//Draw the top border
	if(updateRect.top <= TabsBottom && updateRect.bottom >= TabsBottom)
	{
		if(fCurrentlyShowing > 0 && fTabRect[fCurrentlyShowing].left >= updateRect.left)
		{
			MovePenTo(BPoint(fBounds.left,TabsBottom));
			StrokeLine(BPoint(fTabRect[fCurrentlyShowing].left,TabsBottom));
		}
		if(fTabRect[fCurrentlyShowing].right <= updateRect.right &&
			fTabRect[fCurrentlyShowing].right-1.0 <= fBounds.right)
		{
			MovePenTo(BPoint(fTabRect[fCurrentlyShowing].right-1.0,TabsBottom));
			StrokeLine(BPoint(fBounds.right,TabsBottom));
		}
	}
	
	//Draw the right border
	SetHighColor(BeShadow);
	if(updateRect.right >= fBounds.right)
	{
		if(fBounds.right >= fTabRect[fNumberOfViews-1].right)
			MovePenTo(BPoint(fBounds.right,TabsBottom+1.0));
		else
			//Figure out where to start drawing right border (top or bottom of tab)
			for(Counter = 0; Counter < fNumberOfViews; Counter++)
			{
				if(fBounds.right <= fTabRect[Counter].right)
				{
					//Position cursor for right boundary
					if(fBounds.right == fTabRect[Counter].left)
						MovePenTo(BPoint(fBounds.right,TabsBottom+1.0));
					else if(fBounds.right == fTabRect[Counter].left+1.0)
						MovePenTo(BPoint(fBounds.right,fTabRect[0].top+2.0));
					else if(fBounds.right <= fTabRect[Counter].right-3.0)
						MovePenTo(BPoint(fBounds.right,fTabRect[0].top+1.0));
					else if(fBounds.right <= fTabRect[Counter].right-2.0)
						MovePenTo(BPoint(fBounds.right,fTabRect[0].top+2.0));
					else if(fBounds.right <= fTabRect[Counter].right-1.0)
						MovePenTo(BPoint(fBounds.right,TabsBottom+1.0));
					else
						MovePenTo(BPoint(fBounds.right,TabsBottom+1.0));
					break;
				}
			}
		StrokeLine(fBounds.RightBottom());
	}
	else if(updateRect.bottom >= fBounds.bottom)
		MovePenTo(fBounds.RightBottom());

	//Draw the bottom border
	if(updateRect.bottom >= fBounds.bottom)
		StrokeLine(BPoint(fBounds.left+1.0,fBounds.bottom));
}


void HTabView::MouseDown(BPoint point)
{
	for(int32 Counter = 0; Counter < fNumberOfViews; Counter++)
		if(fTabRect[Counter].Contains(point))
		{
			if(Counter != fCurrentlyShowing)
				SelectTab(Counter);
			break;
		}
	MakeFocus(true);
}


void HTabView::FrameResized(float width, float height)
{
	float XMin = fBounds.right;
	float XMax = fBounds.right;
	float YMin = fBounds.bottom;
	float YMax = fBounds.bottom;
	fBounds = Bounds();
	if(fBounds.right > XMax)
		XMax = fBounds.right;
	if(fBounds.right < XMin)
		XMin = fBounds.right;
	if(fBounds.bottom > YMax)
		YMax = fBounds.bottom;
	if(fBounds.bottom < YMin)
		YMin = fBounds.bottom;
	if(XMin != XMax)
		Invalidate(BRect(XMin,fBounds.top,XMax,fBounds.bottom));
	if(YMin != YMax)
		Invalidate(BRect(fBounds.left,YMin,fBounds.right,YMax));
	//Get rid of a warning
	width = 0;
	height = 0;
}


void HTabView::SelectTab(int32 index)
{
	if(!fEnabled[index])
		return;
	if(index != fCurrentlyShowing)
	{
		Invalidate(fTabRect[fCurrentlyShowing]);
		fViews[fCurrentlyShowing]->Hide();
		fCurrentlyShowing = index;
		Invalidate(fTabRect[fCurrentlyShowing]);
		fViews[fCurrentlyShowing]->Show();
	}
}

void HTabView::SetTabLabel(int32 index,const char* name)
{
	delete[] fNames[index];
	int32 len = ::strlen(name);
	char *buf = new char[len+1];
	::strcpy(buf,name);
	buf[len] = '\0';
	fNames[index] = buf;
	CalcTabsWidth();
	Invalidate();
}

void HTabView::CalcTabsWidth()
{
	float* Widths = new float[fNumberOfViews];
	int32* LengthArray = new int32[fNumberOfViews];
	for(int32 Counter = 0; Counter < fNumberOfViews; Counter++)
		LengthArray[Counter] = strlen(fNames[Counter]);
		
	BFont labelFont;
	GetFont(&labelFont);
	labelFont.GetStringWidths((const char**)fNames,LengthArray,fNumberOfViews,Widths);

	font_height FontAttributes;
	labelFont.GetHeight(&FontAttributes);
	float LabelFontAscent = ceil(FontAttributes.ascent);
	float LabelFontHeight = LabelFontAscent + ceil(FontAttributes.descent);
	float TabViewBottom = 4.0 +LabelFontHeight+ 4.0;

	delete[] LengthArray;
	float MaxTabsWidth = 0.0;
	for(int32 Counter = 0; Counter < fNumberOfViews; Counter++)
		if(Widths[Counter]+18.0 > MaxTabsWidth)
			MaxTabsWidth = Widths[Counter]+18.0;
	for(int32 Counter = 0; Counter < fNumberOfViews; Counter++)
	{
		fTabRect[Counter].Set((MaxTabsWidth+2.0)*Counter,0.0,(MaxTabsWidth+2.0)*(Counter+1)-1.0,
			TabViewBottom);
		float Offset = floor((MaxTabsWidth-Widths[Counter])/2.0);
		fLabelRect[Counter].Set(fTabRect[Counter].left+Offset,fTabRect[Counter].top+5.0,
			fTabRect[Counter].left+Offset+Widths[Counter],fTabRect[Counter].top+5.0+LabelFontHeight);
		fLabelPosition[Counter].Set(fLabelRect[Counter].left,fLabelRect[Counter].top+LabelFontAscent);
	}
	delete[] Widths;
}

void HTabView::SetTabEnabled(int32 index,bool enable)
{
	if(fEnabled[index] != enable)
	{
		fEnabled[index] = enable;
		Invalidate();
	}
}

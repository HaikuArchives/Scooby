#include "HCoolListView.h"

#include <Debug.h>
#include <ClassInfo.h>

const rgb_color highlight = {200,200,255,100};

/***********************************************************
 * Constructor
 ***********************************************************/
HCoolListView::HCoolListView(BRect frame,
						CLVContainerView** ContainerView,
						const char* Name,
						uint32 ResizingMode,
						uint32 flags,
						list_view_type Type,
						bool hierarchical,
						bool horizontal,
						bool vertical,
						bool scroll_view_corner,
						border_style border,
						const BFont* LabelFont)
	:_inherited(frame,ContainerView,Name,ResizingMode,flags,Type,
					hierarchical,horizontal,vertical,scroll_view_corner,border,LabelFont)
	,fOldSelection(-1)
{
	SetViewColor(tint_color( ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));
}

/***********************************************************
 * Destructor
 ***********************************************************/
HCoolListView::~HCoolListView()
{
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HCoolListView::MouseMoved(BPoint where,uint32 code,const BMessage *message)
{
	_inherited::MouseMoved(where,code,message);
	
	int32 count = CountItems();
	if(count == 0)
		return;
	BRect itemRect = ItemFrame(0);
	float itemHeight = itemRect.Height()+1;
	
	itemRect.OffsetBy(0,-1);
	
	if(fOldSelection >= 0 && (code == B_EXITED_VIEW || code == B_OUTSIDE_VIEW))
	{
		BRect oldRect = itemRect;
		oldRect.OffsetBy(0,itemHeight*fOldSelection);
		fOldSelection = -1;
		Invalidate(oldRect);
		return;
	}
	
	BRect newRect = itemRect;
	for(int32 i = 0;i < count;i++)
	{
		if(newRect.Contains(where))
		{
			if(i == fOldSelection)
				break;
			
			if(fOldSelection >= 0)
			{
				BRect oldRect = itemRect;
				oldRect.OffsetBy(0,itemHeight*fOldSelection);
				fOldSelection = -1;
				Invalidate(oldRect);
			}
			SetHighColor(highlight);
			SetDrawingMode(B_OP_ALPHA);
			FillRect(newRect);
			fOldSelection = i;
			break;
		}
		newRect.OffsetBy(0,itemHeight);
	}	
}
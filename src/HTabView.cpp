#include "HTabView.h"
#include "HHtmlView.h"

#include <Debug.h>
#include <ClassInfo.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HTabView::HTabView(BRect frame, const char *name,
							button_width width,
							uint32 resizingMode,
							uint32 flags)
		:BTabView(frame,name,width,resizingMode,flags)
{

}

/***********************************************************
 * Destructor
 ***********************************************************/
HTabView::~HTabView()
{
}

/***********************************************************
 * Select
 ***********************************************************/
void
HTabView::Select(int32 tab_index)
{
	// Resize assigned views
	BTab *tab = TabAt(tab_index);
	if(!tab->IsEnabled())
		return;
	BTabView::Select(tab_index);
	if(tab)
	{
		BView *view = tab->View();
		if(view)
		{
			BRect rect;
			rect = Bounds();
			
			if(tab_index == 0)
			{
				HHtmlView *htmlview= cast_as(view,HHtmlView);
				// We have to reload file
				if(htmlview)
					htmlview->Reload();
			}	
			if(tab_index != 1)
			{
				rect.right -= 1;
				rect.bottom -= 2;
			}else{
				rect.right += 3;
				rect.bottom += 2;
			}
			rect.bottom -= TabHeight();
			view->ResizeTo(rect.Width(),rect.Height());
		}
	}	
}
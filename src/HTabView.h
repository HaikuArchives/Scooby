#ifndef __HTABVIEW_H__
#define __HTABVIEW_H__


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <View.h>


//******************************************************************************************************
//**** ColumnListView CLASS DECLARATION
//******************************************************************************************************
class HTabView : public BView
{
	public:
		//Constructor and destructor
		HTabView(BRect frame, const char *name, const char** names, int32 number_of_views,
			uint32 resizingMode =  B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_NAVIGABLE | B_WILL_DRAW |
			B_FRAME_EVENTS, const BFont *labelFont = be_plain_font);
		BRect GetContentArea();
		float GetTabsWidth();
		void AddViews(BView** tabbedViews);
		~HTabView();
		//@{
		//!Override function.
				void Draw(BRect updateRect);
				void MouseDown(BPoint point);
				void FrameResized(float width, float height);
				void SelectTab(int32 index);
		//@}
				void SetTabLabel(int32 index,const char* label);
			int32	 Selection() {return fCurrentlyShowing;}
			
				void SetTabEnabled(int32 index,bool enabled);
	protected:
				void CalcTabsWidth();	
	private:
		char** fNames;
		BView** fViews;
		int32 fNumberOfViews;
		float fTabViewBottom;
		BPoint* LabelPosition;
		BRect* LabelRect;
		int32 fCurrentlyShowing;
		BRect* fTabRect;
		BRect* fLabelRect;
		BPoint* fLabelPosition;
		BRect fBounds;
		BRect fContentArea;
		bool*  fEnabled;
};


#endif

#ifndef __HCOOLLISTVIEW_H__
#define __HCOOLLISTVIEW_H__

#include "ColumnListView.h"

//!Hover highlighting list view.
class HCoolListView :public ColumnListView {
public:
				//!Constructor
				HCoolListView(	BRect Frame,
								CLVContainerView** ContainerView,	/*<!Used to get back a pointer to the container
																	view that will hold the ColumnListView, the
																	the CLVColumnLabelView, and the scrollbars.
																	If no scroll bars or border are asked for,
																	this will act like a plain BView container.*/
								const char* Name = NULL,
								uint32 ResizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
								list_view_type Type = B_SINGLE_SELECTION_LIST,
								bool hierarchical = false,
								bool horizontal = true,					//!<Which scroll bars should I add, if any
								bool vertical = true,
								bool scroll_view_corner = true,
								border_style border = B_NO_BORDER,		//!<What type of border to add, if any
								const BFont* LabelFont = be_plain_font);
				//!Destructor.
	virtual		~HCoolListView();
				//!Enable or disable hovering.
	void		SetEnableHovering(bool enable){fHovering=enable;}
				//!Returns hovering was enabled or not.
	bool		DoesHovering()const {return fHovering;}
protected:
				//!Override function.
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage* message);
private:
	typedef ColumnListView	_inherited;
	bool					fHovering;
	int32					fOldSelection;	
};
#endif
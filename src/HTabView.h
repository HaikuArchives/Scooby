#ifndef __HTABVIEW_H__
#define __HTABVIEW_H__

#include <TabView.h>

class HTabView :public BTabView{
public:
						HTabView(BRect frame, const char *name,
							button_width width=B_WIDTH_AS_USUAL,
							uint32 resizingMode = B_FOLLOW_ALL,
							uint32 flags = B_FULL_UPDATE_ON_RESIZE |
								B_WILL_DRAW | B_NAVIGABLE_JUMP |
								B_FRAME_EVENTS | B_NAVIGABLE);
	virtual				~HTabView();
	virtual	void		Select(int32 tab);
};
#endif
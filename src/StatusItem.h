#ifndef __STATUSITEM_H__
#define __STATUSITEM_H__

#include <View.h>
#include <String.h>
#include <StringView.h>

class StatusItem :public BView{
public:
						StatusItem(BRect rect,
									const char* name,
									const char* initialText,
									void	(*func)(StatusItem* item));
						~StatusItem();
			void		Draw(BRect updateRect);
			void		ResizeToPreferred();
	
			void		SetLabel(const char* label);
protected:
			void		Pulse();	
			void 		FrameResized(float width, float height);
	BString				fLabel;
	BStringView			*fStringView;
			void 		(*pulseFunc)(StatusItem* item);
private:
	float				fFontHeight;
	BRect fCachedBounds;
	rgb_color fBackgroundColor;
	rgb_color fDark_1_color;
	rgb_color fDark_2_color;
};
#endif
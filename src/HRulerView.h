#ifndef __HRULERVIEW_H__
#define __HRULERVIEW_H__

#include <View.h>
#include "HWrapTextView.h"


//!Ruler view for HWrapTextView.
class HRulerView :public BView{
public:
				//!Constructor.
						HRulerView(BRect rect,const char* name,HWrapTextView *view);

				void	FontReseted();
protected:						
				void	Draw(BRect rect);
	 			void	Pulse();
	 			void	MouseDown(BPoint point);
	 			void	MouseUp(BPoint point);
	 			void	MouseMoved(BPoint point, uint32 code, const BMessage* message);
	 			
	 			void	DrawRuler();
private:
	HWrapTextView		*fTextView;
	float				fCaretPosition;
	bool				fDragging;
	float				fFontWidth;
	BFont				fFont;
};
#endif
#ifndef __HSTATUSVIEW_H__
#define __HSTATUSVIEW_H__

#include <View.h>
#include <StringView.h>
#include <ListView.h>
#include <Bitmap.h>
#include <String.h>

class HStatusView :public BView {
public:
					HStatusView(BRect rect,const char* name = "caption",BListView *target=NULL);
	virtual			~HStatusView();
			void	StartBarberPole();
			void	StopBarberPole();
protected:
			void	SetCaption(int32 num,const char* text);
	virtual void	Pulse();	
	virtual void	Draw(BRect updateRect);	
			BRect	BarberPoleInnerRect() const;	
			BRect	BarberPoleOuterRect() const;
private:
	BStringView 	*view;	
	BListView 		*fTarget;
	int32			fOld;
	int32			fLastBarberPoleOffset;
	bool 			fShowingBarberPole;
	BBitmap			*fBarberPoleBits;
};
#endif

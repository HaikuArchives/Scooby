#ifndef __HMAILCAPTION_H__
#define __HMAILCAPTION_H__

#include <View.h>
#include <StringView.h>
#include <ListView.h>
#include <Bitmap.h>

class HMailCaption :public BView {
public:
					HMailCaption(BRect rect,const char* name = "caption",BListView *target=NULL);
	virtual			~HMailCaption();
			void	StartBarberPole();
			void	StopBarberPole();
protected:
			void	SetNumber(int32 num);
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
				
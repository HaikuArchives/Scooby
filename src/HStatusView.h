#ifndef __HSTATUSVIEW_H__
#define __HSTATUSVIEW_H__

#include <View.h>
#include <StringView.h>
#include <ListView.h>
#include <Bitmap.h>
#include <String.h>

//!Barber pole status view.
class HStatusView :public BView {
public:
				//!Constructor.
					HStatusView(BRect rect,
								const char* name = "caption"
								,BListView *target=NULL);
				//!Destructor.
	virtual			~HStatusView();
				//!Start barber pole animation.
			void	StartBarberPole();
				//!Stop barber pole animation.
			void	StopBarberPole();
protected:
				//!Set caption string.
			void	SetCaption(int32 num,const char* text);
	//@{
	//!Override function.
	virtual void	Pulse();	
	virtual void	Draw(BRect updateRect);	
	//@}
	
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

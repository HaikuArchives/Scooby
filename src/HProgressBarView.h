#ifndef __HPROGRESSBARVIEW_H__
#define __HPROGRESSBARVIEW_H__


#include <View.h>
#include <StringView.h>
#include <Bitmap.h>

class HProgressBarView :public BView{
public:
					HProgressBarView(BRect rect,const char* name);
	virtual			~HProgressBarView();
			
			void	StartBarberPole();
			void	StopBarberPole();
			
			void	StartProgress() {fShowingProgress = true;}
			void	StopProgress() {fShowingProgress = false;}
			
			void	Update(float delta);
			void	SetValue(float value);
			void	SetMaxValue(float max) { fMaxValue = max;}
			
			void	SetText(const char* text);
protected:
	virtual	void	Draw(BRect updateRect);
	virtual void	Pulse();

			BRect	BarberPoleInnerRect() const;	
			BRect	BarberPoleOuterRect() const;
private:
	BStringView		*fStringView;
	int32			fLastBarberPoleOffset;
	bool 			fShowingBarberPole;
	bool			fShowingProgress;
	BBitmap			*fBarberPoleBits;
	float			fMaxValue;
	float			fCurrentValue;
	
};
#endif

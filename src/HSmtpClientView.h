#ifndef __SMTPCLIENTVIEW_H__
#define __SMTPCLIENTVIEW_H__

#include <View.h>

class HMailItem;
class SmtpLooper;

enum{
	M_SEND_MAIL = 'SENM'
};

class HSmtpClientView :public BView {
public:
						HSmtpClientView(BRect rect,const char* name);
						~HSmtpClientView();
	
			void	StartBarberPole();
			void	StopBarberPole();
			
			void	StartProgress() {fShowingProgress = true;}
			void	StopProgress() {fShowingProgress = false;}
	
			void	Update(float delta);
			void	SetValue(float value);
			void	SetMaxValue(float max) { fMaxValue = max;}
			
			void	SendMail(HMailItem *item);
			
			bool	IsRunning() const {return fIsRunning;}
			
			void	Cancel();
protected:
	//@{
	//!Override function.
			void	MessageReceived(BMessage *message);
			void	Draw(BRect updateRect);
			void	Pulse();
	//@}
			BRect	BarberPoleInnerRect() const;	
			BRect	BarberPoleOuterRect() const;
private:
	BStringView		*fStringView;
	BBitmap			*fBarberPoleBits;
	int32			fLastBarberPoleOffset;
	bool 			fShowingBarberPole;
	bool			fShowingProgress;	
	float			fMaxValue;
	float			fCurrentValue;
	bool			fIsRunning;
	SmtpLooper		*fSmtpLooper;
};
#endif

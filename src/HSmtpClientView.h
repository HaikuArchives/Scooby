#ifndef __SMTPCLIENTVIEW_H__
#define __SMTPCLIENTVIEW_H__

#include <View.h>

class BStringView;

class HMailItem;
class SmtpClient;

enum{
	M_SEND_MAIL = 'SENM'
};

class HSmtpClientView :public BView {
public:
						HSmtpClientView(BRect rect,const char* name);
	virtual				~HSmtpClientView();
	
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
	virtual void	MessageReceived(BMessage *message);
	virtual void	Draw(BRect updateRect);
	virtual	void	Pulse();
		
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
	SmtpClient		*fSmtpClient;
};
#endif

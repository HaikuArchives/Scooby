#ifndef __SMTPCLIENTVIEW_H__
#define __SMTPCLIENTVIEW_H__

#include "HProgressBarView.h"

class HMailItem;
class SmtpLooper;

enum{
	M_SEND_MAIL = 'SENM',
	M_SMTP_ABORT='mSmA'
};

class HSmtpClientView :public HProgressBarView {
public:
						HSmtpClientView(BRect rect,const char* name);
						~HSmtpClientView();
	
			void	SendMail(HMailItem *item);
			bool	IsRunning() const {return fIsRunning;}
			void	Cancel();
protected:
	//@{
	//!Override function.
			void	MessageReceived(BMessage *message);
			void	MouseDown(BPoint point);
	//@}
private:
	bool			fIsRunning;
	SmtpLooper		*fSmtpLooper;
	
	typedef		HProgressBarView	_inherited;
};
#endif

#ifndef __SMTPLOOPER_H__
#define __SMTPLOOPER_H__

#include <NetworkKit.h>
#include <String.h>
#include <Looper.h>
#include "ExtraMailAttr.h"


#define XMAILER "Scooby for BeOS"
#define OUTBOX "out"

class HMailItem;
class SmtpClient;

enum{
	M_SMTP_CONNECT = 'mSTC',
	M_SMTP_ERROR = 'mSER',
	M_SMTP_END = 'mSED',
	M_SEND_MAIL_SIZE = 'SmSS',
	M_SET_MAX_SIZE = 'mSMS'
};

//! SMTP client thread.
class SmtpLooper :public BLooper{
public:
			//!Constructor.
						SmtpLooper(BHandler *handler,BLooper *looper);
			//!Destructor.
						~SmtpLooper();
			//!Sent mail.	Returns B_ERROR if failed to send.
		status_t		SendMail(const char* from 	//!<From address.
								,const char* to		//!<To addresses.(adress_A,address_B)
								,const char* data	//!<Data to be sent.
								);
			//!Send mail by HMailItem pointer. Returns B_ERROR if failed to send.
		status_t		SendMail(HMailItem *item);
			//!Close SMTP socket.
			void		ForceQuit();
protected:
	//@{
	//!Override functions.
			void		MessageReceived(BMessage *message);
			bool		QuitRequested();
	//@}
			//!Post error.
			void		PostError(const char* log);
private:
	SmtpClient			*fSmtpClient;			//!<SMTP socket.
		BHandler		*fHandler;			//!<Target handler.
		BLooper			*fLooper;			//!<Target looper.
};
#endif
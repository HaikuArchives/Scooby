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
		friend	void	SmtpTotalSize(int32 size,void *cookie); //!<Callback func update StatusBar max size.
		friend	void	SmtpSentSize(int32 size,void *cookie); //!<Callback func update StatusBar value.
		
	SmtpClient			*fSmtpClient;			//!<SMTP socket.
		BHandler		*fHandler;			//!<Target handler.
		BLooper			*fLooper;			//!<Target looper.
};
#endif
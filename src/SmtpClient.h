#ifndef __SMTPCLIENT_H__
#define __SMTPCLIENT_H__

#include <NetworkKit.h>
#include <String.h>
#include <Looper.h>
#include "ExtraMailAttr.h"

#define XMAILER "Scooby for BeOS"
#define OUTBOX "out"

class HMailItem;

enum{
	M_SMTP_CONNECT = 'mSTC',
	M_SMTP_ERROR = 'mSER',
	M_SMTP_END = 'mSED',
	M_SEND_MAIL_SIZE = 'SmSS',
	M_SET_MAX_SIZE = 'mSMS'
};

//! SMTP client socket thread.
class SmtpClient :public BLooper{
public:
			//!Constructor.
						SmtpClient(BHandler *handler,BLooper *looper);
			//!Destructor.
						~SmtpClient();
			//!Connect to SMTP server. Returns B_ERROR if failed to connect.
		status_t		Connect(const char* addr //!<Server address.
								,int16 port=25	//!<Server port.(default value is 25.)
								);
			//!Sent mail.	Returns B_ERROR if failed to send.
		status_t		SendMail(const char* from 	//!<From address.
								,const char* to		//!<To addresses.(adress_A,address_B)
								,const char* data	//!<Data to be sent.
								);
			//!Send mail by HMailItem pointer. Returns B_ERROR if failed to send.
		status_t		SendMail(HMailItem *item);
			//!Quit SMTP sessison. Returns B_ERROR if failed to send QUIT command.
		status_t		SmtpQuit();
			//!Close SMTP socket.
			void		ForceQuit();
protected:
	//@{
	//!Override functions.
			void		MessageReceived(BMessage *message);
			bool		QuitRequested();
	//@}
			//!Receive one line from socket.
			int32		ReceiveLine(BString &buf);
			//!Send command to socket.
			status_t	SendCommand(const char* cmd);
			//!Post error.
			void		PostError(const char* log);
			//!Parse one E-mail address from input string.
			void		ParseAddress(const char* in,BString& out);
private:
	BNetEndpoint		*fEndpoint;			//!<SMTP socket.
		BString			fLog;				//!<The last command's output log.
		BHandler		*fHandler;			//!<Target handler.
		BLooper			*fLooper;			//!<Target looper.

};
#endif
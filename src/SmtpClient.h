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

class SmtpClient :public BLooper{
public:
						SmtpClient(BHandler *handler,BLooper *looper);
	virtual				~SmtpClient();

		status_t		Connect(const char* addr,
								int16 port=25);
		status_t		SendMail(const char* from,
							const char* to,
							const char* data);
		status_t		SendMail(HMailItem *item);
		status_t		SmtpQuit();
		
			void		ForceQuit();
protected:
	virtual	void		MessageReceived(BMessage *message);
	virtual bool		QuitRequested();
			int32		ReceiveLine(BString &buf);
			status_t	SendCommand(const char* cmd);
			void		PostError(const char* log);
			void		ParseAddress(const char* in,BString& out);
private:
	BNetEndpoint		*fEndpoint;
		BString			fLog;
		BHandler		*fHandler;
		BLooper			*fLooper;

};
#endif
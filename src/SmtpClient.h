#ifndef __SMTPCLIENT_H__
#define __SMTPCLIENT_H__

#include <NetEndpoint.h>
#include <String.h>
#include <Looper.h>
#include <Handler.h>

//! SMTP client socket thread.
class SmtpClient :public BNetEndpoint{
public:
			//!Constructor.
						SmtpClient();
			//!Destructor.
						~SmtpClient();
			//!Connect to SMTP server. Returns B_ERROR if failed to connect.
		status_t		Connect(const char* addr //!<Server address.
								,int16 port=25	//!<Server port.(default value is 25.)
								,bool esmtp = false //!< Use ESMTP EHLO command or not.
								);
			//!ESMTP login
		status_t		Login(const char* login,const char* password);
			//!Quit SMTP sessison. Returns B_ERROR if failed to send QUIT command.
		status_t		SmtpQuit();
		//!Sent mail.	Returns B_ERROR if failed to send.
		status_t		SendMail(const char* from 	//!<From address.
								,const char* to		//!<To addresses.(adress_A,address_B)
								,const char* data	//!<Data to be sent.
								);
		//!Returns the last SMTP command response log.
		const char*		Log() const {return fLog.String();}
			//!Parse one E-mail address from input string.
	static	void		ParseAddress(const char* in,BString& out);
			//!Send command and receive all response.
			status_t	SendCommand(const char* cmd);
			//!Receive all command response
			int32		ReceiveResponse(BString &out);
private:
		BString			fLog;				//!<The last command's output log.
		typedef	BNetEndpoint	_inherited;
};
#endif
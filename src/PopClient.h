#ifndef __POP_CLIENT_H__
#define __POP_CLIENT_H__

#include <NetEndpoint.h>
#include <String.h>
#include <List.h>

//! POP3 client socket.
class PopClient :public BNetEndpoint{
public:
		//!Constructor.
					PopClient();
		//!Login to POP3 server.
		status_t	Login(const char* user		//!<Login name.
						,const char* password 	//!<Password.
						,bool apop = false		//!<Use APOP auth or not.
						);
		//!Connect to POP3 server.
		status_t	Connect(const char* address	//!<Server address.
							,int16 port=110		//!<Server port.
							);
		//!Quit POP3 session.
		status_t	PopQuit();
		//!Send stat command.
		status_t	Stat(int32 *mails,int32 *numBytes);
		//!Send list command.
		status_t	List(int32 index,BString &outlist);
		//!Send retr command.
		status_t	Retr(int32 index	 //!<Mail index to be fetched.
						,BString &content	 //!<Output string stored mail content.
						,void (*TotalSize)(int32,void*)  //!<Callback func update StatusBar max size.
						,void (*SentSize)(int32,void*)  //!<Callback func update StatusBar value.
						,void *cookie  //!<Cookie.
						);
		//!Send delete command.
		status_t	Delete(int32 index);
		//!Send rest command.
		status_t	Rset();
		//!Send uidl command.
		status_t	Uidl(int32 index,BString &outlist);
		//!Send last command.
		status_t	Last(int32 *index);
		//!Send top command.
		status_t	Top(int32 index,int32 lines,BString &out);
		//!Receive one line from socket.
			int32	ReceiveLine(BString &buf);
		//!Close POP3 socket.
			void	ForceQuit();
		//!Check SPAM mails. If header contains spam address, returns true.
			bool	IsSpam(const char* header);
		//!Initialize spam mail address list.
			void	InitBlackList();
		//!Send command to server.
		status_t	SendCommand(const char* cmd);	
		//!Make MD5 digest string. (You need to free output buffer.)
			void	MD5Digest (unsigned char *in,char *out);
		//!Returns the last POP command response log.
		const char*		Log() const {return fLog.String();}
private:
		BString		fHost;			//!<Server address.
		uint16		fPort;			//!<Server port.
		BString		fLog;			//!<The last command's output log.
	typedef	BNetEndpoint	_inherited;
};
#endif

#ifndef __POP_CLIENT_H__
#define __POP_CLIENT_H__

#include <NetworkKit.h>
#include <String.h>
#include <Looper.h>

enum{
	H_STAT_MESSAGE = 'HSTM',
	H_LIST_MESSAGE = 'HLSI',
	H_RETR_MESSAGE = 'HRET',
	H_DELETE_MESSAGE = 'HDEL',
	H_LAST_MESSAGE = 'HLAS',
	H_RECEIVING_MESSAGE ='HREI',
	H_CONNECT_MESSAGE = 'HCON',
	H_LOGIN_MESSAGE = 'HLOG',
	H_ERROR_MESSAGE = 'HERR',
	H_RESET_MESSAGE = 'HRES',
	M_QUIT_FINISHED = 'HOKQ',
	H_UIDL_MESSAGE = 'UIDL',
	H_SET_MAX_SIZE = 'seMS'
};

//! POP3 client socket thread.
class PopClient :public BLooper{
public:
		//!Constructor.
					PopClient(BHandler *handler,BLooper *looper);
		//!Destructor.
					~PopClient();
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
		status_t	Retr(int32 index,BString &content);
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
protected:
		//!Send command to server.
		status_t	SendCommand(const char* cmd);	
		//!Post error.
			void	PostError();
	//@{
	//!Override function.
			void	MessageReceived(BMessage *msg);	
			bool	QuitRequested();
	//@}	
		//!Make time_t struct from date string.
		time_t		MakeTime_t(const char* date);
		//!Make MD5 digest string. (You need to free output buffer.)
			char*	MD5Digest (unsigned char *s);
private:
	BNetEndpoint	*fEndpoint;		//!<POP3 socket.
		BString		fHost;			//!<Server address.
		uint16		fPort;			//!<Server port.
		BString		fLog;			//!<The last command's output log.
	BHandler		*fHandler;		//!<Target handler.
	BLooper			*fLooper;		//!<Target looper.
	BList			fBlackList;		//!<SPAM mail address list.
	int32			fBlackListCount;//!<SPAM mail address count.
};
#endif
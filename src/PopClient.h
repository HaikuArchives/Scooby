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

class PopClient :public BLooper{
public:
					PopClient(BHandler *handler,BLooper *looper);
	virtual			~PopClient();
		status_t	Login(const char* user,
						const char* password,
						bool apop = false);
		status_t	Connect(const char* address,
							int16 port);
		status_t	PopQuit();
		
		status_t	Stat(int32 *mails,int32 *numBytes);
		status_t	List(int32 index,BString &outlist);
		status_t	Retr(int32 index,BString &content);
		status_t	Delete(int32 index);
		status_t	Rset();
		status_t	Uidl(int32 index,BString &outlist);
		status_t	Last(int32 *index);
			int32	ReceiveLine(BString &buf);
			void	ForceQuit();
protected:
		status_t	SendCommand(const char* cmd);	
			void	PostError();
	virtual void	MessageReceived(BMessage *msg);	
	virtual bool	QuitRequested();
		time_t		MakeTime_t(const char* date);
			char*	MD5Digest (unsigned char *s);
private:
	BNetEndpoint	*fEndpoint;
		BString		fHost;
		uint16		fPort;
		bool		fBlocking;
		BString		fLog;
	BHandler		*fHandler;
	BLooper			*fLooper;
};
#endif
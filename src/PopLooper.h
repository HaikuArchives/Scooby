#ifndef __POPLOOPER_H__
#define __POPLOOPER_H__

#include <NetworkKit.h>
#include <String.h>
#include <Looper.h>

class PopClient;

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

//! POP3 socket thread.
class PopLooper :public BLooper{
public:
		//!Constructor.
					PopLooper(BHandler *handler,BLooper *looper);
		//!Destructor.
					~PopLooper();
		//!FetchMail.
		status_t	FetchMail(int32 index,BString &content);
		//!Close POP3 socket.
			void	ForceQuit();
		//!Check SPAM mails. If header contains spam address, returns true.
			bool	IsSpam(const char* header);
		//!Initialize spam mail address list.
			void	InitBlackList();
protected:
		//!Post error.
			void	PostError(const char* log);
	//@{
	//!Override function.
			void	MessageReceived(BMessage *msg);	
			bool	QuitRequested();
	//@}	
private:
	friend	void	PopTotalSize(int32 size,void* cookie);  //!<Callback func update StatusBar max size.
	friend	void	PopSentSize(int32 size,void* cookie);  //!<Callback func update StatusBar value.
		
	PopClient		*fPopClient;		//!<POP3 socket.
	BHandler		*fHandler;		//!<Target handler.
	BLooper			*fLooper;		//!<Target looper.
	BString			fLog;			//!<Command response log.
	BList			fBlackList;		//!<SPAM mail address list.
	int32			fBlackListCount;//!<SPAM mail address count.
};
#endif
#ifndef __IMAP4CLIENT_H__
#define __IMAP4CLIENT_H__

#include <NetworkKit.h>
#include <String.h>
#include <Locker.h>

enum{
	IMAP_SESSION_CONTINUED = 0,
	IMAP_SESSION_OK,
	IMAP_SESSION_BAD	
};

//! IMAP4 client socket.
class IMAP4Client :public BNetEndpoint {
public:
		//!Constructor.
					IMAP4Client();
		//!Connect to IMAP4 server.
		status_t	Connect(const char* addr,int port);
		//!Login to IMAP4 server.
		status_t	Login(const char* login,const char* password);
		//!Fetch mail list.
		status_t	List(const char* folder_name,BList *namelist);
		//! Select folder and returns how number of mail contains.
		int32		Select(const char* folder_name); 
		//Fetch mail fields by index.
		status_t	FetchFields(int32 index
								,BString &subject
								,BString &from
								,BString &to
								,BString &cc
								,BString &reply
								,BString &date
								,BString &priority
								,bool	&read
								,bool    &attachment
								);
		//!Fetch mail body by index.
		status_t	FetchBody(int32 index,BString &outBody);
		//!Mark mail as read.
		status_t	MarkAsRead(int32 index);
		//!Mark mail as delete.
		status_t	MarkAsDelete(int32 index);
		//!Store new mail flags.
		status_t	Store(int32 index,const char* flags,bool add = true);
		//!Send noop command.
		status_t	Noop();
		//!Send expunge commnad.
		status_t	Expunge();
		//!Reconnect to IMAP4 server.
		status_t	Reconnect();
		//!Returns true if connection is alive.
		bool		IsAlive();
		//!Logout.
		void		Logout();
		//!Copy
		status_t	Copy(const char* indexList //!<Mail index list like 1:4
						,const char* dest_path //!<Destination folder.
						);
		//!Create new mailbox
		status_t	Create(const char* name,const char* dest_path=NULL);
		//!Delete mailbox.
		status_t	Delete(const char* path);
		//!Close mailbox
		status_t	CloseMailBox();
protected:
		status_t	SendCommand(const char* str);
		int32		ReceiveLine(BString &out);
		int32		CheckSessionEnd(const char* line,int32 session);
		
		bool		Lock(){return fSocketLocker.Lock();}
		void		Unlock(){return fSocketLocker.Unlock();}
		
		status_t	ReceiveResponse(BString &out);
private:
		typedef	BNetEndpoint	_inherited;
		int32			fCommandCount;
		BString			fAddress;
		int				fPort;
		BString			fLogin;
		BString			fPassword;
		BString			fFolderName;
		time_t			fIdleTime;
		BLocker			fSocketLocker;
};
#endif
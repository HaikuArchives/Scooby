#ifndef __IMAP4CLIENT_H__
#define __IMAP4CLIENT_H__

#include <NetworkKit.h>
#include <String.h>

enum{
	IMAP_SESSION_CONTINUED = 0,
	IMAP_SESSION_OK,
	IMAP_SESSION_BAD	
};

class IMAP4Client :public BNetEndpoint {
public:
					IMAP4Client();
	virtual			~IMAP4Client();
	
		status_t	Connect(const char* addr,int port);
		status_t	Login(const char* login,const char* password);
		status_t	List(const char* folder_name,BList *namelist);
		int32		Select(const char* folder_name); // return mail count
		status_t	FetchFields(int32 index,
								BString &subject,
								BString &from,
								BString &to,
								BString &cc,
								BString &reply,
								BString &date,
								BString &priority,
								bool	&read,
								bool    &attachment);
		status_t	FetchBody(int32 index,BString &outBody);
		status_t	MarkAsRead(int32 index);
		status_t	MarkAsDelete(int32 index);
		status_t	Store(int32 index,const char* flags,bool add = true);
		status_t	Noop();
		status_t	Reconnect();
		bool		IsAlive();
		void		Logout();
protected:
		status_t	SendCommand(const char* str);
		int32		ReceiveLine(BString &out);
		int32		CheckSessionEnd(const char* line,int32 session);
private:
		typedef	BNetEndpoint	_inherited;
		int32			fCommandCount;
		BString			fAddress;
		int				fPort;
		BString			fLogin;
		BString			fPassword;
		BString			fFolderName;
		time_t			fIdleTime;
};
#endif
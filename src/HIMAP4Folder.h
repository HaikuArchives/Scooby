#ifndef __HIMAP4FOLDER_H__
#define __HIMAP4FOLDER_H__

#include "HFolderItem.h"
#include "IMAP4Client.h"

#include <String.h>

class HIMAP4Folder :public HFolderItem{
public:
						HIMAP4Folder(const char* name,
									const char* folder_name,
									const char* server,
									int			port,
									const char* login,
									const char* password,
									BListView *owner);
	virtual				~HIMAP4Folder();
	virtual	void		StartRefreshCache();
	virtual void		StartGathering();
	
			void		SetServer(const char* addr) {fServer = addr;}
			void		SetLogin(const char* login) {fLogin = login;}
			void		SetPort(int port) {fPort = port;}
			void		SetPassword(const char* pass){fPassword = pass;}
			void		SetFolderName(const char* folder) {fFolderName = name;}
protected:
			void		IMAPGetList();
		status_t		IMAPConnect();
			time_t		MakeTime_t(const char* date);
	static	int32		GetListThread(void* data);
	
			void		StoreSettings();
private:
	IMAP4Client			*fClient;
		BString			fServer;
		int				fPort;
		BString			fLogin;
		BString			fPassword;
		BString			fFolderName;
};
#endif
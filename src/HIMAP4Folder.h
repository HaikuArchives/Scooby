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
	
	const char*			Server()const {return fServer.String();}
	const char*			Login() const {return fLogin.String();}
			int			Port()const {return fPort;}
	const char*			Password() const{return fPassword.String();}
	const char*			RemoteFolderName() const{return fRemoteFolderName.String();}
	
			void		SetServer(const char* addr) {fServer = addr;}
			void		SetLogin(const char* login) {fLogin = login;}
			void		SetPort(int port) {fPort = port;}
			void		SetPassword(const char* pass){fPassword = pass;}
			void		SetFolderName(const char* folder) {fRemoteFolderName = folder;}
protected:
			void		IMAPGetList();
		status_t		IMAPConnect();
			time_t		MakeTime_t(const char* date);
	static	int32		GetListThread(void* data);
			void		GatherChildFolders();
			void		StoreSettings();
private:
	IMAP4Client			*fClient;
		BString			fServer;
		int				fPort;
		BString			fLogin;
		BString			fPassword;
		BString			fRemoteFolderName;
		BString			fDisplayedFolderName;
		bool			fFolderGathered;
};
#endif
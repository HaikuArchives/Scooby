#ifndef __HIMAP4FOLDER_H__
#define __HIMAP4FOLDER_H__

#include "HFolderItem.h"
#include "IMAP4Client.h"

#include <String.h>

//!IMAP4 folder item.
class HIMAP4Folder :public HFolderItem{
public:
						HIMAP4Folder(const char* name,
									const char* folder_name,
									const char* server,
									int			port,
									const char* login,
									const char* password,
									BListView *owner);
						~HIMAP4Folder();
		void			StartRefreshCache();
		void			StartGathering();
	
	const char*			Server()const {return fServer.String();}
	const char*			Login() const {return fLogin.String();}
			int			Port()const {return fPort;}
	const char*			Password() const{return fPassword.String();}
	const char*			RemoteFolderPath() const{return fRemoteFolderPath.String();}
			void		SetRemoteFolderName(const char* folder) {fRemoteFolderPath = folder;}
	const char*			AccountName() const {return fAccountName.String();}
			void		SetAccountName(const char* name){fAccountName = name;}
			
			void		SetServer(const char* addr) {fServer = addr;}
			void		SetLogin(const char* login) {fLogin = login;}
			void		SetPort(int port) {fPort = port;}
			void		SetPassword(const char* pass){fPassword = pass;}
			
			void		SetFolderGathered(bool gathered) {fFolderGathered = gathered;}
			void		SetChildFolder(bool child) {fChildItem = child;}
			bool		IsChildFolder() const {return fChildItem;}
			//!Delete IMAP4 folder.
			bool		DeleteMe();
			//!Create child folder.
			void		CreateChildFolder(const char* name);
			//!Move mails to dest_folder.
		status_t		Move(const char* indexList,const char* dest_path);
protected:
			void		IMAPGetList();
		status_t		IMAPConnect();
	static	int32		GetListThread(void* data);
			void		GatherChildFolders();
			void		StoreSettings();
			
			int32		FindParent(const char* name,const char* path,BList *list);
		HIMAP4Folder*	MakeNewFolder(const char* utf8Name,const char *utf7Path);
private:
	IMAP4Client			*fClient;
		BString			fServer;
		int				fPort;
		BString			fLogin;
		BString			fPassword;
		BString			fRemoteFolderPath; //!<Remove folder path.
		BString			fDisplayedFolderName;//!<Folder name converted to UTF8
		BString			fAccountName;		//!<IMAP4 account name
		bool			fFolderGathered;
		bool			fChildItem;
};
#endif

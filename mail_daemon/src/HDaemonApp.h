#ifndef __HDAEMON_APP__
#define __HDAEMON_APP__

#include <Application.h>
#include <MessageRunner.h>
#include <Path.h>

#include "PopClient.h"
#include "HPrefs.h"
#include "HDeskbarView.h"

#define APP_SIG "application/x-vnd.takamatsu-scooby-daemon"


enum{
	M_TIMER = 'mTim',
	M_CHECK_NOW = 'mChN',
	M_NEW_MESSAGE = 'mNeM',
	M_RELOAD_SETTING = 'mRES',
	M_RESET_ICON = 'mREI',
	M_LAUNCH_SCOOBY = 'mLSc'
};


class HDaemonApp :public BApplication {
public:
							HDaemonApp();
		virtual				~HDaemonApp();
				void		RunTimer();
				void		StopTimer();
				void		CheckMails();
				int16 		RetrieveType()const {return fRetrievingType;}
				void		PlayNotifySound();
				
			HPrefs*			Prefs() {return fPrefs;}
protected:
		virtual void		MessageReceived(BMessage *message);
		virtual bool		QuitRequested();
		
				void		InstallDeskbarIcon();
				void		RemoveDeskbarIcon();
				void		ChangeDeskbarIcon(IconType type);
				
				status_t	CheckFromServer(entry_ref &ref);
				time_t		MakeTime_t(const char* date);
			
				
				bool		Filter(const char* key,
									int32 operation,
									const char* value);
				
				void		SaveMail(const char* content,
									entry_ref *folder_ref,
									entry_ref *file_ref,
									bool *is_delete);
				void		FilterMail(const char* subject,
									const char* from,
									const char* to,
									const char* cc,
									const char* reply,
									BString &outpath);
				
				int32		GetHeaderParam(BString &out,const char* content,int32 offset);
				void		SetNextRecvPos(const char* uidl);
				
			status_t		GetNextAccount(entry_ref &ref);
			
				void		EmptyNewMailList();
				void		AddNewMail(BEntry *entry);
private:
		BMessageRunner		*fRunner;
		HPrefs				*fPrefs;
		BPath				fSettingPath;
		bool				fChecking;
		bool				fGotMails;
		PopClient			*fPopClient;
		BDirectory			*fAccountDirectory;
		bool				fCanUseUIDL;
		BMessage			fDeleteMails;
		bool				fHaveNewMails;
		BList				fNewMailList;
		// Account info
		BString 	fHost;
		int16 		fPort;
		BString 	fLogin;
		BString		fPassword;
		int16		fProtocolType;
		int16		fRetrievingType;
		int32		fDeleteDays;
		BString		fUidl;
		BString		fAccountName;
		//
};
#endif
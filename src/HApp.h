#ifndef __HAPP_H__
#define __HAPP_H__

#include "LocaleApp.h"

#define APP_NAME		"Scooby"
#define QUERY_FOLDER	"Queries"
#define APP_SIG			"application/x-vnd.takamatsu-scooby"

class HWindow;
class HPrefs;

enum{
	M_CREATE_FOLDER = 'mCrF'
};

class HApp :public LocaleApp{
public:
							HApp();
		virtual				~HApp();
		HPrefs*				Prefs() {return fPref;}
		BWindow*			MainWindow() {return (BWindow*)fWindow;}
		
protected:
		virtual void		MessageReceived(BMessage *msg);
		virtual bool		QuitRequested();
		virtual void		AboutRequested();
		virtual void		ReadyToRun();
		virtual void		RefsReceived(BMessage *message);
		virtual void		ArgvReceived(int32 argc,char **argv);
		virtual void		Pulse();
				bool		MakeMainWindow(const char* mail_addr=NULL,
											bool hidden = false);
				void		Print(BView *textview,BView *detail,const char* job_name);
				void		PageSetup();
				void		AddSoundEvent(const char* name);
				void		RemoveTmpImapMails();
				
				bool		IsNetPositiveRunning();
private:		
		BMessage*			fPrintSettings;
		HWindow 			*fWindow;
		HPrefs				*fPref;
		bool				fWatchNetPositive;
		typedef	LocaleApp	_inherited;
};
#endif
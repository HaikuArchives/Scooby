#ifndef __HAPP_H__
#define __HAPP_H__

#include "LocaleApp.h"

#define APP_NAME		"Scooby"
#define QUERY_FOLDER	"Queries"
#define APP_SIG			"application/x-vnd.takamatsu-scooby"

class HWindow;
class HPrefs;

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
				bool		MakeMainWindow(const char* mail_addr=NULL,
											bool hidden = false);
				void		Print(BTextView *textview,BView *detail,const char* job_name);
				void		PageSetup();
				void		AddSoundEvent(const char* name);
				void		RemoveTmpImapMails();
private:		
		BMessage*			fPrintSettings;
		HWindow 			*fWindow;
		HPrefs				*fPref;
		typedef	LocaleApp	_inherited;
};
#endif
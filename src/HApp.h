#ifndef __HAPP_H__
#define __HAPP_H__

#include "LocaleApp.h"
#include <MessageFilter.h>

#define APP_NAME		"Scooby"
#define QUERY_FOLDER	"Queries"
#define APP_SIG			"application/x-vnd.takamatsu-scooby"

class HWindow;
class HFindWindow;
class HPrefs;

enum{
	M_SHOW_FIND_WINDOW = 'mSfW',
	M_FIND_NEXT_WINDOW = 'mFnN'
};

class HApp :public LocaleApp{
public:
							HApp();
		virtual				~HApp();
		HPrefs*				Prefs() {return fPref;}
		BWindow*			MainWindow() {return (BWindow*)fWindow;}
		BBitmap*			GetIcon(const char* icon_name);
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
				
				void		ShowFindWindow();
				void		InitIcons();

#ifdef CHECK_NETPOSITIVE				
			BMessageFilter	*fMessageFilter;
			bool			fFilterAdded;
	static	filter_result	MessageFilter(BMessage *message,BHandler **target,BMessageFilter *messageFilter);
#endif

private:		
		BMessage*			fPrintSettings;
		HWindow 			*fWindow;
		HPrefs				*fPref;
		bool				fWatchNetPositive;
		HFindWindow*		fFindWindow;
		// mail icons
		BBitmap*			fReadMailIcon;
		BBitmap*			fNewMailIcon;
		BBitmap*			fEnclosureIcon;
		BBitmap*			fForwardedMailIcon;
		BBitmap*			fSentMailIcon;
		BBitmap*			fRepliedMailIcon;
		BBitmap*			fPriority1;
		BBitmap*			fPriority2;
		BBitmap*			fPriority4;
		BBitmap*			fPriority5;
		// folder icons
		BBitmap*			fOpenFolderIcon;
		BBitmap*			fCloseFolderIcon;
		BBitmap*			fOpenQueryIcon;
		BBitmap*			fCloseQueryIcon;
		BBitmap*			fOpenIMAPIcon;
		BBitmap*			fCloseIMAPIcon;
		//
		typedef	LocaleApp	_inherited;
};
#endif
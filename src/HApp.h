#ifndef __HAPP_H__
#define __HAPP_H__

#ifndef USE_SPLOCALE
	#include <Application.h>
	#define _(String) (String)
#else
	#include "SpLocaleApp.h"
	#define _( String ) SpTranslate( String )
#endif

#include <MessageFilter.h>

#define APP_NAME		"Scooby"
#define QUERY_FOLDER	"Queries"
#define APP_SIG			"application/x-vnd.takamatsu-scooby"

class HWindow;
class HFindWindow;
class HPrefs;

enum{
	M_SHOW_FIND_WINDOW = 'mSfW',
	M_FIND_NEXT_WINDOW = 'mFnN',
	M_MOVE_FILE = 'mMvF'
};

#ifndef USE_SPLOCALE
class HApp :public BApplication{
#else
class HApp :public SpLocaleApp{
#endif
public:
							HApp();
							~HApp();
		HPrefs*				Prefs() {return fPref;}
		BWindow*			MainWindow() {return (BWindow*)fWindow;}
		BBitmap*			GetIcon(const char* icon_name);
protected:
		//@{
		//!Override function.
				void		MessageReceived(BMessage *msg);
				bool		QuitRequested();
				void		AboutRequested();
				void		ReadyToRun();
				void		RefsReceived(BMessage *message);
				void		ArgvReceived(int32 argc,char **argv);
				void		Pulse();
		//@}
				bool		MakeMainWindow(bool hidden = false);
				void		Print(BView *textview,BView *detail,const char* job_name);
				void		PageSetup();
				void		AddSoundEvent(const char* name);
				void		RemoveTmpImapMails();
				
				bool		IsNetPositiveRunning();
				
				void		ShowFindWindow();
				void		InitIcons();
			
				void		MoveFile(entry_ref file_ref,const char* dest_dir);

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
		BBitmap*			fPendingIcon;
		BBitmap*			fErrorIcon;
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
		BBitmap*			fPersonIcon;
		//
#ifndef USE_SPLOCALE
		typedef	BApplication	_inherited;
#else
		typedef SpLocaleApp _inherited;	
#endif
};
#endif
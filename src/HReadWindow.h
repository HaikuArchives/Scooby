#ifndef __HREADWINDOW_H__
#define __HREADWINDOW_H__

#include <Window.h>
#include <Entry.h>
#include <Message.h>

class HMailView;
class HDetailView;

enum{
	M_NEXT_MESSAGE = 'mNex',
	M_PREV_MESSAGE = 'mPre'
};


//!Read window.
class HReadWindow :public BWindow{
public:
			//!Constructor.
						HReadWindow(BRect rect	//!<Read window's frame.
									,entry_ref ref	//!<entry_ref to be read.
									,BMessenger *messenger = NULL //!<Messenger for scripting.
									);
	
protected:
			//!Destructor.
						~HReadWindow();
	//@{
	//! Override function.
		 	void		MessageReceived(BMessage *message);
		 	bool		QuitRequested();
		 	void		MenusBeginning();
		 	void		DispatchMessage(BMessage *message,BHandler *handler);
	//@}
			//!Initialize all GUI.
			void		InitGUI();
			//!Initialize menubar.
			void		InitMenu();
			//!Load mail message.
			void		LoadMessage(entry_ref ref);
			//!Send selection changed message to messenger.
			void		Select(entry_ref ref);
			//!Send fetch prev or next file message to messenger.
			void		SiblingItem(int32 what/*!<Prev:'sprv' Next:'snxt' */);
			//!Mark a mail as read.
			void		SetRead();
			//!Print out message.
			void		PrintMessage(BMessage *message);
private:
	BView*				fMailView;			//!< Mail body view.
	HDetailView*		fDetailView;		//!< Defail info view displayed top of window.
	BMessenger*			fMessenger;			//!< Scriping target messenger.
		entry_ref		fRef;				//!< Opened file's entry_ref.
		int32			fCurrentIndex;		//!< Opened mail's list index.(use for scripting.)
};
#endif
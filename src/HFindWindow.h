#ifndef __HFINDWINDOW_H__
#define __HFINDWINDOW_H__

#include <Window.h>
#include <String.h>

//!Find window.
class HFindWindow :public BWindow {
public:
	//!Constructor.
						HFindWindow(BRect rect,const char* name);
	//! Set handler to send search message.
			void		SetTarget(BHandler *handler) {fTarget = handler;}
	//! Set quit flag.
	/*!
		If quit flag is false, Find window is not close but hide when close button wa pressed.
		When application is quit, set this flag true.
	*/
			void		SetQuit(bool quit) {fCanQuit = quit;}
	//! Save search keyword history.
			void		SaveHistory();
	//! Load search keyword history.
			void		LoadHistory();
	//! Add keyword to the hisotry.
			void		AddToHistory(const char* text);
protected:
	//!Destructor.
						~HFindWindow();
	//@{
	//!Override function.
			void		MessageReceived(BMessage *message);
			bool		QuitRequested();
	//@}
private:
	BHandler			*fTarget; // Search target handler.
	bool				fCanQuit; // Quit flag.
};
#endif
#ifndef __HFINDWINDOW_H__
#define __HFINDWINDOW_H__

#include <Window.h>
#include <String.h>

class HFindWindow :public BWindow {
public:
						HFindWindow(BRect rect,const char* name);
	virtual				~HFindWindow();
	
			void		SetTarget(BHandler *handler) {fTarget = handler;}
			void		SetQuit(bool quit) {fCanQuit = quit;}
protected:
	virtual	void		MessageReceived(BMessage *message);
	virtual bool		QuitRequested();
private:
	BHandler			*fTarget;
	BString				fText;
	bool				fCanQuit;
};
#endif
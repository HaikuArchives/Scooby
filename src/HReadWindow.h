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

class HReadWindow :public BWindow{
public:
						HReadWindow(BRect rect,
									entry_ref ref,
									BMessenger *messenger = NULL);
	virtual				~HReadWindow();
	
protected:
	virtual void		MessageReceived(BMessage *message);
	virtual	bool		QuitRequested();
	virtual void		MenusBeginning();
	virtual void		DispatchMessage(BMessage *message,BHandler *handler);
			void		InitGUI();
			void		InitMenu();
			
			void		LoadMessage(entry_ref ref);
			
			void		Select(entry_ref ref);
			void		SiblingItem(int32 what);
			
			void		SetRead();
		
			void		PrintMessage(BMessage *message);
private:
	BView*				fMailView;
	HDetailView*		fDetailView;
	BMessenger*			fMessenger;
		entry_ref		fRef;
		int32			fCurrentIndex;
};
#endif

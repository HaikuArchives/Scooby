#ifndef __HIMAP4WINDOW_H__
#define __HIMAP4WINDOW_H__

#include <Window.h>

class HIMAP4Folder;


class HIMAP4Window :public BWindow {
public:
						HIMAP4Window(BRect rect,
									BHandler *handler,
									HIMAP4Folder* item = NULL);
protected:
						~HIMAP4Window();
			void		MessageReceived(BMessage *message);
			void		InitGUI();
	const char*			GetText(const char* name);
			void		SetText(const char* name,const char* text);
private:
			bool		fAddMode;
			BHandler	*fHandler;
		HIMAP4Folder*	fItem;
};
#endif
#ifndef __HIMAP4WINDOW_H__
#define __HIMAP4WINDOW_H__

#include <Window.h>

class HIMAP4Window :public BWindow {
public:
						HIMAP4Window(BRect rect,
									BHandler *handler,
									const char* name = NULL,
									const char* folder = NULL,
									const char* server = NULL,
									int			port = 0,
									const char* login = NULL,
									const char* password = NULL);
	virtual				~HIMAP4Window();
protected:
	virtual	void		MessageReceived(BMessage *message);
			void		InitGUI();
	const char*			GetText(const char* name);
			void		SetText(const char* name,const char* text);
private:
			bool		fAddMode;
			BHandler	*fHandler;
};
#endif
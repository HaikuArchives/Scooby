#ifndef __HCREATEFOLDERDIALOG_H__
#define __HCREATEFOLDERDIALOG_H__

#include <Window.h>
#include <TextControl.h>

class HCreateFolderDialog :public BWindow{
public:
					HCreateFolderDialog(BRect rect,
										const char* title,
										const char* path);
	virtual 		~HCreateFolderDialog();
			void	InitGUI();
protected:
	virtual	void	MessageReceived(BMessage *message);
private:
	BTextControl*	fNameControl;
	char*			fParentPath;
};
#endif

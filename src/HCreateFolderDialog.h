#ifndef __HCREATEFOLDERDIALOG_H__
#define __HCREATEFOLDERDIALOG_H__

#include <Window.h>
#include <TextControl.h>

//!Create folder dialog.
class HCreateFolderDialog :public BWindow{
public:
		//! Constructor.
					HCreateFolderDialog(BRect rect //!< Window frame rectanble.
										,const char* title //!<Window title.
										,const char* path //!< Path that folder to be created in.
										);
		//! Destructor.
					~HCreateFolderDialog();
		//! Initialize all GUI.
			void	InitGUI();
protected:
		//! Handling messages.
			void	MessageReceived(BMessage *message);
private:
	BTextControl*	fNameControl;
	char*			fParentPath;
};
#endif
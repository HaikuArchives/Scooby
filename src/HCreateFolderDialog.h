#ifndef __HCREATEFOLDERDIALOG_H__
#define __HCREATEFOLDERDIALOG_H__

#include <Window.h>
#include <TextControl.h>

class HFolderItem;

//!Create folder dialog.
class HCreateFolderDialog :public BWindow{
public:
		//! Constructor.
					HCreateFolderDialog(BRect rect //!< Window frame rectanble.
										,const char* title //!<Window title.
										,HFolderItem *parent=NULL //!< Path that folder to be created in.
										);
		//! Initialize all GUI.
			void	InitGUI();
protected:
		//! Handling messages.
			void	MessageReceived(BMessage *message);
private:
	BTextControl*	fNameControl;
	HFolderItem		*fParentItem;
};
#endif
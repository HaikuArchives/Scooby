#ifndef __HACCOUNTVIEW_H__
#define __HACCOUNTVIEW_H__

#include <View.h>
#include <ListView.h>

enum{
	M_ADD_ACCOUNT = 'MAdc',
	M_DEL_ACCOUNT = 'MdeA',
	M_CHANGE_ACCOUNT = 'MCHA',
	M_ACCOUNT_SAVE_CHANGED = 'MSAC'
};

//!Account view for preference panel.
class HAccountView :public BView{
public:
		//!	Constructor
						HAccountView(BRect rect);
		//! Destructor
	virtual				~HAccountView();
		//! Initialize all GUI.
			void		InitGUI();
protected:
		//@{
		//! Override function.
	virtual void		MessageReceived(BMessage *message);	
	virtual	void		AttachedToWindow();
		//@}
		//!	Save account file.
			void		SaveAccount(int32 index/*!< List index.*/);
		//! Open account file.
			void		OpenAccount(int32 index/*!< List index.*/);
		//! Create new account file.
			void		New();
		//! Enable or disable all controls.
			void		SetEnableControls(bool enable);
private:
	BListView			*fListView;
};
#endif
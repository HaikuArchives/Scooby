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

class HAccountView :public BView{
public:
						HAccountView(BRect rect);
	virtual				~HAccountView();
			void		InitGUI();
protected:
	virtual void		MessageReceived(BMessage *message);	
			void		SaveAccount(int32 index);
			void		OpenAccount(int32 index);
			void		New();
			void		SetEnableControls(bool enable);
			
private:
	BListView			*fListView;
};
#endif
#ifndef __SPAMFILTERVIEW_H__
#define __SPAMFILTERVIEW_H__

#include <View.h>
#include <ListView.h>
#include <TextControl.h>
#include <Button.h>

enum{
	M_SPAM_ADDRESS_MODIFIED = 'mADM',
	M_SPAM_OK = 'sOKm',
	M_SPAM_DEL = 'sDEL',
	M_SPAM_SELECTION_CHANGED = 'SELc'
};

class HSpamFilterView :public BView {
public:
						HSpamFilterView(BRect rect);
						~HSpamFilterView();
	
			void		Save();
			void		Load();
protected:
	//@{
	//!Override function.
			void		MessageReceived(BMessage *msg);
			void		AttachedToWindow();
	//@}
private:
	BListView*			fAddressList;
	BTextControl*		fAddressCtrl;
	BButton*			fAddBtn;
	BButton*			fDeleteBtn;
};
#endif
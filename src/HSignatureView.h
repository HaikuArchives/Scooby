#ifndef __HSIGNATUREVIEW_H__
#define __HSIGNATUREVIEW_H__

#include <View.h>
#include "CTextView.h"
#include <ListView.h>
#include <TextControl.h>

enum{
	M_ADD_SIGNATURE = 'MSIA',
	M_DEL_SIGNATURE = 'MSID',
	M_CHANGE_SIGNATURE = 'MSCH',
	M_SIGNATURE_SAVE_CHANGED = 'MSCD'
};

class HSignatureView :public BView {
public:
					HSignatureView(BRect rect);
	virtual			~HSignatureView();	
			void	InitGUI();
protected:
	virtual void	MessageReceived(BMessage *message);
			void	SetEnableControls(bool enalbe);
			void	SaveItem(int32 sel);
			void	OpenItem(int32 sel);
	virtual	void	AttachedToWindow();
private:
	CTextView*		fTextView;
	BListView*		fListView;
	BTextControl*	fName;
};
#endif

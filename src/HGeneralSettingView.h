#ifndef __HGENERALSETTINGVIEW_H__
#define __HGENERALSETTINGVIEW_H__

#include <View.h>

enum{
	M_FONT_CHANGED = 'MFNC'
};

class HGeneralSettingView :public BView {
public:
					HGeneralSettingView(BRect rect);
	virtual			~HGeneralSettingView();	
			void	Save();
			void	InitGUI();
protected:
	virtual void	MessageReceived(BMessage *message);
	virtual	void	AttachedToWindow();
private:
	
};
#endif

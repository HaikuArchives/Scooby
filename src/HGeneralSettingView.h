#ifndef __HGENERALSETTINGVIEW_H__
#define __HGENERALSETTINGVIEW_H__

#include <View.h>

enum{
	M_FONT_CHANGED = 'MFNC'
};

class HGeneralSettingView :public BView {
public:
					HGeneralSettingView(BRect rect);
					~HGeneralSettingView();	
			void	Save();
			void	InitGUI();
protected:
			void	MessageReceived(BMessage *message);
			void	AttachedToWindow();
private:
	
};
#endif

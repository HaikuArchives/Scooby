#ifndef __HPREFWINDOW_H__
#define __HPREFWINDOW_H__

#include <Window.h>

class HFilterView;
class HAccountView;
class HSignatureView;
class HGeneralSettingView;

enum{
	M_PREFS_CHANGED = 'MPCD'
};

class HPrefWindow :public BWindow {
public:
					HPrefWindow(BRect rect);
protected:
	virtual			~HPrefWindow();
	virtual	void	MessageReceived(BMessage *message);
	virtual bool	QuitRequested();
private:
	HFilterView*	fFilterView;
	HAccountView*	fAccountView;
	HSignatureView*	fSignatureView;
	HGeneralSettingView*		fGeneralView;
};
#endif
#ifndef __HPREFWINDOW_H__
#define __HPREFWINDOW_H__

#include "MWindow.h"

class HFilterView;
class HAccountView;
class HSignatureView;
class HGeneralSettingView;
class HSpamFilterView;

enum{
	M_PREFS_CHANGED = 'MPCD',
	M_ADD_FOLDERS = 'mADF'
};

class HPrefWindow :public MWindow {
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
	HSpamFilterView*	fSpamFilterView;
	HGeneralSettingView*		fGeneralView;
	
	typedef 	MWindow			_inherited;
};
#endif
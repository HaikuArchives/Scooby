#ifndef __HPREFWINDOW_H__
#define __HPREFWINDOW_H__

#include <Window.h>

class HFilterView;
class HAccountView;
class HSignatureView;
class HGeneralSettingView;
class HSpamFilterView;

enum{
	M_PREFS_CHANGED = 'MPCD',
	M_ADD_FOLDERS = 'mADF'
};

//!Preference window.
class HPrefWindow :public BWindow {
public:
			//!Constructor.
					HPrefWindow(BRect rect);
protected:
			//!Destructor.
	virtual			~HPrefWindow();
	//@{
	//!Override function.
	virtual	void	MessageReceived(BMessage *message);
	virtual bool	QuitRequested();
	//@}
private:
	HFilterView*	fFilterView;
	HAccountView*	fAccountView;
	HSignatureView*	fSignatureView;
	HSpamFilterView*	fSpamFilterView;
	HGeneralSettingView*		fGeneralView;
};
#endif
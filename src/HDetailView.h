#ifndef __HDetailView_H__
#define __HDetailView_H__

#include <View.h>
#include <TextControl.h>
#include <List.h>

class MultiLineTextControl;

#define DETAIL_VIEW_HEIGHT 65;

enum{
	M_MODIFIED 	= 'SUMD',
	M_ADDR_MSG	='TOMS'
};

class HDetailView: public BView {
public:
					HDetailView(BRect rect,bool readOnly = false);
	virtual			~HDetailView();
			void	SetReadOnly(bool enable);
			void	SetInfo(const char* subject,
							const char* from,
							const char* when);
	const char*		Subject();
	BTextControl*	FocusedView() const;
protected:
			void	InitGUI();
private:
	MultiLineTextControl	*fSubject;
	MultiLineTextControl	*fFrom;
	MultiLineTextControl	*fWhen;
		bool		fReadOnly;
};
#endif
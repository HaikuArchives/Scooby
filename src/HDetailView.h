#ifndef __HDetailView_H__
#define __HDetailView_H__

#include <View.h>
#include <TextControl.h>
#include <List.h>

class MultiLineTextControl;

#define DETAIL_VIEW_HEIGHT 65
#define DETAIL_VIEW_HEIGHT_EXPANDED 110

enum{
	M_MODIFIED 	= 'SUMD',
	M_ADDR_MSG	='TOMS',
	M_EXPAND_ATTRIBUTES = 'eXPA'
};

//!Display incoming mails' info such as Subject, To and so on.
class HDetailView: public BView {
public:
	//! Constructor
					HDetailView(BRect rect,bool readOnly = false);
	//! Set all TextControl as readonly.
			void	SetReadOnly(bool enable);
	//! Set all values to be displayed.
			void	SetInfo(const char* subject,
							const char* from,
							const char* when,
							const char* cc,
							const char* to);
	//! Returns current subject.
	const char*		Subject();
	//! Returns current focused view.
	BTextControl*	FocusedView() const;
protected:
	//! Initialize all GUI.
			void	InitGUI();
	//! Override functions.
			void	MessageReceived(BMessage *message);
private:
	MultiLineTextControl	*fSubject;
	MultiLineTextControl	*fFrom;
	MultiLineTextControl	*fWhen;
	MultiLineTextControl	*fCc;
	MultiLineTextControl	*fTo;
	
		bool		fReadOnly;
};
#endif
#ifndef __HDESKBARVIEW_H__
#define __HDESKBARVIEW_H__

#include <View.h>
#include <Message.h>

enum{
	M_SHOW_MSG = 'MSHO',
	M_CHECK_SCOOBY_STATE = 'McSS'
};

//!Deskbar replicant view.
class _EXPORT HDeskbarView :public BView {
public:
			//!Constructor.
						HDeskbarView(BRect frame);
			//!Constructor for archiving.
						HDeskbarView(BMessage *data);
			//!Destructor.
		virtual			~HDeskbarView();
	
protected:	
		//@{
		//! Override function.
		virtual void	Draw(BRect updateRect);
		virtual void	Pulse();
		virtual void 	MouseDown(BPoint);
		virtual void	MessageReceived(BMessage *message);
		//@}
		//@{
		//! Archiving override.
		static 	HDeskbarView *Instantiate(BMessage *data);
		virtual	status_t Archive(BMessage *data, bool deep = true) const;
		//@}
		//!Change the icon to be displayed in desktray.
		void	ChangeIcon(int32 icon);
private:
		BBitmap 		*fIcon;//!< Current icon bitmap.
		int32			fCurrentIconState; //!<Current icon state
		char*			fLabels[3];//!<Menu item labels.
};
#endif
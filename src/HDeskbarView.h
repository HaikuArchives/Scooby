#ifndef __HDESKBARVIEW_H__
#define __HDESKBARVIEW_H__

#include <View.h>

enum{
	M_SHOW_MSG = 'MSHO'
};

class _EXPORT HDeskbarView :public BView {
public:
						HDeskbarView(BRect frame);
						HDeskbarView(BMessage *data);
		virtual			~HDeskbarView();
	
protected:	
		virtual void	Draw(BRect updateRect);
		// archiving overrides
		static 	HDeskbarView *Instantiate(BMessage *data);
		virtual	status_t Archive(BMessage *data, bool deep = true) const;
		virtual 	void MouseDown(BPoint);
		virtual void	MessageReceived(BMessage *message);
				void	WhenDropped(BMessage* message);

private:
		BBitmap 		*fIcon;
};
#endif
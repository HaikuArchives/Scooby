#ifndef __HDetailView_H__
#define __HDetailView_H__

#include <View.h>
#include <TextControl.h>
#include <List.h>

//class ROTextControl;

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
protected:
			void	InitGUI();
private:
	BTextControl	*fSubject;
	BTextControl	*fFrom;
	BTextControl	*fWhen;
		bool		fReadOnly;
};
#endif
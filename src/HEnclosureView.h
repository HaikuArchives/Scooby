#ifndef __HENCLOSUREVIEW_H__
#define __HENCLOSUREVIEW_H__

#include <View.h>
#include <ListView.h>
#include "ArrowButton.h"

enum{
	M_EXPAND_ENCLOSURE = 'ExEN'
};

class HEnclosureView :public BView{
public:
						HEnclosureView(BRect rect);
	virtual				~HEnclosureView();
			void		InitGUI();
			void		AddEnclosure(entry_ref ref);
			void		AddEnclosure(const char* data);
			
			void		RemoveEnclosure(int32 index);
			
			void		SetEncoding(int32 encoding);
		status_t		SetEncoding(const char* str);
			int32		GetEncoding();
protected:
	virtual void		MessageReceived(BMessage *message);
	virtual void		KeyDown(const char* bytes,int32 numBytes);
			void		WhenDropped(BMessage *message);
private:
	BListView*			fListView;
	ArrowButton*		fArrowButton;
};
#endif
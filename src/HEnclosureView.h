#ifndef __HENCLOSUREVIEW_H__
#define __HENCLOSUREVIEW_H__

#include <View.h>
#include <ListView.h>
#include "ArrowButton.h"

enum{
	M_EXPAND_ENCLOSURE = 'ExEN'
};

//! Enclosure view for compose window.(It contains encoding setting panel for outgoing mails.)
class HEnclosureView :public BView{
public:
			//!Constructor.
						HEnclosureView(BRect rect);
			//!Destructor.
						~HEnclosureView();
			//!Initalize all GUI.
			void		InitGUI();
			//!Add enclosure item by entry_ref.
			void		AddEnclosure(entry_ref ref/*!Attachment file's entry_ref.*/);
			//!Add enclosure item by data.
			void		AddEnclosure(const char* data/*!Attachment data.(for sent mails only)*/);
			//!Remove enclosure item by index.
			/*!
				Removed item will be freed.
			*/
			void		RemoveEnclosure(int32 index);
			// Not attachment functions.
			//! Set outgoing mails encoding by BeOS internal encoding name.
			void		SetEncoding(int32 encoding);
			//! Set outgoing mails encoding by charset string. If unsupported charset is passed, returns B_ERROR.
		status_t		SetEncoding(const char* str);
			//! Returns current encoding.
			int32		GetEncoding();
protected:
	//@{
	//!Override function.
		 	void		MessageReceived(BMessage *message);
		 	void		KeyDown(const char* bytes,int32 numBytes);
	//@}
	//!Handle drop files action.
			void		WhenDropped(BMessage *message);
private:
	BListView*			fListView;
	ArrowButton*		fArrowButton;
};
#endif
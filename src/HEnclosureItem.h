#ifndef __HENCLOSUREITEM_H__
#define __HENCLOSUREITEM_H__

#include <ListItem.h>
#include <Bitmap.h>
#include <Entry.h>
#include <String.h>

//!Enclosure listitem for compose window.
class HEnclosureItem :public BListItem{
public:
	//! Constructor.
					HEnclosureItem(entry_ref ref);
	//! Constructor for sent mails.			
					HEnclosureItem(const char* data);
	//! Destructor
					~HEnclosureItem();
	//! Returns mail file entry_ref. (for not sent mails)
		entry_ref	Ref()const {return fRef;}
	//! Returns true if attachment is based on entry_ref.
	/*!
		Sent mails don't have attachment file's entry_ref. 
		They only have attachment data.
	*/
		bool		HasRef() const {return fHaveRef;}
	//! Returns attachment data. (for sent mails)
	const char*		Data() const {return fDataBuf;}
	
protected:
	//@{
	//!Override function.
		 void	DrawItem(BView *owner, BRect frame, bool complete);
		 void 	Update(BView* owner, const BFont* font);
	//@}
	//!Make attachment size label.
			void	MakeSizeLabel(char **out,off_t size);
private:
	entry_ref			fRef;			//!<Attachment file entry_ref.
	BBitmap*			fBitmap;		//!<ListItem bitmap.
	BString				fName; 			//!<UTF8 name.
	float				fFontHeight;	//!<Font height for calc item height.
	char*				fDataBuf;		//!<Attachment data.
	bool				fHaveRef;		//!<Flag whether this has entry_ref.
};
#endif
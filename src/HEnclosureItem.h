#ifndef __HENCLOSUREITEM_H__
#define __HENCLOSUREITEM_H__

#include <ListItem.h>
#include <Bitmap.h>
#include <Entry.h>
#include <String.h>

class HEnclosureItem :public BListItem{
public:
					HEnclosureItem(entry_ref ref);
					HEnclosureItem(const char* data);
					
	virtual			~HEnclosureItem();
		entry_ref	Ref()const {return fRef;}
		
		bool		HasRef() const {return fHaveRef;}
	const char*		Data() const {return fDataBuf;}
	
protected:
	virtual void	DrawItem(BView *owner, BRect frame, bool complete);
	virtual void 	Update(BView* owner, const BFont* font);
	
			void	MakeSizeLabel(char **out,off_t size);
private:
	entry_ref			fRef;
	BBitmap*			fBitmap;
	BString				fName;
	float				fFontHeight;
	char*				fDataBuf;
	bool				fHaveRef;
};
#endif


#ifndef __HIMAP4ITEM_H__
#define __HIMAP4ITEM_H__

#include "HMailItem.h"

class IMAP4Client;
class HIMAP4Folder;

class HIMAP4Item :public HMailItem {
public:
						HIMAP4Item(const char* status,
									const char* subject,
									const char* from,
									const char* to,
									const char* cc,
									const char* reply,
									time_t when,
									const char* priority,
									int8 		enclosure,
									int32		index,
									IMAP4Client	*client,
									HIMAP4Folder *folder);
						~HIMAP4Item();
	//@{
	//!Override function.
			void		SetRead();
			void		RefreshStatus();
			void		RefreshEnclosureAttr();
	
			entry_ref	Ref();
	//@}
			void		Delete();
			
			bool		IsDownloaded() const { return fGotContent;}
protected:
			void		CalcHeaderLength();
private:
	int32				fMailIndex;
	IMAP4Client			*fClient;
	BString				fContent;
	bool				fGotContent;
	int32				fHeaderLength;
	HIMAP4Folder		*fFolder;
};
#endif
#ifndef __HIMAP4ITEM_H__
#define __HIMAP4ITEM_H__

#include "HMailItem.h"
#include "HIMAP4Folder.h"

class IMAP4Client;
class HIMAP4Folder;

//!IMAP4 mail item.
class HIMAP4Item :public HMailItem {
public:
				//!Constructor.
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
				//!Destructor.
						~HIMAP4Item();
	//@{
	//!Override function.
			void		SetRead();
			void		RefreshStatus();
			void		RefreshEnclosureAttr();
	
			entry_ref	Ref();
	//@}
			//!Mark this mail as delete, decrese unread count and invalidate folder label.
			void		Delete();
			//!Returns server side mail index.
			int32		Index() const {return fMailIndex;}
			//!Returns account name of this mail.
		const char*		AccountName() const {return fFolder->AccountName();}
			//!Returns true if mail contents have been downloaded. 
			bool		IsDownloaded() const { return fGotContent;}
			//!
			void		SetFolder(HIMAP4Folder *folder){fFolder = folder;}
protected:
			void		CalcHeaderLength();
private:
	int32				fMailIndex;		//!<Server side mail index.
	IMAP4Client			*fClient;		//!<IMAP4 socket.
	BString				fContent;		//!<Mail message body.
	bool				fGotContent;	//!<
	int32				fHeaderLength;	//!<Mail header length.
	HIMAP4Folder		*fFolder;		//!<Folder pointer storing this mail.
};
#endif
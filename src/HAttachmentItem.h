#ifndef __HATTACHMENTITEM_H__
#define __HATTACHMENTITEM_H__

#include <Entry.h>
#include <File.h>
#include <String.h>
#include "CLVEasyItem.h"

//! Attachment item for AttachmentList in HTML view mode.
/*!
 *	This class is used for incomming mails in HTMLView. not used in plain mode.
 */
class HAttachmentItem :public CLVEasyItem {
public:
			//! Constructor
						HAttachmentItem(const char* name /*! Attachment name.
														It will be converted to UTF8.
														If name is NULL, it become "Unknown".
														*/
										,off_t	file_offset //! File offset that attachment is stored.
										,int32	len //! Attachment data length.
										,const char *content_type //! Attachment content type.
										,const char *encoding	//! Attachment encoding.
										,const char *charset	//! Attachment charset.
										);
			//! Destructor
						~HAttachmentItem();
			//! Returns attachment content type.
		const char*		ContentType() {return fContentType;}
			//! Returns attachment content encoding.
		const char*		ContentEncoding() {return fEncoding;}
			//! Returns attachment content charset.
		const char*		Charset() {return fCharset;}
			//! Returns attachment content charset.
		const char*		Name() {return fName.String();}
			//! Returns attachment data length.
			int32		DataLength() {return fDataLen;}
			//! File offset that attachment is stored.
			int32		Offset() {return fFileOffset;}	
			//! Set extracted flag. If it set true, not extracted again.
				void	SetExtracted(bool extracted) {fExtracted = extracted;}
			//! Set mail file's entry_ref
				void	SetFileRef(entry_ref &ref) {fFileRef = ref;}
			//! Returns mail file's entry_ref	
			entry_ref	FileRef() {return fFileRef;}
			//! Returns true if attachment is extracted.
				bool	IsExtracted() const {return fExtracted;}
protected:
	//! Attachment file offset.
	off_t				fFileOffset;
	//! Attachment data length.
	int32				fDataLen;
	//! Flag whether attachment has already extracted.
	bool				fExtracted;
	//! Attachment content type.
	char*				fContentType;
	//! Attachment encoding.
	char*				fEncoding;
	//! Attachment charset.
	char*				fCharset;
	//! Attachment file's entry_ref.
	entry_ref			fFileRef;
	//! Attachment UTF8 encoded name.
	BString				fName;
};
#endif

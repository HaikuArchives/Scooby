#ifndef __HATTACHMENTITEM_H__
#define __HATTACHMENTITEM_H__

#include <Entry.h>
#include <File.h>
#include "CLVEasyItem.h"

class HAttachmentItem :public CLVEasyItem {
public:
						HAttachmentItem(const char* name,
										off_t	file_offset,
										int32	len,
										const char *content_type,
										const char *encoding,
										const char *charset);
	virtual				~HAttachmentItem();
		
		const char*		ContentType() {return fContentType;}
		const char*		Encoding() {return fEncoding;}
		const char*		Charset() {return fCharset;}
		const char*		Name() {return fName;}
			int32		DataLength() {return fDataLen;}
			int32		Offset() {return fFileOffset;}	

				void	SetExtracted(bool extracted) {fExtracted = true;}
				void	SetFileRef(entry_ref &ref) {fFileRef = ref;}
				
			entry_ref	FileRef() {return fFileRef;}
				bool	IsExtracted() const {return fExtracted;}
protected:
	off_t				fFileOffset;
	int32				fDataLen;
	bool				fExtracted;
	char*				fContentType;
	char*				fEncoding;
	char*				fCharset;
	entry_ref			fFileRef;
	char*				fName;
};
#endif
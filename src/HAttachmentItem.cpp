#include "HAttachmentItem.h"
#include "Encoding.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Mime.h>
#include <Bitmap.h>
#include <Debug.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HAttachmentItem::HAttachmentItem(const char* name,
									off_t	file_offset,
									int32	data_len,
									const char *content_type,
									const char *encoding,
									const char	*charset)
	:CLVEasyItem(0,false,false,20.0)
	,fFileOffset(file_offset)
	,fDataLen(data_len)
	,fExtracted(false)
	,fContentType(NULL)
	,fEncoding(NULL)
	,fCharset(NULL)
	,fName(NULL)
{
	fName = (name)?name:"Unknown";
	
	Encoding().Mime2UTF8(fName);
	
	SetColumnContent(1,fName.String());
	SetColumnContent(2,(content_type)?content_type:"(Unknown)");
	
	char *size = new char[15];
	float d = 0;
	char *unit = new char[6];
	if(data_len < 1024)
	{
		d = data_len;
		::strcpy(unit,"bytes");
	}else if(data_len >= 1024 && data_len < 1024*1024){
		d = data_len/1024.0;
		::strcpy(unit,"KB");
	}else{
		d = data_len/(1024.0*1024.0);
		::strcpy(unit,"MB");
	}
	::sprintf(size,"%6.2f %s",d,unit);
	SetColumnContent(3,size);
	delete[] size;
	delete[] unit;
	BBitmap *bitmap = new BBitmap(BRect(0,0,15,15),B_CMAP8);
	BMimeType preferredApp;
	if(content_type)
	{
		BMimeType mime(content_type);
		char prefApp[B_MIME_TYPE_LENGTH];
		mime.GetPreferredApp(prefApp);
		preferredApp.SetTo(prefApp);
		fContentType = ::strdup(content_type);
		if(preferredApp.GetIconForType(content_type,bitmap,B_MINI_ICON) != B_OK)
		{
			mime.GetIcon(bitmap,B_MINI_ICON);
		}
		SetColumnContent(0,bitmap);
	}
	
	delete bitmap;
	if(encoding)
		fEncoding = ::strdup(encoding);
	if(charset)
		fCharset = ::strdup(charset);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HAttachmentItem::~HAttachmentItem()
{
	free(fContentType);
	free(fCharset);
	free(fEncoding);
}
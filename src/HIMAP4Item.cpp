#include "HIMAP4Item.h"
#include "IMAP4Client.h"
#include "HIMAP4Folder.h"

#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <E-mail.h>
#include <NodeInfo.h>
#include <stdio.h>
#include <Debug.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HIMAP4Item::HIMAP4Item(const char* status,
						const char* subject,
						const char* from,
						const char* to,
						const char* cc,
						const char* reply,
						time_t		 when,
						const char* priority,
						int8 		enclosure,
						int32		index,
						IMAP4Client *client,
						HIMAP4Folder *folder)
	:HMailItem(status,subject,from,to,cc,reply,when,priority,enclosure)
	,fMailIndex(index)
	,fClient(client)
	,fGotContent(false)
	,fHeaderLength(0)
	,fFolder(folder)
{

}

/***********************************************************
 * SetRead
 ***********************************************************/
void
HIMAP4Item::SetRead()
{
	if(fStatus.Compare("New") != 0)
		return;

	fClient->MarkAsRead(fMailIndex);
	fFolder->SetUnreadCount(fFolder->Unread()-1);
	fFolder->InvalidateMe();
	fStatus = "Read";
	ResetIcon();
}

/***********************************************************
 * Delete
 ***********************************************************/
void
HIMAP4Item::Delete()
{
	fClient->MarkAsDelete(fMailIndex);
	PRINT(("IMAP MAIL DELETED\n"));
	if(fStatus.Compare("New") == 0)
	{
		fFolder->SetUnreadCount(fFolder->Unread()-1);
		fFolder->InvalidateMe();
	}
}

/***********************************************************
 * ResetIcon
 ***********************************************************/
void
HIMAP4Item::RefreshStatus()
{
}

/***********************************************************
 * RefreshEnclosureAttr
 ***********************************************************/
void
HIMAP4Item::RefreshEnclosureAttr()
{
}

/***********************************************************
 * CalcHeaderLength
 ***********************************************************/
void
HIMAP4Item::CalcHeaderLength()
{
	int32 index = fContent.FindFirst("\r\n\r\n");
	if(index != B_ERROR)
		fHeaderLength = index + 4;
}

/***********************************************************
 * Ref
 ***********************************************************/
entry_ref
HIMAP4Item::Ref()
{
	BPath path;
	entry_ref ref;
	status_t err = B_OK;
	int i = 1;
	BString mail_name(fSubject);
	mail_name.ReplaceAll("/","_");
	mail_name.ReplaceAll(":","_");
	mail_name.ReplaceAll("\n","");
	mail_name.ReplaceAll("\r","");

	::find_directory(B_SYSTEM_TEMP_DIRECTORY,&path);

	char *name = new char[mail_name.Length()+10];
	::sprintf(name,"%s.imap",mail_name.String());

	while(err == B_OK)
	{
		path.Append(name);
		err = BNode(path.Path()).InitCheck();
		if(err != B_OK)
			break;
		::sprintf(name,"%s_%d.imap",mail_name.String(),i++);
		path.GetParent(&path);
	}
	delete[] name;

	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);

	if(!fGotContent)
	{
		fClient->FetchBody(fMailIndex,fContent);
		fGotContent = true;
	}
	int32 len = fContent.Length();

	if(fHeaderLength == 0)
		CalcHeaderLength();

	int32 content_len = len - fHeaderLength;

	file.Write(fContent.String(),len);
	file.SetSize(len);
	file.WriteAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&fHeaderLength,sizeof(int32));
	file.WriteAttr(B_MAIL_ATTR_CONTENT,B_INT32_TYPE,0,&content_len,sizeof(int32));
	file.WriteAttrString(B_MAIL_ATTR_SUBJECT,&fSubject);
	file.WriteAttrString(B_MAIL_ATTR_TO,&fTo);
	file.WriteAttrString(B_MAIL_ATTR_CC,&fCC);
	file.WriteAttrString(B_MAIL_ATTR_FROM,&fFrom);
	file.WriteAttrString(B_MAIL_ATTR_REPLY,&fReply);
	file.WriteAttrString(B_MAIL_ATTR_PRIORITY,&fPriority);
	const char *mime_version = "1.0";
	file.WriteAttr(B_MAIL_ATTR_MIME,B_STRING_TYPE,0,&mime_version,
												strlen(mime_version)+1);
	file.WriteAttr(B_MAIL_ATTR_WHEN,B_TIME_TYPE,0,&fWhen,sizeof(int32));
	BNodeInfo(&file).SetType(B_MAIL_TYPE);
	::get_ref_for_path(path.Path(),&ref);
	return ref;
}

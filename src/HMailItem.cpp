#include "HMailItem.h"
#include "ResourceUtils.h"
#include "ExtraMailAttr.h"

#include <String.h>
#include <Entry.h>
#include <E-mail.h>
#include <File.h>
#include <Bitmap.h>
#include <NodeInfo.h>
#include <fs_attr.h>
#include <Path.h>
#include <Beep.h>
#include <stdio.h>
#include <Alert.h>
#include <Debug.h>
#include <ClassInfo.h>
#include <stdlib.h>

#define TIME_FORMAT "%a, %m/%d/%Y, %r"

/***********************************************************
 * Constructor
 ***********************************************************/
HMailItem::HMailItem(const entry_ref &ref)
: CLVEasyItem(0, false, false, 18.0)
	,fRef(ref)
	,fStatus("")
	,fSubject("")
	,fFrom("")
	,fTo("")
	,fCC("")
	,fWhen(0)
	,fDate("")
	,fPriority("")
	,fEnclosure(-1)
	,fDeleteMe(false)
{
	InitItem();
}

/***********************************************************
 * Constructor for cache data
 ***********************************************************/
HMailItem::HMailItem(const entry_ref &ref,
					  const char* status,
					  const char* subject,
					  const char* from,
					  const char* to,
					  const char* cc,
					  time_t 	  when,
					  const char* priority,
					  int8 enclosure)
	:CLVEasyItem(0, false, false, 18.0)
	,fRef(ref)
	,fStatus(status)
	,fSubject(subject)
	,fFrom(from)
	,fTo(to)
	,fCC(cc)
	,fWhen(when)
	,fPriority(priority)
	,fEnclosure(enclosure)
	,fDeleteMe(false)
{
	//fDate = ctime(&fWhen);
	struct tm* time = localtime(&fWhen);
	char *tmp = fDate.LockBuffer(24);
	::strftime(tmp, 64,TIME_FORMAT, time);
	fDate.UnlockBuffer();
	SetColumnContent(1,fSubject.String());
	SetColumnContent(2,fFrom.String());
	SetColumnContent(3,fTo.String());
	SetColumnContent(4,fDate.String());
	ResetIcon();
}

/***********************************************************
 * Constructor
 ***********************************************************/
HMailItem::HMailItem(const char* status,
					const char*	subject,
					const char* from,
					const char* to,
					time_t	  when,
					const char* priority,
					int8 enclosure)
	:CLVEasyItem(0, false, false, 18.0)
	,fStatus(status)
	,fSubject(subject)
	,fFrom(from)
	,fTo(to)
	,fCC("")
	,fWhen(when)
	,fPriority(priority)
	,fEnclosure(enclosure)
	,fDeleteMe(false)
{
	struct tm* time = localtime(&fWhen);
	char *tmp = fDate.LockBuffer(24);
	::strftime(tmp, 64,TIME_FORMAT, time);
	fDate.UnlockBuffer();
	SetColumnContent(1,fSubject.String());
	SetColumnContent(2,fFrom.String());
	SetColumnContent(3,fTo.String());
	SetColumnContent(4,fDate.String());
	ResetIcon();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HMailItem::~HMailItem()
{

}

/***********************************************************
 * IsRead
 ***********************************************************/
bool
HMailItem::IsRead()const
{
	if(fStatus.Length() ==0)
		return true;
	return (fStatus.Compare("New")==0)?false:true;
}

/***********************************************************
 * SetRead
 ***********************************************************/
void
HMailItem::SetRead()
{
	BNode node(&fRef);
	if(node.InitCheck() == B_OK)
	{
		node.ReadAttrString(B_MAIL_ATTR_STATUS,&fStatus);
		if(fStatus.Compare("New")!=0 && fStatus.Compare("Read")!=0)
			return;
	
		node.WriteAttr(B_MAIL_ATTR_STATUS,B_STRING_TYPE,0,"Read",5);
		fStatus = "Read";
		ResetIcon();
	}
}

/***********************************************************
 * ResetIcon
 ***********************************************************/
void
HMailItem::ResetIcon()
{
	ResourceUtils utils;
	BBitmap *icon = utils.GetBitmapResource('BBMP',fStatus.String());
	RefreshTextColor();
	
	if(icon)
		SetColumnContent(0,icon,2.0,true,false);
	delete icon;
	
	icon = utils.GetBitmapResource('BBMP',fPriority.String());
	if(icon)
		SetColumnContent(5,icon,2.0,true,false);
	delete icon;
	
	if(fEnclosure == 1)
	{
		icon = utils.GetBitmapResource('BBMP',"Enclosure");
		if(icon)
			SetColumnContent(6,icon,2.0,true,false);
		delete icon;
	}
}

/***********************************************************
 * RefreshTextColor
 ***********************************************************/
void
HMailItem::RefreshTextColor()
{
	if(fStatus.Compare("New") == 0)
		SetTextColor(0,0,200);
	else if(fStatus.Compare("Error") == 0)
		SetTextColor(200,0,0);
	else
		SetTextColor(0,0,0);
}

/***********************************************************
 * RefreshStatus
 ***********************************************************/
void
HMailItem::RefreshStatus()
{
	BNode node(&fRef);
	if(node.InitCheck() == B_OK)
	{
		node.ReadAttrString(B_MAIL_ATTR_STATUS,&fStatus);
		RefreshTextColor();
		ResetIcon();
	}
}

/***********************************************************
 * InitItem
 ***********************************************************/
void
HMailItem::InitItem()
{
	BEntry entry(&fRef);
	attr_info attr;
	BNode node(&fRef);
	
	if(node.InitCheck() == B_OK)
	{
		if(node.GetAttrInfo(B_MAIL_ATTR_SUBJECT,&attr) == B_OK)
			node.ReadAttrString(B_MAIL_ATTR_SUBJECT,&fSubject);
		if(node.GetAttrInfo(B_MAIL_ATTR_FROM,&attr) == B_OK)
			node.ReadAttrString(B_MAIL_ATTR_FROM,&fFrom);
		if(node.GetAttrInfo(B_MAIL_ATTR_TO,&attr) == B_OK)
			node.ReadAttrString(B_MAIL_ATTR_TO,&fTo);
		if(node.GetAttrInfo(B_MAIL_ATTR_STATUS,&attr) == B_OK)
			node.ReadAttrString(B_MAIL_ATTR_STATUS,&fStatus);
		if(node.GetAttrInfo(B_MAIL_ATTR_WHEN,&attr) == B_OK)
		{	
			node.ReadAttr(B_MAIL_ATTR_WHEN,B_TIME_TYPE,0,&fWhen,sizeof(time_t));
			//fDate = ctime(&fWhen);
			struct tm* time = localtime(&fWhen);
			char *tmp = fDate.LockBuffer(30);
			::strftime(tmp, 64,TIME_FORMAT, time);
			fDate.UnlockBuffer();
		}
		if(node.GetAttrInfo(B_MAIL_ATTR_PRIORITY,&attr) == B_OK)
			node.ReadAttrString(B_MAIL_ATTR_PRIORITY,&fPriority);
		int32 priority = atoi(fPriority.String() );
		fPriority = "";
		fPriority << priority;
	
		if(node.GetAttrInfo(B_MAIL_ATTR_ATTACHMENT,&attr) == B_OK)
		{
			bool enclosure;
			node.ReadAttr(B_MAIL_ATTR_ATTACHMENT,B_BOOL_TYPE,0,&enclosure,sizeof(bool));
			fEnclosure = (enclosure)?1:0;
		}else
			fEnclosure = -1;
		
		SetColumnContent(1,fSubject.String());
		SetColumnContent(2,fFrom.String());
		SetColumnContent(3,fTo.String());
		SetColumnContent(4,fDate.String());
		//SetColumnContent(5,fPriority.String(),false);
		ResetIcon();
	}
	return;
}


/***********************************************************
 * CompareItems
 ***********************************************************/
int HMailItem::CompareItems(const CLVListItem *a_Item1, 
								const CLVListItem *a_Item2, 
								int32 KeyColumn)
{
	const HMailItem* Item1 = cast_as(a_Item1,const HMailItem);
	const HMailItem* Item2 = cast_as(a_Item2,const HMailItem);
	
	if(KeyColumn == 4)
	{
		if(Item1->fWhen > Item2 ->fWhen)
			return 1;
		else if(Item1->fWhen == Item2->fWhen)
			return 0;
		else
			return -1;
	}else{
	if(Item1 == NULL || Item2 == NULL || Item1->m_column_types.CountItems() <= KeyColumn ||
		Item2->m_column_types.CountItems() <= KeyColumn)
		return 0;
	
	int32 type1 = ((int32)Item1->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;
	int32 type2 = ((int32)Item2->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;

	if(!((type1 == CLVColStaticText || type1 == CLVColTruncateText || type1 == CLVColTruncateUserText ||
		type1 == CLVColUserText) && (type2 == CLVColStaticText || type2 == CLVColTruncateText ||
		type2 == CLVColTruncateUserText || type2 == CLVColUserText)))
		return 0;

	const char* text1 = NULL;
	const char* text2 = NULL;

	if(type1 == CLVColStaticText || type1 == CLVColTruncateText)
		text1 = (const char*)Item1->m_column_content.ItemAt(KeyColumn);
	else if(type1 == CLVColTruncateUserText || type1 == CLVColUserText)
		text1 = Item1->GetUserText(KeyColumn,-1);

	if(type2 == CLVColStaticText || type2 == CLVColTruncateText)
		text2 = (const char*)Item2->m_column_content.ItemAt(KeyColumn);
	else if(type2 == CLVColTruncateUserText || type2 == CLVColUserText)
		text2 = Item2->GetUserText(KeyColumn,-1);
	
	return strcasecmp(text1,text2);
	}
	return 0;
}


/***********************************************************
 * SetHasEnclosure
 ***********************************************************/
void
HMailItem::RefreshEnclosureAttr()
{
	if(fEnclosure != -1)
		return;
	
	BFile file(&fRef,B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	int32 header_len;
	file.ReadAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&header_len,sizeof(int32));
	char *header = new char[header_len+1];
	header_len = file.Read(header,header_len);
	header[header_len] = '\0';
	
	fEnclosure = (strstr(header,"Content-Type: multipart"))?1:0;
	bool enclosure = (fEnclosure)?true:false;
	file.WriteAttr(B_MAIL_ATTR_ATTACHMENT,B_BOOL_TYPE,0,&enclosure,sizeof(bool));
	PRINT(("Attachment ATTR Refreshed\n"));
	delete[] header;
	BBitmap *icon;
	
	if(fEnclosure == 1)
	{
		icon = ResourceUtils().GetBitmapResource('BBMP',"Enclosure");
		if(icon)
			SetColumnContent(6,icon,2.0,true,false);
		delete icon;
	}else
		SetColumnContent(6,NULL,2.0,true,false);
}
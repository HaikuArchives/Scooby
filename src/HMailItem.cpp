#include "ResourceUtils.h"
#include "ExtraMailAttr.h"
#include "HApp.h"
#include "HPrefs.h"
#include "HMailItem.h"

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
#include <View.h>
#include <StopWatch.h>

#define TIME_FORMAT "%a, %d %b %Y %r"

const rgb_color kBorderColor = ui_color(B_PANEL_BACKGROUND_COLOR);

/***********************************************************
 * Constructor
 ***********************************************************/
HMailItem::HMailItem(const entry_ref &ref)
: _inherited(0, false, false, 18.0)
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
	,fInitThread(-1)
{
	BEntry entry(&ref);
	entry.GetNodeRef(&fNodeRef);
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
					  const char* reply,
					  time_t 	  when,
					  const char* priority,
					  int8 enclosure,
					  ino_t	node,BListView *view)
	:_inherited(0, false, false, 18.0)
	,fRef(ref)
	,fStatus(status)
	,fSubject(subject)
	,fFrom(from)
	,fTo(to)
	,fCC(cc)
	,fReply(reply)
	,fWhen(when)
	,fPriority(priority)
	,fEnclosure(enclosure)
	,fDeleteMe(false)
	,fInitThread(-1)
	,fOwner(view)
{
	if(node == 0)
	{
		BEntry entry(&fRef);
		entry.GetNodeRef(&fNodeRef);
	}else{
		fNodeRef.node = node;
		fNodeRef.device = ref.device;
	}

	MakeTime(fDate);

	SetColumnContent(1,fSubject.String());
	SetColumnContent(2,fFrom.String());
	SetColumnContent(3,fTo.String());
	SetColumnContent(4,fDate.String());
	
	//fInitThread = ::spawn_thread(RefreshStatusWithThread,"MailInitThread",B_NORMAL_PRIORITY,this);
	//::resume_thread(fInitThread);
	
	if(fStatus.Compare("New") == 0)
		RefreshStatus();
	else
		ResetIcon();
}

/***********************************************************
 * Constructor
 ***********************************************************/
HMailItem::HMailItem(const char* status,
					const char*	subject,
					const char* from,
					const char* to,
					const char* cc,
					const char* reply,
					time_t	  when,
					const char* priority,
					int8 enclosure)
	:_inherited(0, false, false, 18.0)
	,fStatus(status)
	,fSubject(subject)
	,fFrom(from)
	,fTo(to)
	,fCC(cc)
	,fReply(reply)
	,fWhen(when)
	,fPriority(priority)
	,fEnclosure(enclosure)
	,fDeleteMe(false)
	,fInitThread(-1)
{
	MakeTime(fDate);
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
	// Wait if init thread is running
	if(fInitThread >= 0)
	{
		status_t err;
		::wait_for_thread(fInitThread,&err);
	}
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
		if(fStatus.Compare("New")!=0 )
			return;
		fStatus = "Read";
		PRINT(("Written\n"));
		node.WriteAttrString(B_MAIL_ATTR_STATUS,&fStatus);
		ResetIcon();
	}
}

/***********************************************************
 * RefreshStatusWithThread
 ***********************************************************/
int32
HMailItem::RefreshStatusWithThread(void *data)
{
	HMailItem *theItem = (HMailItem*)data;
	theItem->RefreshStatus();
	theItem->fInitThread = -1;

	if(theItem->fOwner && theItem->fOwner->Window()->Lock())
	{
		int32 index = theItem->fOwner->IndexOf(theItem);
		if(index >= 0)
			theItem->fOwner->InvalidateItem(index);
		theItem->fOwner->Window()->Unlock();
	}
	return 0;
}

/***********************************************************
 * ResetIcon
 ***********************************************************/
void
HMailItem::ResetIcon()
{
	HApp* app = (HApp*)be_app;
	
	BBitmap *icon = app->GetIcon(fStatus.String());
	
	RefreshTextColor();
	
	if(icon)
		SetColumnContent(0,icon,2.0,false);
	
	icon = app->GetIcon(fPriority.String());
	if(icon)
		SetColumnContent(5,icon,2.0,false,false);
	
	if(fEnclosure == 1)
	{
		icon = app->GetIcon("Enclosure");
		if(icon)
			SetColumnContent(6,icon,2.0,false,false);
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
			MakeTime(fDate);
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
	if(header_len >= 0)
	{
		char *header = new char[header_len+1];
		header_len = file.Read(header,header_len);
	
		header[header_len] = '\0';
		fEnclosure = (strstr(header,"Content-Type: multipart"))?1:0;
		bool enclosure = (fEnclosure)?true:false;
		file.WriteAttr(B_MAIL_ATTR_ATTACHMENT,B_BOOL_TYPE,0,&enclosure,sizeof(bool));
		PRINT(("Attachment ATTR Refreshed\n"));
		BBitmap *icon;
		
		if(fEnclosure == 1)
		{
			icon = ((HApp*)be_app)->GetIcon("Enclosure");
			if(icon)
				SetColumnContent(6,icon,2.0,false,false);
		}else
			SetColumnContent(6,NULL,2.0,true,false);
		delete[] header;	
	}
}

/***********************************************************
 * DrawItemColumn
 ***********************************************************/
void
HMailItem::DrawItemColumn(BView* owner, 
									BRect item_column_rect, 
									int32 column_index, 
									bool complete)
{
	_inherited::DrawItemColumn(owner,item_column_rect,column_index,complete);
	// Stroke line
	rgb_color old_col = owner->HighColor();
	
	owner->SetHighColor(kBorderColor);
	
	BPoint start,end;
	start.y = end.y = item_column_rect.bottom;
	start.x = 0;
	end.x = owner->Bounds().right;
	
	owner->StrokeLine(start,end);
	owner->SetHighColor(old_col);
}

/***********************************************************
 * MakeTime
 ***********************************************************/
void
HMailItem::MakeTime(BString &out)
{
	struct tm* time = localtime(&fWhen);
	char *tmp = out.LockBuffer(64);

	const char* kTimeFormat;
	((HApp*)be_app)->Prefs()->GetData("time_format",&kTimeFormat);
	::strftime(tmp, 64,kTimeFormat, time);
	out.UnlockBuffer();
}
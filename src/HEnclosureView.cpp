#include "HEnclosureView.h"
#include "HEnclosureItem.h"
#include "HApp.h"
#include "Encoding.h"

#include <StringView.h>
#include <ClassInfo.h>
#include <ScrollView.h>
#include <Debug.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HEnclosureView::HEnclosureView(BRect rect)
	:BView(rect,"EnclosureView",B_FOLLOW_TOP|B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

/***********************************************************
 * Destructor
 ***********************************************************/
HEnclosureView::~HEnclosureView()
{
	int32 count = fListView->CountItems();
	while(count>0)
	{
		HEnclosureItem *item = cast_as(fListView->RemoveItem(--count),HEnclosureItem);
		if(item)
			delete item;
	}
}

/***********************************************************
 * InitGUI()
 ***********************************************************/
void
HEnclosureView::InitGUI()
{
	BRect rect(Bounds());
	rect.InsetBy(5,1);
	rect.bottom = rect.top + 20;
	rect.left+= 16;
	
	Encoding encode;
	BMenu *menu = new BMenu("encode");
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	int32 numCharset = encode.CountCharset();
	for(int32 i = 0;i < numCharset;i++)
		menu->AddItem(new BMenuItem(encode.Charset(i),new BMessage(encode.Conversion(i))));
	BMenuField *field = new BMenuField(rect,"encoding",_("Encoding:"),menu);
	field->SetDivider(field->StringWidth(_("Encoding:"))+14);
	AddChild(field);
	rect.left-= 16;
	
	
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 14;
	rect.right = rect.left + 16;
	
	fArrowButton = new ArrowButton(rect,"enc_arrow",new BMessage(M_EXPAND_ENCLOSURE));
	AddChild(fArrowButton);
	rect.left = rect.right + 3;
	rect.right = Bounds().right - 5;
	
	BStringView *stringView = new BStringView(rect,
											"StringView",
											_("Attachments"),
											B_FOLLOW_TOP|B_FOLLOW_LEFT_RIGHT);
	AddChild(stringView);

	const float kDivider = StringWidth(_("Subject:")) +5;
	rect.OffsetBy(0,rect.Height()+7);
	rect.bottom = rect.top + 45;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.left = kDivider + 27;
	fListView = new BListView(rect,"listview",B_SINGLE_SELECTION_LIST
							,B_FOLLOW_TOP|B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW);
	BScrollView *scroll = new BScrollView("scrollview",fListView,
							B_FOLLOW_TOP|B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW,
							false,
							true);
	AddChild(scroll);
}


/***********************************************************
 * AddEnclosure
 ***********************************************************/
void
HEnclosureView::AddEnclosure(entry_ref ref)
{
	fListView->AddItem(new HEnclosureItem(ref));
}

/***********************************************************
 * AddEnclosure
 ***********************************************************/
void
HEnclosureView::AddEnclosure(const char* data)
{
	fListView->AddItem(new HEnclosureItem(data));
}

/***********************************************************
 * RemoveEnclosure
 ***********************************************************/
void
HEnclosureView::RemoveEnclosure(int32 index)
{
	HEnclosureItem *item =cast_as(fListView->RemoveItem(index),HEnclosureItem);
	delete item;
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HEnclosureView::MessageReceived(BMessage *message)
{
	if(message->WasDropped())
		WhenDropped(message);
	else
		BView::MessageReceived(message);
}

/***********************************************************
 * WhenDropped
 ***********************************************************/
void
HEnclosureView::WhenDropped(BMessage *message)
{
	entry_ref ref;
	int32 count;
	type_code type;
	message->GetInfo("refs",&type,&count);
	
	for(int32 i = 0;i < count;i++)
	{
		if(message->FindRef("refs",i,&ref) == B_OK)
			AddEnclosure(ref);
	}
}

/***********************************************************
 * KeyDown
 ***********************************************************/
void
HEnclosureView::KeyDown(const char* bytes,int32 numBytes)
{
	if(numBytes == 1)
	{
		if(bytes[0] == B_DELETE)
		{
			int32 sel = fListView->CurrentSelection();
			if(sel >= 0)
				RemoveEnclosure(sel);
		}
	}
	BView::KeyDown(bytes,numBytes);
}

/***********************************************************
 * SetEncoding
 ***********************************************************/
void
HEnclosureView::SetEncoding(int32 encoding)
{
	BMenuField *field = cast_as(FindView("encoding"),BMenuField);
	BMenu *menu = field->Menu();
	BMenuItem* item = menu->FindItem(encoding);
	if(item)
		item->SetMarked(true);
}

/***********************************************************
 * SetEncoding
 ***********************************************************/
status_t
HEnclosureView::SetEncoding(const char* str)
{
	Encoding encode;
	int32 encoding = -1;
	int32 numCharset = encode.CountCharset();
	for(int32 i = 0;i < numCharset;i++)
	{
		if(::strcasecmp(encode.Charset(i),str) == 0)
		{
			
			encoding = encode.Conversion(i);
			break;
		}
	}

	BMenuField *field = cast_as(FindView("encoding"),BMenuField);
	BMenu *menu = field->Menu();
	BMenuItem* item = menu->FindItem(encoding);
	if(!item)
		return B_ERROR;
		
	item->SetMarked(true);
	return B_OK;
}

/***********************************************************
 * Encoding
 ***********************************************************/
int32
HEnclosureView::GetEncoding()
{
	BMenuField *field = cast_as(FindView("encoding"),BMenuField);
	BMenu *menu = field->Menu();
	BMenuItem* item = menu->FindMarked();
	if(!item)
		return -1;
	return item->Message()->what;
}

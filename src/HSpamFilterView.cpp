#include "HSpamFilterView.h"
#include "HApp.h"
#include "HFile.h"

#include <Path.h>
#include <FindDirectory.h>
#include <String.h>
#include <Box.h>
#include <Button.h>
#include <ScrollView.h>
#include <ClassInfo.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HSpamFilterView::HSpamFilterView(BRect rect)
	:BView(rect,"spamfilter",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	rect.OffsetTo(B_ORIGIN);
	rect.InsetBy(10,10);
	rect.bottom -= 60;
	BBox *box = new BBox(rect,"box");
	box->SetLabel(_("BlackList"));
	BRect listRect(box->Bounds());
	listRect.OffsetTo(B_ORIGIN);
	listRect.InsetBy(20,20);
	listRect.OffsetBy(0,5);
	listRect.right -= B_V_SCROLL_BAR_WIDTH;
	listRect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fAddressList = new BListView(listRect,NULL,B_MULTIPLE_SELECTION_LIST);
	fAddressList->SetSelectionMessage(new BMessage(M_SPAM_SELECTION_CHANGED));
	BScrollView *scrollView = new BScrollView(NULL,fAddressList,B_FOLLOW_ALL,0,true,true);
	box->AddChild(scrollView);
	
	rect.OffsetBy(0,rect.Height()+10);
	rect.bottom = rect.top + 25;
	rect.right -= 200;
	fAddressCtrl = new BTextControl(rect,NULL,_("E-Mail:"),"",NULL);
	fAddressCtrl->SetModificationMessage(new BMessage(M_SPAM_ADDRESS_MODIFIED));
	fAddressCtrl->SetDivider(StringWidth(_("E-Mail:"))+3);
	AddChild(fAddressCtrl);
	
	rect.left = rect.right + 20;
	rect.right = rect.left + 70;
	fAddBtn = new BButton(rect,"add",_("Add"),new BMessage(M_SPAM_OK));
	fAddBtn->SetEnabled(false);
	AddChild(fAddBtn);
	
	rect.OffsetBy(rect.Width()+10,0);
	fDeleteBtn = new BButton(rect,"del",_("Delete"),new BMessage(M_SPAM_DEL));
	fDeleteBtn->SetEnabled(false);
	AddChild(fDeleteBtn);
	
	
	AddChild(box);	
	
	Load();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HSpamFilterView::~HSpamFilterView()
{
	fAddressList->SetSelectionMessage(NULL);
	fAddressCtrl->SetModificationMessage(NULL);
	
	Save();
	
	int32 count = fAddressList->CountItems();
	while(count > 0)
		delete fAddressList->RemoveItem(--count);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HSpamFilterView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_SPAM_ADDRESS_MODIFIED:
	{
		const char* text = fAddressCtrl->Text();
		if(::strlen(text) > 0)
			fAddBtn->SetEnabled(true);
		else
			fAddBtn->SetEnabled(false);
		break;
	}
	case M_SPAM_SELECTION_CHANGED:
	{
		if(fAddressList->CurrentSelection()>=0)
			fDeleteBtn->SetEnabled(true);
		else
			fDeleteBtn->SetEnabled(false);
		break;
	}
	case M_SPAM_DEL:
	{
		int32 index=0;
		int32 sel;
		BList removeItems;
		while((sel = fAddressList->CurrentSelection(index++)) >= 0)
		{
			removeItems.AddItem(fAddressList->ItemAt(sel));
		}
		int32 count = removeItems.CountItems();
		
		while(count>0)
		{
			BStringItem *item = (BStringItem*)removeItems.RemoveItem(--count);
			fAddressList->RemoveItem(item);
			delete item;
		}
		
		break;
	}
	case M_SPAM_OK:
	{
		fAddressList->AddItem(new BStringItem(fAddressCtrl->Text()));
		fAddressCtrl->SetText("");
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * Save
 ***********************************************************/
void
HSpamFilterView::Save()
{
	// Load list
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append( APP_NAME );
	path.Append("BlackList");
	
	HFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
	if(file.InitCheck() != B_OK)
		return;

	int32 count = fAddressList->CountItems();
	int32 totlen = 0;
	int32 len = 0;
	for(int32 i = 0;i < count;i++)
	{
		BStringItem *item = cast_as(fAddressList->ItemAt(i),BStringItem);
		if(!item)
			continue;
		const char* text = item->Text();
		len = ::strlen(text);
		
		file.Write(text,len);
		file.Write("\n",1);
		totlen+=len+1;
	}
	file.SetSize(totlen);
}

/***********************************************************
 * Load
 ***********************************************************/
void
HSpamFilterView::Load()
{
	// Load list
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append( APP_NAME );
	path.Append("BlackList");
	
	HFile file(path.Path(),B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	
	BString line;
	while(file.GetLine(&line) > 0)
	{
		line.RemoveAll("\n");
		if(line.Length() > 0)
			fAddressList->AddItem(new BStringItem(line.String()));
	}
}

/***********************************************************
 * AttachedToWindow
 ***********************************************************/
void
HSpamFilterView::AttachedToWindow()
{
	BButton *button;
	button = cast_as(FindView("add"),BButton);
	button->SetTarget(this);
	button = cast_as(FindView("del"),BButton);
	button->SetTarget(this);
	fAddressList->SetTarget(this);
	fAddressCtrl->SetTarget(this);
}
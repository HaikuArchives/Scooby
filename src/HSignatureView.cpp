#include "HSignatureView.h"
#include "HApp.h"
#include "TrackerUtils.h"
#include "HPrefs.h"

#include <Button.h>
#include <ScrollView.h>
#include <ClassInfo.h>
#include <FindDirectory.h>
#include <Path.h>
#include <File.h>
#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <Alert.h>
#include <Debug.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HSignatureView::HSignatureView(BRect rect)
	:BView(rect,"signature",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	InitGUI();
	
	HPrefs *prefs = ((HApp*)be_app)->Prefs();
	const char* family,*style;
	float size;
	
	prefs->GetData("font_family",&family);
	prefs->GetData("font_style",&style);
	prefs->GetData("font_size",&size);
	
	BFont font;
	font.SetFamilyAndStyle(family,style);
	font.SetSize(size);
	
	font.SetSpacing(B_FIXED_SPACING);
	fTextView->SetFontAndColor(&font);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HSignatureView::~HSignatureView()
{
	fListView->SetSelectionMessage(NULL);
	
	int32 count = fListView->CountItems();
	while(count>0)
	{
		BStringItem *item = cast_as(fListView->RemoveItem(--count),BStringItem);
		delete item;
	}
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HSignatureView::InitGUI()
{
	BRect rect(Bounds());
	rect.top += 10;
	rect.left +=5;
	rect.bottom-= 55;
	rect.right = rect.left +100;
	BRect frame;
	
	fListView = new BListView(rect,"list");
	fListView->SetFont(be_bold_font);
	fListView->SetSelectionMessage(new BMessage(M_CHANGE_SIGNATURE));
	BScrollView *scroll = new BScrollView("scrollview",fListView,
						B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT,
						0,false,true);
	AddChild(scroll);
	// Gather signatures
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Signatures");
	BDirectory dir(path.Path());
	BEntry entry;
	status_t err = B_OK;
	while(err == B_OK)
	{
		if((err = dir.GetNextEntry(&entry)) == B_OK )
		{
			char name[B_FILE_NAME_LENGTH+1];
			entry.GetName(name);
			fListView->AddItem( new BStringItem(name));
		}
	}
	//
	frame= rect;
	frame.top = frame.bottom + 5;
	frame.right= frame.left + 50;
	BButton *button;
	button = new BButton(frame,"del","Delete",new BMessage(M_DEL_SIGNATURE));
	button->SetEnabled(false);
	AddChild(button);
	frame.OffsetBy(55,0);
	button = new BButton(frame,"add","Add",new BMessage(M_ADD_SIGNATURE));
	AddChild(button);
	
	rect.OffsetBy(rect.Width()+30,0);
	rect.right= rect.left + 200;
	rect.bottom = rect.top + 35;
	fName = new BTextControl(rect,"name","Name:","",NULL);
	fName->SetDivider(StringWidth("Name:")+5);
	AddChild(fName);
	
	rect.right = Bounds().right - 5 - B_V_SCROLL_BAR_WIDTH;
	rect.OffsetBy(0,30);
	rect.bottom = Bounds().bottom - 55 - B_H_SCROLL_BAR_HEIGHT;
	fTextView = new CTextView(rect,"text",B_FOLLOW_ALL,B_WILL_DRAW);
	
	scroll = new BScrollView("scroll",fTextView,B_FOLLOW_ALL,
												B_WILL_DRAW,true,true);
	AddChild(scroll);
	
	// Apply change button
	rect.bottom = Bounds().bottom - 30;
	rect.right = Bounds().right - 5;
	rect.left = rect.right - 80;
	rect.top = rect.bottom - 20;
	
	button = new BButton(rect,"apply","Apply Change",new BMessage(M_SIGNATURE_SAVE_CHANGED));
	AddChild(button);	
	
	SetEnableControls(false);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HSignatureView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_ADD_SIGNATURE:
	{
		BPath folder_path;
		::find_directory(B_USER_SETTINGS_DIRECTORY,&folder_path);
		folder_path.Append(APP_NAME);
		folder_path.Append("Signatures");
		::create_directory(folder_path.Path(),0777);
		BPath path(folder_path.Path());
		path.Append("Untitled");
		
		BFile file;
		status_t err = B_ERROR;
		int32 i = 1;
		while(err != B_OK)
		{
			err = file.SetTo(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_FAIL_IF_EXISTS);
			if(err == B_OK)
				break;
			BString new_name("Untitled");
			new_name << i;
			path.SetTo(folder_path.Path());
			path.Append(new_name.String());
		}
		fListView->AddItem(new BStringItem(path.Leaf()));
		break;
	}
	case M_DEL_SIGNATURE:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel>=0)
		{
			fListView->DeselectAll();
			BStringItem *item = cast_as(fListView->RemoveItem(sel),BStringItem);
			BPath path;
			::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
			path.Append(APP_NAME);
			path.Append("Signatures");
			path.Append(item->Text());
			entry_ref ref;
			::get_ref_for_path(path.Path(),&ref);
			TrackerUtils().MoveToTrash(ref);
			
			delete item;
		}
		break;
	}
	case M_CHANGE_SIGNATURE:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel>=0)
			OpenItem(sel);
		else
			SetEnableControls(false);
		break;
	}
	case M_SIGNATURE_SAVE_CHANGED:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel>=0)
			SaveItem(sel);
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * Open
 ***********************************************************/
void
HSignatureView::OpenItem(int32 sel)
{
	if(sel < 0)
		return;
	BStringItem *item = cast_as(fListView->ItemAt(sel),BStringItem);
	
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Signatures");
	path.Append(item->Text());
	
	BFile file(path.Path(),B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	off_t size;
	file.GetSize(&size);
	char *buf = new char[size +1];
	
	size = file.Read(buf,size);
	buf[size] = '\0';
	fTextView->SetText(buf);
	fName->SetText(item->Text());
	delete[] buf;
	SetEnableControls(true);
}

/***********************************************************
 * Save
 ***********************************************************/
void
HSignatureView::SaveItem(int32 sel)
{
	if(sel<0)
		return;
	BStringItem *item = cast_as(fListView->ItemAt(sel),BStringItem);
	if(!item)
		return;
	BString name = fName->Text();
	
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	::create_directory(path.Path(),0777);
	path.Append("Signatures");
	
	path.Append(item->Text());
	
	if( name.Compare(item->Text()) != 0)
	{
		BEntry entry(path.Path());
		if(entry.Rename(name.String()) != B_OK)
		{
			(new BAlert("","Could not rename signature file","OK"))->Go();
			return;
		}
		entry.GetPath(&path);
		PRINT(("RENAMED\n"));
		
		item->SetText(name.String());
		fListView->InvalidateItem(sel);
	}
	
	BFile file(path.Path(),B_WRITE_ONLY);
	if(file.InitCheck() != B_OK)
	{
		(new BAlert("","Could not open signature file","OK"))->Go();
		return;
	}
	
	const char* kText = fTextView->Text();
	
	file.Write(kText,strlen(kText));
	file.SetSize(strlen(kText));	
	PRINT(("SAVE SIGNATURE\n"));
}

/***********************************************************
 * SetEnableControls
 ***********************************************************/
void
HSignatureView::SetEnableControls(bool enable)
{
	fTextView->MakeEditable(enable);
	if(!enable)
		fTextView->SetText("");
	fName->SetEnabled(enable);
	if(!enable)
		fName->SetText("");
	BButton *button;
	button = cast_as(FindView("apply"),BButton);
	button->SetEnabled(enable);
}
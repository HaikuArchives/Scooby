#include "HAccountView.h"
#include "HApp.h"
#include "TrackerUtils.h"
#include "Utilities.h"
#include "NumberControl.h"

#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <Button.h>
#include <Debug.h>
#include <String.h>
#include <ClassInfo.h>
#include <File.h>
#include <Alert.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <Box.h>
#include <ScrollView.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <StringView.h>
#include <MenuField.h>

const char* kLabel[] = {"Account Name:","E-Mail:","Host:","Port:","Login:",
							"Password:","SMTP host:","Real name:","Reply to:"};

/***********************************************************
 * Constructor
 ***********************************************************/
HAccountView::HAccountView(BRect rect)
	:BView(rect,"account",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	InitGUI();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HAccountView::~HAccountView()
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
 * Destructor
 ***********************************************************/
void
HAccountView::InitGUI()
{
	BRect rect(Bounds());
	rect.top += 10;
	rect.left +=5;
	rect.bottom-= 65;
	rect.right = rect.left +100;
	BRect frame;
	
	fListView = new BListView(rect,"list");
	fListView->SetFont(be_bold_font);
	fListView->SetSelectionMessage(new BMessage(M_CHANGE_ACCOUNT));
	BScrollView *scroll = new BScrollView("scrollview",fListView,
						B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT,
						0,false,true);
	AddChild(scroll);
	frame= rect;
	frame.top = frame.bottom + 5;
	frame.right= frame.left + 55;
	BButton *button;
	button = new BButton(frame,"del",_("Delete"),new BMessage(M_DEL_ACCOUNT));
	button->SetEnabled(false);
	AddChild(button);
	frame.OffsetBy(60,0);
	button = new BButton(frame,"add",_("Add"),new BMessage(M_ADD_ACCOUNT));
	AddChild(button);
	
	int32 i = 0;
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	BDirectory dir(path.Path());
	BEntry entry;
	status_t err = B_OK;
	
	while(err == B_OK)
	{
		if((err = dir.GetNextEntry(&entry)) == B_OK 
			&& !entry.IsDirectory())
		{
			char name[B_FILE_NAME_LENGTH+1];
			entry.GetName(name);
			fListView->AddItem(new BStringItem(name));
			i++;
		}
	}
	
	rect.OffsetBy(rect.Width()+30,0);
	frame = rect;
	frame.OffsetBy(250,0);
	frame.right = frame.left + 60;
	
	rect.right= Bounds().right -220;
	//rect.OffsetBy(0,40);
	rect.bottom = Bounds().bottom - 65;
	BBox *box = new BBox(rect,"Account Info");
	box->SetLabel(_("Account Info"));
	const float kDivider = StringWidth(_("Account Name:"))+5;

	BTextControl *ctrl;
	frame = box->Bounds();
	frame.InsetBy(10,20);
	frame.top -= 5;
	frame.bottom = frame.top +25;
	for(int32 i = 0;i < 9;i++)
	{
		ctrl = new BTextControl(frame,_(kLabel[i]),_(kLabel[i]),"",NULL);
		ctrl->SetDivider(kDivider);
		box->AddChild(ctrl);
		if(i == 0)
		{
			DisallowFilenameKeys(ctrl->TextView());
			DisallowMetaKeys(ctrl->TextView());
		}
		if(i == 5)
			ctrl->TextView()->HideTyping(true);
		frame.OffsetBy(0,23);
	}
	
	BMenu *menu = new BMenu("protocol");
	menu->AddItem(new BMenuItem("POP3",NULL));
	menu->AddItem(new BMenuItem("APOP",NULL));
	menu->AddItem(new BMenuItem("IMAP4",NULL));
	
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	menu->ItemAt(0)->SetMarked(true);
	BMenuField *menuField = new BMenuField(frame,"protocol",_("Protocol Type:")
												,menu);
	menuField->SetDivider(kDivider);
	box->AddChild(menuField);
	
	frame.OffsetBy(0,23);
	menu = new BMenu("signature");
	menu->AddItem(new BMenuItem(_("<none>"),NULL));
	// Scan all signatures
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Signatures");
	dir.SetTo(path.Path());
	char name[B_FILE_NAME_LENGTH];
	while(dir.GetNextEntry(&entry) == B_OK)
	{
		if(entry.IsDirectory())
			continue;
		entry.GetName(name);
		menu->AddItem(new BMenuItem(name,NULL));
	}
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	menu->ItemAt(0)->SetMarked(true);
	menuField = new BMenuField(frame,"signature",_("Signature:")
												,menu);
	menuField->SetDivider(kDivider);
	box->AddChild(menuField);
	
	AddChild(box);
	// POP3 Options
	rect.OffsetBy(rect.Width()+3,0);
	rect.right = Bounds().right - 5;
	rect.bottom = rect.top + rect.Height()/2;
	box= new BBox(rect,"retrieving_option");
	box->SetLabel(_("POP3 Options"));
	frame = box->Bounds();
	frame.InsetBy(10,20);
	frame.bottom = frame.top +25;
	

	BRadioButton *radio = new BRadioButton(frame,"leave",_("Leave mails on the server"),NULL);
	box->AddChild(radio);
	radio->SetValue(B_CONTROL_ON);
	frame.OffsetBy(0,20);
	radio = new BRadioButton(frame,"delete",_("Delete mails from server"),NULL);
	box->AddChild(radio);
	frame.OffsetBy(0,20);
	frame.right = frame.left + StringWidth(_("Delete mails after")) + 20;
	radio = new BRadioButton(frame,"delete_after",_("Delete mails after"),NULL);
	box->AddChild(radio);
	frame.OffsetBy(frame.Width(),0);
	frame.right = frame.left + 25;
	NumberControl *num_ctrl = new NumberControl(frame,"day","",5,NULL);
	num_ctrl->SetDivider(0);
	box->AddChild(num_ctrl);
	frame.OffsetBy(frame.Width()+3,-8);
	frame.right+=5;
	BStringView *string_view = new BStringView(frame,"days",_("days"));
	box->AddChild(string_view);
	AddChild(box);
	// SMTP options
	rect.OffsetBy(0,rect.Height());
	rect.top += 3;
	box= new BBox(rect,"smtp_option");
	box->SetLabel(_("SMTP Options"));
	
	frame = box->Bounds();
	frame.InsetBy(10,20);
	frame.bottom = frame.top + 25;
	BCheckBox *checkbox = new BCheckBox(frame,"smtp_auth",_("Use SMTP authentication"),NULL);
	box->AddChild(checkbox);
	
	AddChild(box);
	// Apply change button
	rect.bottom = Bounds().bottom - 30;
	rect.right = Bounds().right - 10;
	rect.left = rect.right - 80;
	rect.top = rect.bottom - 30;
	float width = rect.Width();
	button = new BButton(rect,"apply",_("Apply Changes"),new BMessage(M_ACCOUNT_SAVE_CHANGED));
	button->ResizeToPreferred();
	if(width < button->Bounds().Width() )
		button->MoveBy(width-button->Bounds().Width(),0 );
	AddChild(button);
	
	SetEnableControls(false);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HAccountView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_ADD_ACCOUNT:
		New();
		break;
	case M_DEL_ACCOUNT:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel>=0)
		{
			fListView->DeselectAll();
			BStringItem *item = cast_as(fListView->RemoveItem(sel),BStringItem);
			BPath path;
			::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
			path.Append(APP_NAME);
			path.Append("Accounts");
			path.Append(item->Text());
			entry_ref ref;
			::get_ref_for_path(path.Path(),&ref);
			TrackerUtils().MoveToTrash(ref);
			BButton *button = cast_as(FindView("del"),BButton);
			button->SetEnabled(false);
			delete item;
		}
		break;
	}
	case M_CHANGE_ACCOUNT:
	{
		int32 sel = fListView->CurrentSelection();
		OpenAccount(sel);
		break;
	}
	case M_ACCOUNT_SAVE_CHANGED:
	{
		int32 sel = fListView->CurrentSelection();
		if(sel<0)
			break;
		SaveAccount(sel);
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}


/***********************************************************
 * OpenAccount
 ***********************************************************/
void
HAccountView::OpenAccount(int32 index)
{
	if(index<0)
	{
err:
		for(int32 i = 0;i < 9;i++)
		{
			BTextControl *ctrl = cast_as(FindView(_(kLabel[i])),BTextControl);
			ctrl->SetText("");
		}
		BRadioButton *radio = cast_as(FindView("leave"),BRadioButton);
		radio->SetValue(B_CONTROL_ON);
		
		BButton *button = cast_as(FindView("del"),BButton);
		button->SetEnabled(false);
		SetEnableControls(false);
		return;
	}
	PRINT(("OPEN ACCOUNT\n"));
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	::create_directory(path.Path(),0777);
	BStringItem *item = cast_as(fListView->ItemAt(index),BStringItem);
	path.Append(item->Text());
	
	BFile file(path.Path(),B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		goto err;
	
	BMessage msg;
	msg.Unflatten(&file);
	//
	const char* kName[] = {"name","address","pop_host","pop_port","pop_user",
						"pop_password","smtp_host","real_name","reply_to"};
	
	for(int32 i = 0;i < 9;i++)
	{
		const char* str;
		msg.FindString(kName[i],&str);
		BTextControl *ctrl = cast_as(FindView(_(kLabel[i])),BTextControl);
		if(i == 5){		
			int32 len = strlen(str);
			char *pass = new char[len+1];
			for(int32 k = 0;k < len;k++)
				pass[k] = (char)(255-str[k]);
			pass[len] = '\0';
			ctrl->SetText(pass);
		}else{
			ctrl->SetText(str);
		}
	}
	
	int16 retrieve;
	msg.FindInt16("retrieve",&retrieve);
	PRINT(("retrieve:%d\n",retrieve));
	BRadioButton *radio = cast_as(FindView("leave"),BRadioButton);
	radio->SetValue((retrieve==0)?true:false);
	
	radio = cast_as(FindView("delete"),BRadioButton);
	radio->SetValue((retrieve==1)?true:false);
	
	radio = cast_as(FindView("delete_after"),BRadioButton);
	radio->SetValue((retrieve==2)?true:false);
	
	NumberControl *num_ctrl = cast_as(FindView("day"),NumberControl);
	int32 day;
	msg.FindInt32("delete_day",&day);
	num_ctrl->SetValue(day);
	
	BButton *button = cast_as(FindView("del"),BButton);
	button->SetEnabled(true);
	
	BMenuField *field = cast_as(FindView("protocol"),BMenuField);
	int16 protocol;
	msg.FindInt16("protocol_type",&protocol);
	field->Menu()->ItemAt(protocol)->SetMarked(true);
	
	bool smtp_auth;
	BCheckBox *checkbox = cast_as(FindView("smtp_auth"),BCheckBox);
	if(msg.FindBool("smtp_auth",&smtp_auth) == B_OK)
		checkbox->SetValue(smtp_auth);
	else
		checkbox->SetValue(B_CONTROL_OFF);
	
	const char* kPath;
	msg.FindString("signature",&kPath);
	field = cast_as(FindView("signature"),BMenuField);
	BMenu *menu = field->Menu();
	
	if(!kPath || ::strlen(kPath) == 0)
		menu->ItemAt(0)->SetMarked(true);
	else{
		path.SetTo(kPath);
		BMenuItem *menuitem = menu->FindItem(path.Leaf());
		if(menuitem)
			menuitem->SetMarked(true);
		else
			menu->ItemAt(0)->SetMarked(true);
	}
	SetEnableControls(true);
}

/***********************************************************
 * New
 ***********************************************************/
void
HAccountView::New()
{
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	::create_directory(path.Path(),0777);
	
	BDirectory dir(path.Path());
	BFile file;
	entry_ref ref;
	TrackerUtils().SmartCreateFile(&file,&dir,_("New Account"),"",B_READ_WRITE,&ref);
	
	path.SetTo(&ref);	
	fListView->AddItem(new BStringItem(path.Leaf()));
	
	const char* kName[] = {"name","address","pop_host","pop_port","pop_user",
						"pop_password","smtp_host","real_name","reply_to"};
	BMessage msg(B_SIMPLE_DATA);
	for(int32 i = 0;i < 9;i++)
	{
		if(i == 0)
			msg.AddString(kName[i],path.Leaf());
		else if(i == 3)
			msg.AddString(kName[i],"110");
		else
			msg.AddString(kName[i],"");
	}
	msg.AddInt16("retrieve",0);
	msg.AddInt32("delete_day",5);
	msg.AddInt16("protocol_type",0);
	msg.AddBool("smtp_auth",false);
	msg.AddString("signature","");
	msg.Flatten(&file);
}

/***********************************************************
 * SaveAccount
 ***********************************************************/
void
HAccountView::SaveAccount(int32 index)
{
	if(index<0)
		return;
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	::create_directory(path.Path(),0777);
	BStringItem *item = cast_as(fListView->ItemAt(index),BStringItem);
	BString old_name = item->Text();
	
	
	BTextControl *ctrl = cast_as(FindView(_("Account Name:")),BTextControl);
	path.Append(old_name.String());
	
	if(old_name.Compare(ctrl->Text()) != 0)
	{
		BEntry entry(path.Path());
		if(entry.Rename(ctrl->Text()) != B_OK)
		{
			(new BAlert("",_("Could not create the account file"),_("OK")))->Go();
			PRINT(("LINE:%d\n",__LINE__));
			return;
		}
		entry.GetPath(&path);
		PRINT(("RENAMED\n"));
	}
	item->SetText(ctrl->Text());
	fListView->InvalidateItem(fListView->IndexOf(item));
	
	BFile file(path.Path(),B_READ_WRITE);
	PRINT(("%s\n",path.Path() ));
	if(file.InitCheck() != B_OK)
	{
		if(file.SetTo(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE) != B_OK)
		{
			(new BAlert("",_("Could not create the account file"),_("OK")))->Go();
			PRINT(("LINE:%d\n",__LINE__));
		}
		return;
	}
	const char* kName[] = {"name","address","pop_host","pop_port","pop_user",
						"pop_password","smtp_host","real_name","reply_to"};

	// rewrite all settings
	
	BMessage msg(B_SIMPLE_DATA);
	msg.Unflatten(&file);
	for(int32 i = 0;i < 9;i++)
	{
		ctrl = cast_as(FindView(_(kLabel[i])),BTextControl);
		if(i == 5)
		{
			const char* text = ctrl->Text();
			int32 len = strlen(text);
			char *pass = new char[len+1];
			for(int32 k = 0;k < len;k++)
				pass[k] = (char)(255-text[k]);
			pass[len] = '\0';
			msg.RemoveData(kName[i]);
			msg.AddString(kName[i],pass);
			delete[] pass;
		}else{
			msg.RemoveData(kName[i]);
			msg.AddString(kName[i],ctrl->Text());
		}
	}
	BRadioButton *radio;
	
	radio = cast_as(FindView("leave"),BRadioButton);

	msg.RemoveData("retrieve");
	if(radio->Value() )
		msg.AddInt16("retrieve",0);
	else{
		radio = cast_as(FindView("delete"),BRadioButton);
		if(radio->Value())
			msg.AddInt16("retrieve",1);
		else
			msg.AddInt16("retrieve",2);
	}
	NumberControl *num_ctrl = cast_as(FindView("day"),NumberControl);
	msg.RemoveData("delete_day");
	msg.AddBool("delete_day",num_ctrl->Value());
	
	BMenuField *field = cast_as(FindView("protocol"),BMenuField);
	
	BMenu *menu = field->Menu();
	int16 protocol = menu->IndexOf(menu->FindMarked());
	msg.RemoveData("protocol_type");
	msg.AddInt16("protocol_type",protocol);
	
	BCheckBox *checkbox = cast_as(FindView("smtp_auth"),BCheckBox);
	msg.RemoveData("smtp_auth");
	msg.AddBool("smtp_auth",checkbox->Value());
	
	field = cast_as(FindView("signature"),BMenuField);
	menu = field->Menu();
	if(menu->IndexOf(menu->FindMarked())==0)
		msg.AddString("signature","");
	else if(menu->FindMarked()){
		BPath path;
		::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
		path.Append(APP_NAME);
		path.Append("Signatures");
		path.Append(menu->FindMarked()->Label());
		msg.AddString("signature",path.Path());
	}
	
	file.Seek(0,SEEK_SET);
	ssize_t numBytes;
	msg.Flatten(&file,&numBytes);
	file.SetSize(numBytes);
	PRINT(("SAVE ACCOUNT\n"));
}

/***********************************************************
 * SetEnableControls
 ***********************************************************/
void
HAccountView::SetEnableControls(bool enable)
{
	BTextControl *ctrl;					
	for(int32 i = 0;i < 9;i++)
	{
		ctrl = cast_as(FindView(_(kLabel[i])),BTextControl);
		ctrl->SetEnabled(enable);
	}
	
	BRadioButton *radio = cast_as(FindView("delete"),BRadioButton);
	radio->SetEnabled(enable);
	radio = cast_as(FindView("delete_after"),BRadioButton);
	radio->SetEnabled(enable);
	radio = cast_as(FindView("leave"),BRadioButton);
	radio->SetEnabled(enable);
	ctrl = cast_as(FindView("day"),BTextControl);
	ctrl->SetEnabled(enable);
	
	BButton *button = cast_as(FindView("apply"),BButton);
	button->SetEnabled(enable);

	BMenuField *field = cast_as(FindView("protocol"),BMenuField);
	field->SetEnabled(enable);
	if(!enable)
		field->Menu()->ItemAt(0)->SetMarked(true);
		
	BStringView *stringView = cast_as(FindView("days"),BStringView);
	stringView->SetHighColor((enable)?
						tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_MAX_TINT)
						:tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_3_TINT));
	stringView->Invalidate();
	
	BCheckBox *checkbox = cast_as(FindView("smtp_auth"),BCheckBox);
	checkbox->SetEnabled(enable);
	
	field = cast_as(FindView("signature"),BMenuField);
	field->SetEnabled(enable);
	if(!enable)
		field->Menu()->ItemAt(0)->SetMarked(true);
}

/***********************************************************
 * AttachedToWindow
 ***********************************************************/
void
HAccountView::AttachedToWindow()
{
	BButton *button;
	button = cast_as(FindView("add"),BButton);
	button->SetTarget(this);
	button = cast_as(FindView("del"),BButton);
	button->SetTarget(this);
	button = cast_as(FindView("apply"),BButton);
	button->SetTarget(this);
	fListView->SetTarget(this);
}

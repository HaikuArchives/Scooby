#include "HAccountView.h"
#include "HApp.h"
#include "TrackerUtils.h"
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
	rect.bottom-= 55;
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
	frame.right= frame.left + 50;
	BButton *button;
	button = new BButton(frame,"del",_("Delete"),new BMessage(M_DEL_ACCOUNT));
	button->SetEnabled(false);
	AddChild(button);
	frame.OffsetBy(55,0);
	button = new BButton(frame,"add",_("Add"),new BMessage(M_ADD_ACCOUNT));
	AddChild(button);
	
	int32 i = 0;
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append(_("Accounts"));
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
	rect.bottom = Bounds().bottom - 55;
	BBox *box = new BBox(rect,"Account Info");
	box->SetLabel(_("Account Info"));
	const float kDivider = StringWidth(_("POP password:"))+5;
	const char* kLabel[] = {_("Account Name:"),_("POP host:"),_("POP port:"),_("POP user name:"),
							_("POP password:"),_("SMTP host:"),_("Real name:"),
							_("Reply to:")};
	BTextControl *ctrl;
	frame = box->Bounds();
	frame.InsetBy(10,20);
	frame.bottom = frame.top +25;
	for(int32 i = 0;i < 8;i++)
	{
		ctrl = new BTextControl(frame,kLabel[i],kLabel[i],"",NULL);
		ctrl->SetDivider(kDivider);
		box->AddChild(ctrl);
		
		if(i == 4)
			ctrl->TextView()->HideTyping(true);
		frame.OffsetBy(0,23);
	}
	
	BMenu *menu = new BMenu("protocol");
	menu->AddItem(new BMenuItem("POP3",NULL));
	menu->AddItem(new BMenuItem("APOP",NULL));
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	menu->ItemAt(0)->SetMarked(true);
	BMenuField *menuField = new BMenuField(frame,"protocol",_("Protocol Type:")
												,menu);
	menuField->SetDivider(kDivider);
	box->AddChild(menuField);
	AddChild(box);

	rect.OffsetBy(rect.Width()+3,0);
	rect.right = Bounds().right - 5;
	box= new BBox(rect,"retrieving_option");
	box->SetLabel(_("Retrieving Options"));
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
	BStringView *string_view = new BStringView(frame,NULL,_("days"));
	box->AddChild(string_view);
	AddChild(box);

	
	// Apply change button
	rect.bottom = Bounds().bottom - 30;
	rect.right = Bounds().right - 5;
	rect.left = rect.right - 80;
	rect.top = rect.bottom - 20;
	
	button = new BButton(rect,"apply",_("Apply Change"),new BMessage(M_ACCOUNT_SAVE_CHANGED));
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
	const char* kLabel[] = {_("Account Name:"),_("POP host:"),_("POP port:"),_("POP user name:"),
							_("POP password:"),_("SMTP host:"),_("Real name:"),
							_("Reply to:")};
	if(index<0)
	{
err:
		for(int32 i = 0;i < 8;i++)
		{
			BTextControl *ctrl = cast_as(FindView(kLabel[i]),BTextControl);
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
	
	
	const char* kName[] = {"name","pop_host","pop_port","pop_user",
						"pop_password","smtp_host","real_name","reply_to"};
	
	for(int32 i = 0;i < 8;i++)
	{
		BString str;
		msg.FindString(kName[i],&str);
		BTextControl *ctrl = cast_as(FindView(kLabel[i]),BTextControl);
		if(i == 4)
		{		
			const char* text = str.String();
			BString pass("");
			int32 len = strlen(text);
			for(int32 k = 0;k < len;k++)
				pass << (char)(255-text[k]);
			ctrl->SetText(pass.String());
		}else{
			msg.FindString(kName[i],&str);
			ctrl->SetText(str.String());
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
	
	const char* kDefaultName = "New Account";
	path.Append(kDefaultName);
	
	BFile file(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_FAIL_IF_EXISTS);
	status_t err = file.InitCheck();
	int32 i = 1;
	while(err != B_OK)
	{
		path.GetParent(&path);
		BString newName(kDefaultName);
		newName << i++;
		path.Append(newName.String());
		err = file.SetTo(path.Path(),B_WRITE_ONLY|B_CREATE_FILE|B_FAIL_IF_EXISTS);
	}
	
	fListView->AddItem(new BStringItem(path.Leaf()));
	
	const char* kName[] = {"name","pop_host","pop_port","pop_user",
						"pop_password","smtp_host","real_name","reply_to"};
	BMessage msg(B_SIMPLE_DATA);
	for(int32 i = 0;i < 8;i++)
	{
		if(i == 0)
			msg.AddString(kName[i],path.Leaf());
		else if(i == 2)
			msg.AddString(kName[i],"110");
		else
			msg.AddString(kName[i],"");
	}
	msg.AddInt16("retrieve",0);
	msg.AddInt32("delete_day",5);
	msg.AddInt16("protocol_type",0);
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
			(new BAlert("",_("Could not create account file"),_("OK")))->Go();
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
			(new BAlert("",_("Could not create account file"),_("OK")))->Go();
			PRINT(("LINE:%d\n",__LINE__));
		}
		return;
	}
	
	const char* kLabel[] = {_("Account Name:"),_("POP host:"),_("POP port:"),_("POP user name:"),
							_("POP password:"),_("SMTP host:"),_("Real name:"),
							_("Reply to:")};
	const char* kName[] = {"name","pop_host","pop_port","pop_user",
						"pop_password","smtp_host","real_name","reply_to"};

	// rewrite all settings
	
	BMessage msg(B_SIMPLE_DATA);
	msg.Unflatten(&file);
	for(int32 i = 0;i < 8;i++)
	{
		ctrl = cast_as(FindView(kLabel[i]),BTextControl);
		if(i == 4)
		{
			const char* text = ctrl->Text();
			BString pass("");
			int32 len = strlen(text);
			for(int32 k = 0;k < len;k++)
				pass << (char)(255-text[k]);
			msg.RemoveData(kName[i]);
			msg.AddString(kName[i],pass);
		}else{
			msg.RemoveData(kName[i]);
			msg.AddString(kName[i],ctrl->Text());
		}
	}
	BRadioButton *radio;
	
	radio = cast_as(FindView("leave"),BRadioButton);

	msg.RemoveData("retrieve");
	if(radio->Value())
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
	const char* kLabel[] = {_("Account Name:"),_("POP host:"),_("POP port:"),_("POP user name:"),
							_("POP password:"),_("SMTP host:"),_("Real name:"),
							_("Reply to:")};
	BTextControl *ctrl;					
	for(int32 i = 0;i < 8;i++)
	{
		ctrl = cast_as(FindView(kLabel[i]),BTextControl);
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
}

#include "HGeneralSettingView.h"
#include "HApp.h"
#include "HPrefs.h"
#include "MenuUtils.h"
#include "NumberControl.h"
#include "HPrefWindow.h"
#include "Encoding.h"

#include <ClassInfo.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <UTF8.h>
#include <Box.h>
#include <Debug.h>
#include <CheckBox.h>
#include <StringView.h>
#include <stdlib.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HGeneralSettingView::HGeneralSettingView(BRect rect)
	:BView(rect,"signature",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	InitGUI();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HGeneralSettingView::~HGeneralSettingView()
{
	Save();
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HGeneralSettingView::InitGUI()
{
	HPrefs *prefs = ((HApp*)be_app)->Prefs();
	// Encoding menu
	BMenu *menu = new BMenu("encoding");
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	BMenuItem *item(NULL);
	
	Encoding encode;
	int32 numCharset = encode.CountCharset();
	for(int32 i = 0;i < numCharset;i++)
		menu->AddItem(new BMenuItem(encode.Charset(i),new BMessage(encode.Conversion(i))));
	
	int32 encoding;
	((HApp*)be_app)->Prefs()->GetData("encoding",&encoding);
	
	item = menu->FindItem(encoding);
	if(item)
		item->SetMarked(true);
	BRect rect(Bounds());
	rect.InsetBy(10,0);
	rect.OffsetBy(0,10);
	rect.bottom = rect.top + 40;
	BBox *box = new BBox(rect,"messagebox");
	box->SetLabel(_("Message"));
	BRect frame(rect);
	frame.OffsetTo(B_ORIGIN);
	frame.OffsetBy(10,10);
	frame.InsetBy(5,5);
	BMenuField *field = new BMenuField(frame,
									"encode",
									_("Default encoding:"),
									menu);
	field->SetDivider(StringWidth(_("Default encoding:"))+5);
	
	box->AddChild(field);
	AddChild(box);
	// Font Setting
	rect.OffsetBy(0,rect.Height()+5);
	rect.bottom = rect.top + 75;
	
	box = new BBox(rect,"fontbox");
	box->SetLabel(_("Font"));
	menu = new BMenu("fontmenu");
	MenuUtils().AddFontMenu(menu,M_FONT_CHANGED,this,Window(),true);
	// load font settings
	const char* family,*style;
	prefs->GetData("font_family",&family);
	prefs->GetData("font_style",&style);

	item = menu->FindItem(family);
	if(item)
	{
		item->SetMarked(true);
		int32 index = menu->IndexOf(item);
		item = (menu->SubmenuAt(index))->FindItem(style);
		if(item != NULL)
			item->SetMarked(true);
	}
	frame = rect;
	frame.OffsetTo(B_ORIGIN);
	frame.OffsetBy(10,10);
	frame.InsetBy(5,5);
	field = new BMenuField(frame,
								"font",
								_("Font:"),
								menu);
	field->SetDivider(StringWidth(_("Font Size:"))+5);
	box->AddChild(field);
	BString label("");
	label << family << "  " << style;
	menu->Superitem()->SetLabel( label.String() ); 
	
	menu = new BMenu("sizemenu");
	menu->AddItem(new BMenuItem("10",NULL));
	menu->AddItem(new BMenuItem("12",NULL));
	menu->AddItem(new BMenuItem("14",NULL));
	menu->AddItem(new BMenuItem("18",NULL));
	menu->AddItem(new BMenuItem("24",NULL));
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	// get font size setting
	float size;
	prefs->GetData("font_size",&size);
	label = "";
	label << (int32)size;
	
	item = menu->FindItem(label.String());
	if(item)
		item->SetMarked(true);
	
	frame.OffsetBy(0,25);
	field = new BMenuField(frame,
								"size",
								_("Font Size:"),
								menu);
	field->SetDivider(StringWidth(_("Font Size:"))+5);
	box->AddChild(field);
	
	AddChild(box);
	
	// Option
	rect.OffsetBy(0,rect.Height()+5);
	rect.bottom = Bounds().bottom - 30;
	
	box = new BBox(rect,"optionbox");
	box->SetLabel(_("Options"));
	frame = rect;
	frame.OffsetTo(B_ORIGIN);
	frame.OffsetBy(10,10);
	frame.InsetBy(5,5);
	frame.right = frame.left + StringWidth(_("Check mail every")) + 25;
	frame.bottom = frame.top + 17;
	
	BCheckBox *checkbox = new BCheckBox(frame,"check",_("Check mail every"),NULL);
	bool bValue;
	prefs->GetData("auto_check",&bValue);
	checkbox->SetValue(bValue);
	box->AddChild(checkbox);
	
	BRect frame2(frame);
	frame2.OffsetBy(frame2.Width(),0);
	frame2.right = frame2.left + 50;
	int32 minutes;
	prefs->GetData("check_minutes",&minutes);
	NumberControl *numberCtrl = new NumberControl(frame2,"minutes","",minutes,NULL);
	numberCtrl->SetDivider(0);
	numberCtrl->SetAlignment(B_ALIGN_LEFT,B_ALIGN_RIGHT);
	box->AddChild(numberCtrl);
	frame2.OffsetBy(frame2.Width()+5,0);
	BStringView *stringView = new BStringView(frame2,"",_("minutes"));
	
	frame.OffsetBy(0,20);
	frame.right = box->Bounds().right-10;
	checkbox = new BCheckBox(frame,"cache",_("Use folder cache"),NULL);
	prefs->GetData("use_folder_cache",&bValue);
	checkbox->SetValue(bValue);
	box->AddChild(checkbox);
	
	frame.OffsetBy(0,22);
	checkbox = new BCheckBox(frame,"list_start",
				_("Create all mail lists on start up"),NULL);
	prefs->GetData("load_list_on_start_up",&bValue);
	checkbox->SetValue(bValue);
	box->AddChild(checkbox);
	
	frame.OffsetBy(0,22);
	checkbox = new BCheckBox(frame,"desktray",
				_("Use deskbar replicant"),NULL);
	prefs->GetData("use_desktray",&bValue);
	checkbox->SetValue(bValue);
	box->AddChild(checkbox);
	
	frame.OffsetBy(0,22);
	checkbox = new BCheckBox(frame,"html",
				_("Use HTML view"),NULL);
	prefs->GetData("use_html",&bValue);
	checkbox->SetValue(bValue);
	box->AddChild(checkbox);
	
	frame.OffsetBy(0,22);
	checkbox = new BCheckBox(frame,"check_inbox",
				_("Open inbox when Scooby started up"),NULL);
	prefs->GetData("check_inbox",&bValue);
	checkbox->SetValue(bValue);
	box->AddChild(checkbox);
	
	frame.OffsetBy(0,22);
	menu = new BMenu("toolbar_mode");
	menu->AddItem(new BMenuItem(_("Text and Icon"),NULL));
	menu->AddItem(new BMenuItem(_("Icon only"),NULL));
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	int16 mode;
	prefs->GetData("toolbar_mode",&mode);
	item = menu->ItemAt(mode);
	if(item)
		item->SetMarked(true);
	field = new BMenuField(frame,"toolbar",_("Toolbar mode:"),menu);
	field->SetDivider(StringWidth(_("Toolbar mode:")) + 3);
	box->AddChild(field);
	
	box->AddChild(stringView);
	AddChild(box);
}


/***********************************************************
 * Save
 ***********************************************************/
void
HGeneralSettingView::Save()
{
	HPrefs *prefs = ((HApp*)be_app)->Prefs();
	
	// encoding
	BMenuField *field = cast_as(FindView("encode"),BMenuField);
	BMenu *menu = field->Menu();
	BMenuItem *item = menu->FindMarked();
	if(item)
	{
		BMessage *message = item->Message();
		int32 encoding = message->what;
		prefs->SetData("encoding",encoding);
	}
	
	// font
	BMenuField *menufield = cast_as(FindView("font"),BMenuField);
	menu = menufield->Menu();
	item = menu->FindMarked();
	if(item)
	{
		prefs->SetData("font_family",item->Label());
		item->SetMarked(true);
		int32 index = menu->IndexOf(item);
		item = (menu->SubmenuAt(index))->FindMarked();
		if(item)
			prefs->SetData("font_style",item->Label());
	}
	// font size
	menufield = cast_as(FindView("size"),BMenuField);
	menu = menufield->Menu();
	item = menu->FindMarked();
	if(item)
		prefs->SetData("font_size",(float)atof(item->Label()));
	// Schedule
	BCheckBox *checkBox = cast_as(FindView("check"),BCheckBox);
	prefs->SetData("auto_check",(bool)checkBox->Value());
	NumberControl *numCtrl = cast_as(FindView("minutes"),NumberControl);
	prefs->SetData("check_minutes",numCtrl->Value());
	checkBox = cast_as(FindView("list_start"),BCheckBox);
	prefs->SetData("load_list_on_start_up",(bool)checkBox->Value());
	checkBox = cast_as(FindView("cache"),BCheckBox);
	prefs->SetData("use_folder_cache",(bool)checkBox->Value());
	checkBox = cast_as(FindView("desktray"),BCheckBox);
	prefs->SetData("use_desktray",(bool)checkBox->Value());
	checkBox = cast_as(FindView("html"),BCheckBox);
	prefs->SetData("use_html",(bool)checkBox->Value());
	checkBox = cast_as(FindView("check_inbox"),BCheckBox);
	prefs->SetData("check_inbox",(bool)checkBox->Value());
	
	menufield = cast_as(FindView("toolbar"),BMenuField);
	menu = menufield->Menu();
	item = menu->FindMarked();
	if(item)
		prefs->SetData("toolbar_mode",(int16)menu->IndexOf(item));
	prefs->StorePrefs();
	((HApp*)be_app)->MainWindow()->PostMessage(M_PREFS_CHANGED);
}


/***********************************************************
 * MesasgeReceived
 ***********************************************************/
void
HGeneralSettingView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case 'FSel':
	{
		const char* family = message->FindString("family");
		const char* style = message->FindString("style");
		
		BString title = family;
		title << "  ";
		title << style;
		BMenuField *menufield = cast_as(FindView("font"),BMenuField);
		
		BMenu *menu = menufield->Menu();
		menu->Superitem()->SetLabel( title.String() ); 
		
		MenuUtils utils;
		utils.DeMarkAll(menu);
		BMenuItem *item = menu->FindItem(family);
		if(item)
		{
			item->SetMarked(true);
			int32 index = menu->IndexOf(item);
			item = (menu->SubmenuAt(index))->FindItem(style);
			if(item != NULL)
				item->SetMarked(true);
		}
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}
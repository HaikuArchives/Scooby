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

const char *kTimeFormat[] = {"%a, %d %b %Y %r",
							"%a, %d %b %y %T",
							"%m/%d/%Y %r",
							"%d/%m/%Y %r",
							"%m/%d/%y %T",
							"%d/%m/%y %T",
							"%Y/%m/%d %r",
							"%Y/%d/%m %r",
							"%y/%m/%d %T",
							"%y/%d/%m %T",
							"%m/%d %d",
							"%d/%m %d"
							};
const char* kFormatedTime[] = {"Mon, 15 Feb 2001 04:33:10 PM",
								"Mon, 15 Feb 01 16:33:10",
								"02/15/2001 04:33:10 PM",
								"15/02/2001 04:33:10 PM",
								"02/15/01 16:33:10",
								"15/02/01 16:33:10",
								"2001/02/15 04:33:10 PM",
								"2001/15/02 04:33:10 PM",
								"01/02/15 16:33:10",
								"01/15/02 16:33:10",
								"02/15 16:33:10",
								"15/02 16:33:10"
								};

const char* kLabels[] = {"Cache mail folders",
							"Create folder caches on startup (in background)",
							"Open inbox on startup",
							"Support mail sub-folders",
							"Use Deskbar replicant",
							"Always use HTML view",
							"Always open links in new window (HTML view)",
							"Use LED blinking"};

const char* kNames[] = {"use_folder_cache",
							"load_list_on_start_up",
							"check_inbox",
							"tree_mode",
							"use_desktray",
							"use_html",
							"open_link_as_new_window",
							"led_blink"};

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
	field->SetDivider(StringWidth(_("Default encoding:"))+8);
	
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
	field->SetDivider(StringWidth(_("Font Size:"))+8);
	box->AddChild(field);
	BString label("");
	label << family << "  " << style;
	menu->Superitem()->SetLabel( label.String() ); 
	
	menu = new BMenu("sizemenu");
	menu->AddItem(new BMenuItem(" 9",NULL));
	menu->AddItem(new BMenuItem("10",NULL));
	menu->AddItem(new BMenuItem("12",NULL));
	menu->AddItem(new BMenuItem("13",NULL));
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
	field->SetDivider(StringWidth(_("Font Size:"))+8);
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
	frame.right = frame.left + StringWidth(_("Check for new mail every")) + 20;
	frame.bottom = frame.top + 17;
	
	BCheckBox *checkbox = new BCheckBox(frame,"check",_("Check for new mail every"),NULL);
	bool bValue;
	prefs->GetData("auto_check",&bValue);
	checkbox->SetValue(bValue);
	box->AddChild(checkbox);
	
	BRect frame2(frame);
	frame2.OffsetBy(frame2.Width(),0);
	frame2.right = frame2.left + 25;
	int32 minutes;
	prefs->GetData("check_minutes",&minutes);
	NumberControl *numberCtrl = new NumberControl(frame2,"minutes","",minutes,NULL);
	numberCtrl->SetDivider(0);
	numberCtrl->SetAlignment(B_ALIGN_LEFT,B_ALIGN_RIGHT);
	box->AddChild(numberCtrl);
	frame2.OffsetBy(frame2.Width()+5,0);
	BStringView *stringView = new BStringView(frame2,"",_("mins"));
	
	const float x_offset = 300;
	frame.OffsetBy(x_offset,0);		
	frame.right = box->Bounds().right-10;
	
	int32 count = sizeof(kLabels)/sizeof(kLabels[0]);
	for(int32 i = 0;i < count;i++)
	{
		checkbox = new BCheckBox(frame,kNames[i],_(kLabels[i]),NULL);
		prefs->GetData(kNames[i],&bValue);
		checkbox->SetValue(bValue);
		box->AddChild(checkbox);
		if(i%2 != 0)
			frame.OffsetBy(x_offset,0);
		else
			frame.OffsetBy(-x_offset,22);
		
		if(i == count-1 && i%2 != 0)
				frame.OffsetBy(-x_offset,22);
	}
	
	count = sizeof(kTimeFormat)/sizeof(kTimeFormat[0]);
	menu = new BMenu("time_format");
	for(int32 i = 0;i < count;i++)
		menu->AddItem(new BMenuItem(kFormatedTime[i],NULL));
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	const char* timeformat;
	int32 index = 0;
	prefs->GetData("time_format",&timeformat);
	
	for(int32 i = 0;i < count;i++)
	{
		if(strcmp(timeformat,kTimeFormat[i]) == 0)
			index = i;
	}
	item = menu->FindItem(kFormatedTime[index]);
	if(item)
		item->SetMarked(true);
	field = new BMenuField(frame,"time_format",
				_("Time format:"),menu);
	field->SetDivider(StringWidth(_("Toolbar mode:")) + 8);
	
	box->AddChild(field);
	
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
	field->SetDivider(StringWidth(_("Toolbar mode:")) + 8);
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
	
	int32 count = sizeof(kLabels)/sizeof(kLabels[0]);
	for(int32 i = 0;i < count;i++)
	{
		checkBox = cast_as(FindView(kNames[i]),BCheckBox);
		prefs->SetData(kNames[i],(bool)checkBox->Value());
	}
	
	menufield = cast_as(FindView("toolbar"),BMenuField);
	menu = menufield->Menu();
	item = menu->FindMarked();
	if(item)
		prefs->SetData("toolbar_mode",(int16)menu->IndexOf(item));

	menufield = cast_as(FindView("time_format"),BMenuField);
	menu = menufield->Menu();
	item = menu->FindMarked();
	if(item)
		prefs->SetData("time_format",kTimeFormat[menu->IndexOf(item)]);
	
	
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

/***********************************************************
 * AttachedToWindow
 ***********************************************************/
void
HGeneralSettingView::AttachedToWindow()
{
}

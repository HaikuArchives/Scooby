#include "HCriteriaView.h"

#include <Menu.h>
#include <MenuItem.h>


/***********************************************************
 * Constructor
 ***********************************************************/
HCriteriaView::HCriteriaView(BRect rect,const char* name)
	:BView(rect,name,B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	//SetViewColor(200,0,0);
	InitGUI();
}

/***********************************************************
 * Destructor
 ***********************************************************/
HCriteriaView::~HCriteriaView()
{
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HCriteriaView::InitGUI()
{
	BRect rect = Bounds();
	rect.left += 5;
	rect.top += 2;
	rect.right = rect.left + 80;
	BMenu *menu = new BMenu("menu");
	menu->AddItem(new BMenuItem("Subject",NULL));
	menu->AddItem(new BMenuItem("To",NULL));
	menu->AddItem(new BMenuItem("From",NULL));
	menu->AddItem(new BMenuItem("Cc",NULL));
	menu->AddItem(new BMenuItem("Reply-To",NULL));
	menu->ItemAt(0)->SetMarked(true);
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	fAttrMenu = new BMenuField(rect,"attr","",menu);
	fAttrMenu->SetDivider(0);
	AddChild(fAttrMenu);
	
	rect.OffsetBy(80,0);
	menu = new BMenu("menu");
	menu->AddItem(new BMenuItem("contain",NULL));
	menu->AddItem(new BMenuItem("is",NULL));
	menu->AddItem(new BMenuItem("begin with",NULL));
	menu->AddItem(new BMenuItem("end with",NULL));
	menu->ItemAt(0)->SetMarked(true);
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	fOpMenu = new BMenuField(rect,"op","",menu);
	fOpMenu->SetDivider(0);
	AddChild(fOpMenu);
	
	rect.OffsetBy(80,2);
	rect.right= Bounds().right - 10;
	fValue = new BTextControl(rect,"value","","",NULL);
	fValue->SetDivider(0);
	AddChild(fValue);
}

/***********************************************************
 * SetEnableControls
 ***********************************************************/
void
HCriteriaView::SetEnableControls(bool enable)
{
	fAttrMenu->SetEnabled(enable);
	BMenu *menu = fAttrMenu->Menu();
	if(menu)
		menu->ItemAt(0)->SetMarked(true);
	
	fOpMenu->SetEnabled(enable);
	menu = fOpMenu->Menu();
	if(menu)
		menu->ItemAt(0)->SetMarked(true);
	fValue->SetEnabled(enable);
	fValue->SetText("");
}

/***********************************************************
 * SetValue
 ***********************************************************/
void
HCriteriaView::SetValue(int32 attr,
						int32 operation,
						const char* attr_value)
{
	BMenu *menu = fAttrMenu->Menu();
	BMenuItem *item;
	if(menu)
	{
		item = menu->ItemAt(attr);
		if(item) item->SetMarked(true);
	}
	menu = fOpMenu->Menu();
	if(menu)
	{
		item = menu->ItemAt(operation);
		if(item) item->SetMarked(true);
	}
	fValue->SetText(attr_value);
}

/***********************************************************
 * Draw
 ***********************************************************/
void
HCriteriaView::Draw(BRect updateRect)
{
	BRect rect = Bounds();
	if(!IsFocus())
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_3_TINT));
	else
	{
		rect.InsetBy(0,1);
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	}
	StrokeRect(rect);
}

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HCriteriaView::MouseDown(BPoint point)
{
	MakeFocus(true);
	BView::MouseDown(point);
}

/***********************************************************
 * MakeFocus
 ***********************************************************/
void
HCriteriaView::MakeFocus(bool focus)
{
	Invalidate();
	BView::MakeFocus(focus);
}

/***********************************************************
 * Attribute
 ***********************************************************/
int32
HCriteriaView::Attribute() const
{
	BMenu *menu = fAttrMenu->Menu();
	BMenuItem *item = menu->FindMarked();
	if(item)
		return menu->IndexOf(item);
	return 0;	
}

/***********************************************************
 * Operator
 ***********************************************************/
int32
HCriteriaView::Operator() const
{
	BMenu *menu = fOpMenu->Menu();
	BMenuItem *item = menu->FindMarked();
	if(item)
		return menu->IndexOf(item);
	return 0;	
}
#include "StatusBar.h"
#include <View.h>

/***********************************************************
 * Constructor
 ***********************************************************/
StatusBar::StatusBar(BRect rect,const char* name,uint32 resize,uint32 flags)
	:BView(rect,name,resize,flags)
{
	fItemList.MakeEmpty();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

/***********************************************************
 * AddItem
 ***********************************************************/
void
StatusBar::AddItem(const char* name,const char* initialText,void (*pulseFunc)(StatusItem* item))
{
	BRect rect(Bounds());
	rect.InsetBy(0,2);
	rect.OffsetBy(0,1);
	rect.right = rect.left + 100;
	float width = 0;
	int32 count = fItemList.CountItems();
	if(count != 0)	
		width += ((StatusItem*)fItemList.ItemAt(count-1))->Frame().right+2;
	rect.OffsetBy(width,0);
	StatusItem *item = new StatusItem(rect,name,initialText,pulseFunc);
	fItemList.AddItem(item);
	AddChild(item);
	item->ResizeToPreferred();
}

/***********************************************************
 * UpdateItemWidth
 ***********************************************************/
void
StatusBar::RearrangeItems(StatusItem* startItem,float delta)
{
	int32 count = fItemList.CountItems();
	StatusItem *item(NULL);	
	int32 startpos = fItemList.IndexOf(startItem);
	for(int32 i = startpos+1;i < count;i++)
	{
		item = (StatusItem*)fItemList.ItemAt(i);
		item->MoveBy(delta,0);
	}
}

#include "HAttachmentList.h"
#include "CLVColumn.h"
#include "HAttachmentItem.h"
#include "HHtmlMailView.h"

#include <PopUpMenu.h>
#include <Menu.h>
#include <Window.h>
#include <MenuItem.h>
#include <string.h>
#include <ClassInfo.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HAttachmentList::HAttachmentList(BRect frame,
								BetterScrollView **scroll,
								const char* title)
		:ColumnListView(frame,
					(CLVContainerView**)scroll,
					title,
					B_FOLLOW_ALL,
					B_WILL_DRAW,
					B_SINGLE_SELECTION_LIST)
{
	AddColumn( new CLVColumn(NULL,22,CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE|
		CLV_NOT_RESIZABLE|CLV_PUSH_PASS|CLV_MERGE_WITH_RIGHT) );
	const char* kLabels[] = {"Name","Content-Type","Size"};
	CLVColumn*			column;
	for(int32 i = 0;i < 3;i++)
	{
		column = new CLVColumn(kLabels[i],180,CLV_SORT_KEYABLE|CLV_TELL_ITEMS_WIDTH);
		AddColumn(column);
	}
	SetInvocationMessage(new BMessage(M_OPEN_ATTACHMENT));
}

/***********************************************************
 * Destructor
 ***********************************************************/
HAttachmentList::~HAttachmentList()
{
	SetInvocationMessage(NULL);
}

/***********************************************************
 * FindPart
 ***********************************************************/
int32
HAttachmentList::FindPart(const char* type)
{
	int32 count = CountItems();
	
	HAttachmentItem **items = (HAttachmentItem**)Items();
	for(int32 i = 0;i < count;i++)
	{
		if(items[i] && ::strcmp(items[i]->ContentType(),type) == 0)
			return i;
	}
	return -1;
}

/***********************************************************
 * MoseDown
 ***********************************************************/
void
HAttachmentList::MouseDown(BPoint pos)
{
	uint32			buttons;
	
	if (Window()->CurrentMessage())
		Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
	if(buttons == B_SECONDARY_MOUSE_BUTTON)
	{
		int32 sel = CurrentSelection();
		
		BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
    	BFont font(be_plain_font);
    	font.SetSize(10);
    	theMenu->SetFont(&font);
    	BMenuItem *item = new BMenuItem("Save As…",new BMessage(M_SAVE_ATTACHMENT));
    	item->SetEnabled((sel < 0)?false:true);
    	theMenu->AddItem(item);
    	item = new BMenuItem("Open",new BMessage(M_OPEN_ATTACHMENT));
    	item->SetEnabled((sel < 0)?false:true);
    	theMenu->AddItem(item);
		BRect r;
        ConvertToScreen(&pos);
        r.top = pos.y - 5;
        r.bottom = pos.y + 5;
        r.left = pos.x - 5;
        r.right = pos.x + 5;
        
    	item = theMenu->Go(pos, false,true,r);  
    	if(item)
    	{
    	 	BMessage*	aMessage = item->Message();
			if(aMessage)
				this->Window()->PostMessage(aMessage);
	 	} 
	 	delete theMenu;
	}else
		ColumnListView::MouseDown(pos);
}

/***********************************************************
 * InitiateDrag
 ***********************************************************/
bool
HAttachmentList::InitiateDrag(BPoint  point,
						int32 index,
						bool wasSelected)
{
	if (wasSelected) 
	{
		BMessage msg(B_SIMPLE_DATA);
		HAttachmentItem *item = cast_as(ItemAt(index),HAttachmentItem);
		if(item == NULL)
			return false;
		BRect	theRect = this->ItemFrame(index);
		
		int32 selected; 
		int32 sel_index = 0;
		
		while((selected = CurrentSelection(sel_index++)) >= 0)
		{
			item=cast_as(ItemAt(selected),HAttachmentItem);
			if(!item)
				continue;
			msg.AddPointer("pointer",item);
		}
			
		const char *subject = item->GetColumnContentText(1);
		theRect.OffsetTo(B_ORIGIN);
		theRect.right = theRect.left + StringWidth(subject) + 20;
		BBitmap *bitmap = new BBitmap(theRect,B_RGBA32,true);
		BView *view = new BView(theRect,"",B_FOLLOW_NONE,0);
		bitmap->AddChild(view);
		bitmap->Lock();
		view->SetHighColor(0,0,0,0);
		view->FillRect(view->Bounds());
		
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetHighColor(0,0,0,128);
		view->SetBlendingMode(B_CONSTANT_ALPHA,B_ALPHA_COMPOSITE);
		const BBitmap *icon = item->GetColumnContentBitmap(0);
		if(icon)
			view->DrawBitmap(icon);
		
		BFont font;
		GetFont(&font);	
		view->SetFont(&font);
		view->MovePenTo(theRect.left+18, theRect.bottom-3);
		
		if(subject)
			view->DrawString( subject );
		bitmap->Unlock();
		
		DragMessage(&msg, bitmap, B_OP_ALPHA,
				BPoint(bitmap->Bounds().Width()/2,bitmap->Bounds().Height()/2));
		// must not delete bitmap
	}	
	return (wasSelected);
}

#include "HAttachmentList.h"
#include "CLVColumn.h"
#include "HAttachmentItem.h"

#include <PopUpMenu.h>
#include <Menu.h>
#include <Window.h>
#include <MenuItem.h>

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
#include "HEnclosureItem.h"
#include "ResourceUtils.h"
#include "Utilities.h"
#include "Encoding.h"

#include <View.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Node.h>
#include <stdio.h>
#include <stdlib.h>
#include <Debug.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HEnclosureItem::HEnclosureItem(entry_ref ref)
	:BListItem()
	,fRef(ref)
	,fFontHeight(-1)
	,fDataBuf(NULL)
	,fHaveRef(true)
{
	fName = BPath(&ref).Leaf();
	
	BNode node(&ref);
	
	off_t size;
	node.GetSize(&size);

	char *buf = new char[50];
	MakeSizeLabel((char**)&buf,size);
	
	fName += buf;
	delete[] buf;
	
	fBitmap = new BBitmap(BRect(0,0,15,15),B_CMAP8);
	if(BNodeInfo::GetTrackerIcon(&ref,fBitmap,B_MINI_ICON) != B_OK)
	{
		delete fBitmap;
		fBitmap = NULL;
	}

}

HEnclosureItem::HEnclosureItem(const char* data)
	:BListItem()
	,fFontHeight(-1)
	,fHaveRef(false)
{
	// make name,bitmap and calc size
	fBitmap = ResourceUtils().GetBitmapResource('BBMP',"Enclosure");
	
	char* param = NULL;
	GetParameter(data,"name=",&param);
	fName = param;
	delete[] param; param =NULL;
	Encoding().Mime2UTF8(fName);
	
	off_t size=0;
	char *p = ::strstr(data,"\r\n\r\n");
	if(p)
		size = ::strlen(p+4);
	
	char *buf = new char[50];
	MakeSizeLabel(&buf,size);
	fName += buf;
	delete[] buf;
	
	fDataBuf = ::strdup(data);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HEnclosureItem::~HEnclosureItem()
{
	delete fBitmap;
	free(fDataBuf);
}

/***********************************************************
 * DrawItem
 ***********************************************************/
void
HEnclosureItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	const rgb_color kSelectionColor = {184,194, 255,180};
	if (IsSelected() || complete)
	{
		rgb_color color;
		if (IsSelected())
			color = kSelectionColor;
		else
			color = owner->ViewColor();
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(frame);
	}
	
	if(fFontHeight <0)
	{
		BFont font;
		owner->GetFont(&font);
		font_height height;
		font.GetHeight(&height);
		fFontHeight = height.ascent+height.descent;
	}
	
	owner->SetHighColor(0,0,0);
	owner->MovePenTo(frame.left+22, frame.top + 2+ fFontHeight);
	owner->DrawString(fName.String());

	if(fBitmap)
	{
		drawing_mode mode = owner->DrawingMode();
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->MovePenTo(frame.left+2,frame.top+2);
		owner->DrawBitmap(fBitmap);
		owner->SetDrawingMode(mode);
	}
}

/***********************************************************
 * Update
 ***********************************************************/
void
HEnclosureItem::Update(BView* owner, const BFont* font)
{
	const float kMinHeight = 20;
	
	BListItem::Update(owner,font);
	float ItemHeight = Height();
	if(ItemHeight < kMinHeight)
		ItemHeight = kMinHeight;
	SetHeight(ItemHeight);
}

/***********************************************************
 * MakeSizeLabel
 ***********************************************************/
void
HEnclosureItem::MakeSizeLabel(char** buf,off_t size)
{
	char notation[6];
	char format[16];
	::strcpy(format," ( %6.2f %s ) ");
	float new_size = (float)size;
	if(size < 1024)
	{
		::strcpy(notation,"Bytes");
		::strcpy(format," ( %4.0f %s ) ");
	}else if(size >= 1024 && size < 1048576){	
		::strcpy(notation,"KB");
		new_size = new_size/1024.0;
	}else{
		::strcpy(notation,"MB");
		new_size = new_size/1048576.0;	
	}
	::sprintf(*buf,format,new_size,notation);
}
#include "HWrapTextView.h"
#include "HRulerView.h"

#include <Debug.h>

#define RULER_HEIGHT 16

/***********************************************************
 * Constructor
 ***********************************************************/
HWrapTextView::HWrapTextView(BRect rect,const char* name,int32 resize,int32 flag)
		:_inherited(rect,name,BRect(4.0,4.0,rect.right-rect.left-4.0,rect.bottom-rect.top-4.0),resize,flag)
		,fUseRuler(false)
		,fRightLimit(490.0F)
		,fRulerView(NULL)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
HWrapTextView::~HWrapTextView()
{

}

/***********************************************************
 * AddRuler
 ***********************************************************/
void
HWrapTextView::AddRuler()
{
	if(!fRulerView && Parent())
	{
		ResizeBy(0,-(RULER_HEIGHT+1));
		MoveBy(0,RULER_HEIGHT+1);
		
		BRect rect = Bounds();
		rect = Parent()->Bounds();
		rect.top += 2;
		rect.OffsetBy(2,0);
		rect.bottom = rect.top + RULER_HEIGHT;
		fRulerView = new HRulerView(rect,"ruler",this);
		Parent()->AddChild(fRulerView);
		ResetTextRect();
	}
}

/***********************************************************
 * RemoveRuler
 ***********************************************************/
void
HWrapTextView::RemoveRuler()
{
	if(fRulerView)
	{
		fRulerView->RemoveSelf();
		delete fRulerView;
		fRulerView=NULL;
		
		ResizeBy(0,RULER_HEIGHT+1);
		MoveBy(0,-(RULER_HEIGHT+1));
		
		ResetTextRect();
	}
}

/***********************************************************
 * UseRuler
 ***********************************************************/
void
HWrapTextView::UseRuler(bool use)
{
	if(use == fUseRuler)
		return;
	fUseRuler = use;
	if(fUseRuler)
		AddRuler();
	else
		RemoveRuler();
}

/***********************************************************
 * FrameResized
 ***********************************************************/
void
HWrapTextView::FrameResized(float width, float height)
{
	ResetTextRect();
	if(fUseRuler)
	{
		BRect rect= fRulerView->Bounds();
		fRulerView->ResizeTo(width,rect.Height());
		
		rect.left= rect.right - 50;
		rect.right = width;
		fRulerView->Invalidate(rect);
	}
	_inherited::FrameResized(width,height);
}

/***********************************************************
 * SetRightLimit
 ***********************************************************/
void
HWrapTextView::SetRightLimit(float limit)
{
	if(fRightLimit != limit)
	{
		fRightLimit = limit;
		ResetTextRect();
	}
}
/***********************************************************
 * ResetTextRect
 ***********************************************************/
void
HWrapTextView::ResetTextRect()
{
	BRect textRect = Bounds();
	textRect.left = 4.0;
	textRect.top = 4.0;
	textRect.right -= 4.0;
	textRect.bottom -= 4.0;
	if(fUseRuler)
		textRect.right = fRightLimit;
	SetTextRect(textRect);
}

/***********************************************************
 * HardWrap
 ***********************************************************/
void
HWrapTextView::GetHardWrapedText(BString &out)
{
	MakeEditable(false);
	
	BFont font;
	uint32 propa;
	GetFontAndColor(&font,&propa);
	out = "";
	
	BString line;
	int32 length = TextLength();
	float view_width = TextRect().Width();
	char c=0;
	bool inserted;
	
	for(int32 i=0;i < length;i++)
	{
		c = ByteAt(i);
		if(c == '\n')
		{
			line = "";
			continue;
		}
		line += c;
		if(font.StringWidth(line.String())>view_width)
		{
			// Back 1 charactor.
			i--;
			line.Truncate(line.Length()-1);
			// Insert line break.
			inserted = false;
			int32 len = line.Length();
			for(int32 k = 0;k<len;k++)
			{
				if(CanEndLine(i-k))
				{
					Insert(i-k+1,"\n",1);
					inserted=true;
					i = i-k+1;
					break;
				}
			}
			// If could not find proper position, add line break to end.
			if(!inserted)
				Insert(i,"\n",1);
			line = "";
		}
	}
	out = Text();
	//PRINT(("%s\n",out.String()));
	MakeEditable(true);
}

/***********************************************************
 * ScrollTo
 ***********************************************************/
void
HWrapTextView::ScrollTo(BPoint pos)
{
	if(fRulerView)
	{
		pos.PrintToStream();
		BPoint rulerPos=pos;
		rulerPos.y=0;
		fRulerView->ScrollTo(rulerPos);
		fRulerView->Invalidate();
	}
	_inherited::ScrollTo(pos);
}

/***********************************************************
 * SetFontAndColor
 ***********************************************************/
void
HWrapTextView::SetFontAndColor(const BFont		*inFont, 
								uint32			inMode,
								const rgb_color	*inColor)
{
	if(fRulerView)
		fRulerView->FontReseted();
	_inherited::SetFontAndColor(inFont,inMode,inColor);
}

/***********************************************************
 * SetFontAndColor
 ***********************************************************/
void
HWrapTextView::SetFontAndColor(int32			startOffset, 
								int32			endOffset, 
								const BFont		*inFont,
								uint32			inMode ,
								const rgb_color	*inColor)
{
	if(fRulerView)
		fRulerView->FontReseted();
	_inherited::SetFontAndColor(startOffset,endOffset,inFont,inMode,inColor);
}
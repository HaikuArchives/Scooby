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
	if(!fRulerView)
	{
		BRect rect = TextRect();
		rect.top += RULER_HEIGHT;
		rect = Bounds();
		rect.bottom = rect.top + RULER_HEIGHT;
		fRulerView = new HRulerView(rect,"ruler",this);
		AddChild(fRulerView);
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
	{
		textRect.top += RULER_HEIGHT;
		textRect.right = fRightLimit;
	}
	SetTextRect(textRect);
}

/***********************************************************
 * HardWrap
 ***********************************************************/
void
HWrapTextView::GetHardWrapedText(BString &out)
{
	BFont font;
	uint32 propa;
	GetFontAndColor(&font,&propa);
	const char* text = Text();
	out = "";
	
	float lineWidth = 0;
	BString c;
	int32 length = TextLength();
	float view_width = TextRect().Width();
	int32 linefeedCount = CountLines();
	int32 nextbytes = 0;
	int32 k = 0;
	int32 *insertPos = new int32[linefeedCount];
	
	// Check positions to insert linefeeds
	for(int32 i = 0;i < length;i++)
	{
		c = text[i];
		nextbytes = ByteLength(c[0])-1;
		
		for(int32 j = 1;j <= nextbytes;j++)
			c += text[i+j];
		i += nextbytes;
		lineWidth += font.StringWidth(c.String());
		if(view_width <= lineWidth || c[0] == '\n')
		{
			lineWidth = 0;
			if(c[0] != '\n')
			{
				lineWidth = font.StringWidth(c.String());
				int32 oldLen = i - c.Length();
				for(int32 n = 0;n <oldLen;n++)
				{
					if(CanEndLine(oldLen-n))
					{
						int32 skip = ByteLength(text[oldLen-n])-1;
						insertPos[k++] = oldLen-n+1+skip;
						int32 charLen = n - skip;
						char *tmp = new char[charLen+1];
						::strncpy(tmp,&text[oldLen-n+1+skip],charLen);
						tmp[charLen] = '\0';
						c.Insert(tmp,0);
						lineWidth = font.StringWidth(c.String());
						delete[] tmp;
						break;
					}
				}
			}
		}
	}
	
	// Insert linefeeds
	out = text;
	for(int32 i = 0;i < k;i++)
	{
		out.Insert('\n',1,insertPos[i]+i);
	}
	delete[] insertPos;
	//PRINT(("%s\n",out.String()));
}

/***********************************************************
 * ByteLength: Calculate UTF-8 charactor byte length
 ***********************************************************/
int32
HWrapTextView::ByteLength(char c)
{
	if( !(c & 0x80) )
		return 1;
	if((c & 0x20)&&(c & 0x40))
		return 3;
	return 2;
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
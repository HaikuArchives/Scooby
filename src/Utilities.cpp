#include "Utilities.h"
#include "scandir.h"
#include "StatusItem.h"
#include "HApp.h"

#include <String.h>
#include <Debug.h>
#include <File.h>
#include <stdio.h>
#include <dirent.h>
#include <Window.h>
#include <TextView.h>
#include <MenuItem.h>
#include <ClassInfo.h>

void
DisallowFilenameKeys(BTextView *textView)
{
	textView->DisallowChar(':');
	textView->DisallowChar('/');
}

void
DisallowMetaKeys(BTextView *textView)
{
	textView->DisallowChar(B_TAB);
	textView->DisallowChar(B_ESCAPE);
	textView->DisallowChar(B_INSERT);
	textView->DisallowChar(B_DELETE);
	textView->DisallowChar(B_HOME);
	textView->DisallowChar(B_END);
	textView->DisallowChar(B_PAGE_UP);
	textView->DisallowChar(B_PAGE_DOWN);
	textView->DisallowChar(B_FUNCTION_KEY);
}


int32 GetAllDirents(const char* path,struct dirent ***outdirent,bool folder_only)
{
	register int32 count;
	register struct dirent **dirent;
	
	count = scandir(path,&dirent,(folder_only)?folderselector:NULL,NULL);
	
	*outdirent = dirent;
	return count;
}


bool IsHeaderLine(const char* line)
{
	if(!line )
		return false;
	while (*line != '\0' && *line != ' ') {
		if (*line == ':')
			return true;
		line++;
	}
	return false;	
}

int32 ReadLine(BFile *file,BString* out)
{
	if(!file || !out)
	{
		PRINT(("NULL POINTER:%s %d",__FILE__,__LINE__));
		return -1;
	}
	char c=0;
	out->SetTo("");
	int32 len = 0;
	while(file->Read(&c,1) > 0)
	{
		out->Append(c,1);
		len++;
		if(c == '\n')
			break;
	}
	return len;
}


void EnableMenuItem(BMenuItem *item,bool enabled)
{
	if(!item)
		return;
	item->SetEnabled(enabled);
}

void MarkMenuItem(BMenuItem *item,bool marked)
{
	if(!item)
		return;
	item->SetMarked(marked);
}

void SetMenuItemLabel(BMenuItem *item,const char* label)
{
	if(!item)
		return;
	item->SetLabel(label);
}

/***********************************************************
 * LineUpdate
 ***********************************************************/
void LineUpdate(StatusItem *item)
{
	BWindow *window = item->Window();
	BTextView *view = cast_as(window->FindView("HMailView"),BTextView);
//	int32 lines = view->CountLines();
	int32 current = view->CurrentLine();
	int32 lineoffset = view->OffsetAt(current);
	int32 start,end;
	view->GetSelection(&start,&end);
			
	BString label;
	label << _("Row") << ": " << current+1 <<"  Col: " << start-lineoffset;
	item->SetLabel( label.String() );
	item->ResizeToPreferred();
}

/***********************************************************
 * SizeUpdate
 ***********************************************************/
void SizeUpdate(StatusItem *item)
{
	BWindow *window = item->Window();
	BTextView *view = cast_as(window->FindView("HMailView"),BTextView);
	int32 size = view->TextLength();
	BString label;
	label << _("Size") << ": " << size << " byte";
	if(size != 0)
		label += "s";
	item->SetLabel( label.String() );
	item->ResizeToPreferred();
}
#include "Utilities.h"
#include "scandir.h"

#include <String.h>
#include <Debug.h>
#include <File.h>
#include <stdio.h>
#include <dirent.h>

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


int32 GetAllDirents(const char* path,struct dirent ***outdirent)
{
	register int32 count;
	register struct dirent **dirent;
	
	count = scandir(path,&dirent,NULL,NULL);
	
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

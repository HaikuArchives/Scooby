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
#include <fs_attr.h>
#include <Node.h>
#include <stdlib.h>

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
	if(!item) return;
	BWindow *window = item->Window();
	if(!window) return;
	BTextView *view = cast_as(window->FindView("HMailView"),BTextView);
	if(!view) return;
//	int32 lines = view->CountLines();
	int32 current = view->CurrentLine();
	int32 lineoffset = view->OffsetAt(current);
	int32 start,end;
	view->GetSelection(&start,&end);
			
	BString label;
	label << _("Row") << ": " << current + 1 
			<< "  " << _("Col") << ": " << start - lineoffset;
	item->SetLabel( label.String() );
	item->ResizeToPreferred();
}

/***********************************************************
 * SizeUpdate
 ***********************************************************/
void SizeUpdate(StatusItem *item)
{
	if(!item) return;
	BWindow *window = item->Window();
	if(!window) return;
	BTextView *view = cast_as(window->FindView("HMailView"),BTextView);
	if(!view) return;
	int32 size = view->TextLength();
	BString label;
	label << _("Size") << ": " << size << " byte";
	if(size != 0)
		label += "s";
	item->SetLabel( label.String() );
	item->ResizeToPreferred();
}

/***********************************************************
 * Get mail header param (HTMLVIEW ONLY)
 ***********************************************************/
bool GetParameter(const char *src,const char *param, char **dst)
{
	char		*offset;
	int32		len;
	char 		*out;

	if ((offset = cistrstr((char*)src, (char*)param))) {
		offset += strlen(param);
		//len = strlen(src) - (offset - src);
		if (*offset == '"')
			offset++;
		if( *offset == ' ')
			offset++;
		len = 0;
		while (offset[len] != '"' && 
				offset[len] != ';' && 
				offset[len] != '\n'&& 
				offset[len] != '\r')
			len++;
		out = new char[len+1];
		if(!out)
		{
			(new BAlert("","Memory was exhausted","OK"))->Go();
			return false;
		}
		::strncpy(out, offset,len);
		out[len] = '\0';
		char *p = strchr(out,';');
		if(p)
			p[0] = '\0';
		*dst = out;
		return true;
	}
	return false;
}

/***********************************************************
 * case-insensitive version of strstr
 ***********************************************************/
char* cistrstr(char *cs, char *ct)
{
	char		c1;
	char		c2;
	int32		cs_len;
	int32		ct_len;
	int32		loop1;
	int32		loop2;

	cs_len = strlen(cs);
	ct_len = strlen(ct);
	for (loop1 = 0; loop1 < cs_len; loop1++) {
		if (cs_len - loop1 < ct_len)
			goto done;
		for (loop2 = 0; loop2 < ct_len; loop2++) {
			c1 = cs[loop1 + loop2];
			if ((c1 >= 'A') && (c1 <= 'Z'))
				c1 += ('a' - 'A');
			c2 = ct[loop2];
			if ((c2 >= 'A') && (c2 <= 'Z'))
				c2 += ('a' - 'A');
			if (c1 != c2)
				goto next;
		}
		return(&cs[loop1]);
next:;
	}
done:;
	return(NULL);
}

/***********************************************************
 * ReadNodeAttrString
 *	ReadAttrString is little weak, So check attr before read.
 ***********************************************************/
status_t
ReadNodeAttrString(BNode *node,const char* attrName,BString *out,const char* defaultValue)
{
	// reset output string.
	out->SetTo("");
	
	attr_info attr;
	status_t err = B_OK;
	// check whether it has the attribute.
	if(node->GetAttrInfo(attrName,&attr) == B_OK && attr.type == B_STRING_TYPE)
	{
		if(attr.size > 0)
		{
			char *buf = out->LockBuffer(attr.size+1);
			node->ReadAttr(attrName,B_STRING_TYPE,0,buf,attr.size);
			out->UnlockBuffer();
		}	
	}else{
		// if it doesn't have attr, write default value.
		if(defaultValue)
		{
			node->WriteAttr(attrName,B_STRING_TYPE,0,defaultValue,::strlen(defaultValue)+1);
			out->SetTo(defaultValue);
		}else
			err = B_ERROR;
	}
	return err;	
}

/***********************************************************
 * Alert
 ***********************************************************/
void Alert(alert_type type,const char* fmt, ...)
{
	int32 BUFFER_SIZE = 4096;
	char *buf;
	va_list args;
	va_start(args,fmt);
#if __INTEL__
	int len = 0;
	do{ 
        buf = new char[BUFFER_SIZE]; 
        len = ::vsnprintf( buf, BUFFER_SIZE, fmt, args); 
        if ( len < BUFFER_SIZE ) 
            (new BAlert("",buf,"OK"))->Go();
        delete[] buf; 
        BUFFER_SIZE *= 2;
    } while( len >= BUFFER_SIZE );
#else
	buf = new char[BUFFER_SIZE];
	::vsprintf(buf, fmt, args);
	(new BAlert("",buf,_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,type))->Go();
	delete[] buf;
#endif
	va_end(args);
}

/***********************************************************
 * MakeTime_t
 ***********************************************************/
time_t MakeTime_t(const char* date)
{
	const char* mons[] = {"Jan","Feb","Mar","Apr","May"
						,"Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	const char* wdays[] = {"Sun","Mon","Tue","Wed","Thu"
						,"Fri","Sat"};
	
	char swday[5];
	int wday;
	int day;
	char smon[4];
	int mon;
	char stime[9];
	int year;
	int hour;
	int min;
	int sec;
	int gmt_off = 0;
	char offset[5];
	::sscanf(date,"%s%d%s%d%s%s",swday,&day,smon,&year,stime,offset);
	
	// parse time
	hour = atoi(stime);
	min = atoi(stime+3);
	sec = atoi(stime+6);
	//PRINT(("TIME:%s\n",stime));
	//PRINT(("H:%d M:%d S:%d\n",hour,min,sec ));
	// month
	for(mon = 0;mon < 12;mon++)
	{
		if(strncmp(mons[mon],smon,3) == 0)
			break;
	}
	//PRINT(("MONTH:%s\n",smon));
	//PRINT(("M:%d\n",mon));
	// week of day
	for(wday = 0;wday < 12;wday++)
	{
		if(strncmp(wdays[wday],swday,3) == 0)
			break;
	}
	//PRINT(("WEEK:%s\n",swday));
	//PRINT(("W:%d\n",wday));
	// offset 
	char op = offset[0];
	int off = atoi(offset+1);
	if(op == '+')
		gmt_off  = (off/100)*60*60;
	if(op == '-')
		gmt_off  = -(off/100)*60*60;
	//PRINT(("GMT_OFF:%s\n",offset));
	//PRINT(("G:%d\n",gmt_off));
	struct tm btime;
	btime.tm_sec = sec;
	btime.tm_min = min;
	btime.tm_hour = hour;
	btime.tm_mday = day;
	btime.tm_mon = mon;
	btime.tm_year = year-1900;
	btime.tm_wday = wday;
	btime.tm_gmtoff = gmt_off;
	return mktime(&btime);	
}
 
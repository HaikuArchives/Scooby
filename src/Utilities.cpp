#include "Utilities.h"
#include "scandir.h"
#include "StatusItem.h"
#include "HApp.h"

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
#include <Resources.h>

typedef struct {
uint32 v1;
uint32 v2;
uint32 v3;
uint32 status;
uint32 rel;
} app_version_info;

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

BString& operator << (BString &str,BFile &file)
{
	int32 orglen = str.Length();
	off_t size;
	file.GetSize(&size);
	char *buf = str.LockBuffer(orglen+size+1);
	buf+=orglen;
	size = file.Read(buf,size);
	buf[size]='\0';
	str.UnlockBuffer();
	return str;
}


BString& operator >> (BString &str,BFile &file)
{
	file.Write(str.String(),str.Length());
	return str;
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

	const char* wdays[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	
	char swday[5];
	int wday=0;
	int day;
	char smon[4];
	int mon;
	//char stime[9];
	int year;
	int hour;
	int min;
	int sec;
	int gmt_off = 0;
	char offset[5];
	
	// Mon, 16 Oct 2000 00:50:04 +0900 or Mon, 16 Oct 00 00:50:04 +0900
	int num_scan  = ::sscanf(date,"%3s,%d%3s%d %2d:%2d:%2d %5s"
							,swday,&day,smon,&year,&hour,&min,&sec,offset);

   	if(num_scan != 8)
	{
		// 9 Nov 2000 11:30:23 -0000 or  29 Jan 01 9:16:08 PM
		num_scan = ::sscanf(date, "%d%3s%d %2d:%2d:%2d %5s"
								,&day,smon,&year,&hour,&min,&sec,offset);
		if(strcasecmp(offset,"PM") == 0)
			hour += 12;
		::strcpy(offset,"-0000");      
		if(num_scan != 7)
		{
			// 01 Feb 01 12:22:42 PM
			num_scan = sscanf(date, "%d %3s %3d %d %2d:%2d %5s",
                                  &day, smon, &year, &hour, &min,&sec,offset);
            if(strcasecmp(offset,"PM") == 0)
				hour += 12;
			if (num_scan != 7)
			{
			// Return current time if this can not parse date
				PRINT(("Unknown date format\n"));
					return time(NULL);
			}
		}
	}
	// month
	for(mon = 0;mon < 12;mon++)
	{
		if(strncmp(mons[mon],smon,3) == 0)
			break;
	}
	// week of day
	if(num_scan == 8)
	{
		for(wday = 0;wday < 12;wday++)
		{
			if(strncmp(wdays[wday],swday,3) == 0)
				break;
		}
	}
	// offset
	char op = offset[0];
	float off = atof(offset+1);
	if(op == '+')
		gmt_off  = static_cast<int>((off/100.0)*60*60);
	else if(op == '-')
		gmt_off  = static_cast<int>(-(off/100.0)*60*60);
	else
		gmt_off = 0;
	struct tm btime;
	btime.tm_sec = sec;
	btime.tm_min = min;
	btime.tm_hour = hour;
	btime.tm_mday = day;
	btime.tm_mon = mon;

	if(year < 100)
	{
		if(year < 70)
			year+=2000;
		else
			year+=1900;
	}
	btime.tm_year = year - 1900;
	if(num_scan == 8)
		btime.tm_wday = wday;
	btime.tm_gmtoff = gmt_off;
	return mktime(&btime);
}
 
/***********************************************************
 * GetAppVersion
 ***********************************************************/
void GetAppVersion(BString &version)
{
	version ="";
	BResources* rsrc = BApplication::AppResources();
	if(rsrc)
	{
		size_t resource_size;
		const char* app_version_data = (const char*)rsrc->LoadResource('APPV',
			"BEOS:APP_VERSION",&resource_size);
		if(app_version_data && resource_size > 20)
		{
			const char* status[] = {"Development","Alpha","Beta","Gamma",
				"Golden master","Final"};
			app_version_info *info = (app_version_info*)app_version_data;
			uint32 v1 = info->v1;
			uint32 v2 = info->v2;
			uint32 v3 = info->v3;
			version << v1 << "." << v2 << "." << v3;
			if(info->status != 5)
				version << " " <<status[info->status];
			if(info->rel != 0)
				version << " Release " << info->rel;
		}
	}
}
#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <TextView.h>
class StatusItem;

// misc functions
void	DisallowMetaKeys(BTextView *);
void	DisallowFilenameKeys(BTextView *);

int32	GetAllDirents(const char* path,struct dirent ***dirent,bool folder_only=false);
bool	IsHeaderLine(const char* line);
int32	ReadLine(BFile *file,BString* out);
void	EnableMenuItem(BMenuItem *item,bool enabled);
void	MarkMenuItem(BMenuItem *item,bool marked);
void	SetMenuItemLabel(BMenuItem *item,const char* label);
/*************** Statusbar Update func ***************/
void	LineUpdate(StatusItem *item);
void	SizeUpdate(StatusItem *item);
/******************************************************/
// get mail header parameters
bool 	GetParameter(const char *src,const char *param, char **dst);
char*	cistrstr(char *cs, char *ct);

#endif
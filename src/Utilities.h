#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <TextView.h>

// misc functions
void	DisallowMetaKeys(BTextView *);
void	DisallowFilenameKeys(BTextView *);

int32	GetAllDirents(const char* path,struct dirent ***dirent);
bool	IsHeaderLine(const char* line);
int32	ReadLine(BFile *file,BString* out);
void	EnableMenuItem(BMenuItem *item,bool enabled);
void	MarkMenuItem(BMenuItem *item,bool marked);
void	SetMenuItemLabel(BMenuItem *item,const char* label);
#endif
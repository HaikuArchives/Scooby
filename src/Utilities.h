#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <TextView.h>

// misc functions
void	DisallowMetaKeys(BTextView *);
void	DisallowFilenameKeys(BTextView *);

int32	GetAllDirents(const char* path,struct dirent ***dirent);
bool	IsHeaderLine(const char* line);
int32	ReadLine(BFile *file,BString* out);

#endif
#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <Alert.h>
#include <TextView.h>
class StatusItem;

//Misc functions.

//! Disallow meta keys in BTextView.
void	DisallowMetaKeys(BTextView *);
//! Disallow keys that not allow to use in filename.
void	DisallowFilenameKeys(BTextView *);

//! Get all dirent struct.
int32	GetAllDirents(const char* path,struct dirent ***dirent,bool folder_only=false);
//! Returns true if line is E-mail header.
bool	IsHeaderLine(const char* line);
//! Read line from file.
int32	ReadLine(BFile *file,BString* out);
//! Enable menu items with NULL pointer checking.
void	EnableMenuItem(BMenuItem *item,bool enabled);
//! Enable menu items with NULL pointer checking.
void	MarkMenuItem(BMenuItem *item,bool marked);
//! Set meuu item label with NULL pointer checking.
void	SetMenuItemLabel(BMenuItem *item,const char* label);
/** 
Statusbar item update func.
*/
void	LineUpdate(StatusItem *item);
void	SizeUpdate(StatusItem *item);
//! Get mail header parameters.
bool 	GetParameter(const char *src,const char *param, char **dst);
//! Case insensitive version of strstr.
char*	cistrstr(char *cs, char *ct);
//! Read mail attr checking. if you set default value and it doesn't have attr, default value will be written.
status_t	ReadNodeAttrString(BNode *node,const char* attrName,BString *out,const char* defaultValue=NULL);
//! Show Alert.
void	Alert(alert_type type,const char* format,...);
//! Convert date string to time_t
time_t	MakeTime_t(const char* date);
//! Get application version from its resource.
void	GetAppVersion(BString &out);
#endif
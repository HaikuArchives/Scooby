#ifndef __HSTRING_H__
#define __HSTRING_H__

#include <String.h>

class HString :public BString {
public:
						HString();
						HString(const char *);
						HString(const BString &);
						HString(const char *, int32 maxLength);
					
	virtual				~HString();
	
	int32				GetLine(int32 pos,BString* out);
	void				Format(const char* format, ...);
	
};
#endif
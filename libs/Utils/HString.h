#ifndef __HSTRING_H__
#define __HSTRING_H__

#include <String.h>

class HString :public BString {
public:
	int32				GetLine(int32 pos,BString* out);
	HString&			Format(const char* format, ...);
};
#endif

#include "Utilities.h"

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
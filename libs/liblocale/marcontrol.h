
#ifndef MARCONTROL_H_
#define MARCONTROL_H_

#include <String.h>

class BList;
class MarLexer;
struct MarStack;

enum MarError
{
	MAR_OK										= 0,
	MAR_FILE_OPEN_FAIL						= -2,
	MAR_ERROR									= -1				// means i've yet to write
																		// better error reporting :)
};

class MarControl
{
	MarLexer					*lexer;
	MarStack					*stack;
	BList						*files;

	protected:

	MarError					error;
	BString					errorstr;

	public:
								// pass a list of filenames to parse
								MarControl (int32, const char **);

								// read from stdin
								MarControl (void);

	virtual					~MarControl (void);

								// for parser use -- do not call!
	MarStack					*CurrentMessage (void);
	void						PushMessage (MarStack *);
	void						PopMessage (const char *, uint32);
	MarLexer					*Lexer (void) const;

								// you can redefine this to take appropriate
								// action if the parse fails.  You can use
								// either the error enum or errorstr to take
								// appropriate action.  If ParseError is called
								// and error is still MAR_OK, assume MAR_ERROR.
								// Also, you can call LineNumber for where the
								// parser left off
	virtual int				ParseError (const char *);

								// user land methods -- okay to call
	BMessage					*Message (void) const;
	MarError					Parse (void);
	int						LineNumber (void);
};

#endif

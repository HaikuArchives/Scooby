#ifndef __HMAILVIEW_H__
#define __HMAILVIEW_H__

#include <TextView.h>
#include <FilePanel.h>
#include <Entry.h>
#include "SpellCheckView.h"

#define MESSAGE_TEXT		"Message:"
#define MESSAGE_TEXT_H		 16
#define MESSAGE_TEXT_V		 20
#define MESSAGE_FIELD_H		 59
#define MESSAGE_FIELD_V		 11

#define CONTENT_TYPE		"content-type: "
#define CONTENT_ENCODING	"content-transfer-encoding: "
#define MIME_TEXT_PLAIN		"text/plain"
#define MIME_TEXT			"text"
#define MIME_HTML			"text/html"
#define MIME_MULTIPART		"multipart/"
#define QUOTE				">"

class HMailView;
class TSavePanel;

enum ENCLOSURE_TYPE		{TYPE_ENCLOSURE = 100, TYPE_BE_ENCLOSURE,
						 TYPE_HTML, TYPE_FTP, TYPE_MAILTO};

typedef struct {
	bool		header;
	bool		raw;
	bool		quote;
	bool		incoming;
	bool		close;
	bool		mime;
	HMailView	*view;
	BFile		*file;
	BList		*enclosures;
	sem_id		*stop_sem;
} reader;

typedef struct {
	int32		type;
	char		*name;
	char		*content_type;
	char		*encoding;
	int32		text_start;
	int32		text_end;
	off_t		file_offset;
	off_t		file_length;
	bool		saved;
	bool		have_ref;
	entry_ref	ref;
} hyper_text;


enum	MESSAGES			{REFS_RECEIVED = 64, LIST_INVOKED, WINDOW_CLOSED,
							 CHANGE_FONT};

enum	TEXT				{SUBJECT_FIELD = REFS_RECEIVED + 64, TO_FIELD,
							 ENCLOSE_FIELD, CC_FIELD, BCC_FIELD, NAME_FIELD};

enum	MENUS	{/* app 	{M_NEW = SUBJECT_FIELD + 64, M_EDIT_SIGNATURE,
								M_FONT, M_STYLE, M_SIZE, M_BEGINNER, M_EXPERT,*/
				/* file */	 M_REPLY, M_REPLY_ALL, M_FORWARD, M_RESEND,
								M_HEADER, M_RAW, /*M_SEND_NOW, M_SEND_LATER,*/
								M_SAVE, M_PRINT_SETUP, M_PRINT, M_DELETE,
								M_CLOSE_READ, M_CLOSE_SAME, M_CLOSE_CUSTOM,
				/* edit */	 M_UNDO, M_SELECT, M_QUOTE, M_REMOVE_QUOTE,
								M_SIGNATURE,
				/* encls*/ 	 M_ADD, M_REMOVE};

//!Read and compose view for plain mode.
class HMailView : public SpellCheckView {
public:
			//!Constructor.
					HMailView(BRect, bool, BFile*);
			//!Destructor.
					~HMailView(void);
			//!Reset view font.
			void	ResetFont();
			//!Reset all text_run_array.
			void	ResetTextRunArray();
			//!Check whether header is shown.
			bool	IsShowingHeader()const {return fHeader;}
			//!Check whether raw message is shown.
			bool	IsShowingRawMessage()const {return fRaw;}
			
			void	ClearList(void);
			void	LoadMessage(BFile*, bool quote, bool close, const char*);
			void	Open(hyper_text*);
	static status_t	Reader(reader*);
		status_t	Save(BMessage*);
			void	SaveBeFile(BFile*, char*, ssize_t);
			void	StopLoad(void);
			void	SetContent(BFile*);
	
			void	GetHardWrapedText(BString &out);
			int32	ByteLength(char c);
	//@{
	//!Override function.
			void	KeyDown(const char*, int32);
			void	MakeFocus(bool);
protected:
			void	AttachedToWindow(void);
			void	MessageReceived(BMessage*);
			void	MouseDown(BPoint);
			void	MouseMoved(BPoint, uint32, const BMessage*);
	//@}
	static	bool	get_semaphore(BWindow*, sem_id*);
	static	bool	insert(reader*, char*, int32, bool);
	static	bool	parse_header(char*, char*, off_t, char*, reader*, off_t*);
	static	bool	strip_it(char*, int32, reader*);
	static	int32	cistrncmp(char*, char*, int32);
	static	char*	cistrstr(char*, char*);
	static	void	extract(char**, char*);
	static	int32	linelen(char*, int32, bool);
	static	bool	get_parameter(char*, char*, char*);
	static	char*	find_boundary(char*, char*, int32);
	static	void	HighlightQuote(BTextView *view);
			void	Find(const char* text);
	
private:
	bool			fIncoming;
	bool			fRaw;
	char			*fYankBuffer;
	int32			fLastPosition;
	BFile			*fFile;
	BFont			fFont;
	sem_id			fStopSem;
	thread_id		fThread;
	BList			*fEnclosures;
	BPopUpMenu		*fMenu;
	TSavePanel		*fPanel;

	bool			fHeader;
	bool			fReady;
	
	typedef SpellCheckView _inherited;
};


//====================================================================
//!Attachment save panel.
class TSavePanel : public BFilePanel {

private:

hyper_text		*fEnclosure;
HMailView		*fView;

public:
				TSavePanel(hyper_text*, HMailView*);
		 void	SendMessage(const BMessenger*, BMessage*);
void			SetEnclosure(hyper_text*);
void			SetSaveFileName(const char* name);
};
#endif
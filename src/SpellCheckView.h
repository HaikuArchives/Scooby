#ifndef __SPELLCHECKVIEW_H__

#include "CTextView.h"
#include <String.h>
#include <List.h>

class SpellCheckView;

typedef struct {
	int32 start;
	int32 end;
	SpellCheckView *view;
} CheckThreadData;

class SpellCheckView :public CTextView {
public:
						SpellCheckView(BRect	frame,
								 	 const char	*name,
								 	 //BRect		textRect,
								 	 uint32		resizeMask,
								 	 uint32		flags = B_WILL_DRAW | B_PULSE_NEEDED);
						/*SpellCheckView(BRect		frame, 
								  const char		*name, 
								  BRect				textRect,
						 	 	  const BFont		*initialFont,
								  const rgb_color	*initialColor, 
								  uint32			resizeMask, 
								  uint32			flags);
						*/
	virtual 			~SpellCheckView();
			
			void		SetEnableSpellChecking(bool enable) {fEnabled = enable;}
			void		StartChecking(int32 start,int32 end);	
			void		SetColor(int32 start,int32 end,rgb_color *col);
			
			thread_id	fCheckThreadID;
protected:	
	virtual	void		InsertText(const char				*inText, 
								   int32					inLength, 
								   int32					inOffset,
								   const text_run_array		*inRuns);
	virtual	void		DeleteText(int32 fromOffset, int32 toOffset);
	
			bool		CheckWord(const char* word);
			bool		IsSameColor(rgb_color c1 ,rgb_color c2);
			
			void		InitDictionaries();
private:
	static	int32		CheckThread(void *data);
	bool				fEnabled;
	BList				fWordList;
	typedef	CTextView	_inherited;
};
#endif
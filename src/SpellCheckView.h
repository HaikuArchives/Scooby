#ifndef __SPELLCHECKVIEW_H__

#include "HWrapTextView.h"
#include <String.h>
#include <List.h>

//! Spell checking view.
class SpellCheckView :public HWrapTextView {
public:
		//!Constructor.
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
		//!Destructor.
	virtual 			~SpellCheckView();
		//!Set enable spell checking.
			void		SetEnableSpellChecking(bool enable);
		//!Start spell checking from start to end.
			void		StartChecking(int32 start //!<Start position offset.
									,int32 end //!<End position offset.
									);	
		//!Set text color.
			void		SetColor(int32 start //!<Start position offset.
									,int32 end //!<End position offset.
									,rgb_color *col //!<Text color.
									);
			
			thread_id	fCheckThreadID; //!<Check thread ID.
protected:	
	//@{
	//!Override functions.
	virtual	void		InsertText(const char				*inText, 
								   int32					inLength, 
								   int32					inOffset,
								   const text_run_array		*inRuns);
	virtual	void		DeleteText(int32 fromOffset, int32 toOffset);
	virtual void		MessageReceived(BMessage *message);
	//@}
			//! Returns true if input word's spelling is correct.
			bool		CheckWord(const char* word);
			//! Initialize word dictionaries.
			void		InitDictionaries();
private:
	static	int32		CheckThread(void *data);	//!<Spell check thread entry.
	bool				fEnabled;					//!<Flag that enable or disable spell checking.
	BList				fWordList;					//!<Word list.
	bool				fIsIM;						//!<Flag that avoid input from InputMethod.
	bool				fDicInited;					//!<Flag that whether all dictionaries have been loaded.
	
	typedef	HWrapTextView	_inherited;
};
#endif
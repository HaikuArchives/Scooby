#ifndef __HWRITEVIEW_H__
#define __HWRITEVIEW_H__

#include <TextView.h>
#include <String.h>

class HRulerView;

class HWrapTextView :public BTextView{
public:
						HWrapTextView(BRect frame
										,const char* name
										,int32 resize_mode
										,int32 flags);
		virtual 		~HWrapTextView();
		
		void			SetRightLimit(float right);
		float			RightLimit() const {return fRightLimit;}
		void			ResetTextRect();
		
		void			UseRuler(bool use);
		bool			IsUsingRuler() const {return fUseRuler;}
		
		void			AddRuler();
		void			RemoveRuler();
		
		int32			ByteLength(char c);
		void			GetHardWrapedText(BString &out);
		//! Hide BTextControls SetFontAndColor.
		void			SetFontAndColor(const BFont		*inFont, 
										uint32			inMode = B_FONT_ALL,
										const rgb_color	*inColor = NULL);
		void			SetFontAndColor(int32			startOffset, 
										int32			endOffset, 
										const BFont		*inFont,
										uint32			inMode = B_FONT_ALL,
										const rgb_color	*inColor = NULL);
protected:
		virtual void	FrameResized(float width, float height);
private:
		bool		fUseRuler;
		float		fRightLimit;
		HRulerView*	fRulerView;
		typedef		BTextView _inherited;
};
#endif
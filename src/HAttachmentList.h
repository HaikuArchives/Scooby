#ifndef __HATTACHMENTLIST_H__
#define __HATTACHMENTLIST_H__

#include "ColumnListView.h"

enum{
	M_SAVE_ATTACHMENT = 'mSaT',
	M_OPEN_ATTACHMENT = 'mOaT'
};

//!AttachmentList in HTML view mode.
/*! 
 * This class is used for incomming mails in HTMLView. not used in plain mode.
 */
class HAttachmentList :public ColumnListView{
public:
		//! Constructor.
						HAttachmentList(BRect frame,
										BetterScrollView **scroll,
										const char* name);
		//! Destructor.
						~HAttachmentList();
		//! Find attachment by content type.
			int32		FindPart(const char* content_type);
			
protected:
	//@{
	//!Override function.
		void		MouseDown(BPoint point);
		bool		InitiateDrag(BPoint point, 
								int32 index, 
								bool wasSelected);
	//@}
};
#endif

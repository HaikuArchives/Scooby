#ifndef __HATTACHMENTLIST_H__
#define __HATTACHMENTLIST_H__

#include "ColumnListView.h"

enum{
	M_SAVE_ATTACHMENT = 'mSaT',
	M_OPEN_ATTACHMENT = 'mOaT'
};

class HAttachmentList :public ColumnListView{
public:
						HAttachmentList(BRect frame,
										BetterScrollView **scroll,
										const char* title);
	virtual				~HAttachmentList();
			
			int32		FindPart(const char* content_type);
			
protected:
	virtual void		MouseDown(BPoint point);
	virtual	bool	InitiateDrag(BPoint point, 
								int32 index, 
								bool wasSelected);
};
#endif

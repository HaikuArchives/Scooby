#ifndef __MAIL_LIST_H__
#define __MAIL_LIST_H__

#include <string>
#include <iostream>
#include "ColumnListView.h"
#include "HMailItem.h"

#include <Entry.h>

enum{
M_SET_ITEM_MSG = 'HESI',
M_SET_CONTENT = 'HSEC',
M_SCROLL_DOWN = 'HSCD',
M_SET_UNREAD_COUNT = 'SEUR',
M_MAIL_DRAG = 'MDAG',
M_DELETE_MSG = 'DELM',
M_ADD_TO_PEOPLE= 'APEP',
M_SELECT_NEXT_MAIL = 'MSEN',
M_SELECT_PREV_MAIL = 'MseP',
M_INVOKE_MAIL = 'MINM'
};

enum ColumnType{
	COL_SUBJECT = 0,
	COL_FROM,
	COL_TO,
	COL_WHEN,
	COL_PRIORITY ,
	COL_ATTACHMENT
};

// column flags
enum{
	COLUMN_SUBJECT 		= 1,
	COLUMN_FROM 		= 1<<1,
	COLUMN_TO 			= 1<<2,
	COLUMN_WHEN 		= 1<<3,
	COLUMN_PRIORITY 	= 1<<4,
	COLUMN_ATTACHMENT 	= 1<<5

};


class HMailList	:public	 ColumnListView {
public:
					HMailList(BRect frame,BetterScrollView **scroll,const char* title);
	virtual			~HMailList();
	
			void	SaveColumns();
			bool	IsColumnShown(ColumnType type);
			void	SetColumnShown(ColumnType type,bool shown);
			void	RemoveMails(BList *list);
			void	AddMail(HMailItem *item);
			int32	SelectionCount();
protected:
	virtual void 	MessageReceived(BMessage *message);	
	virtual	bool	InitiateDrag(BPoint point, 
								int32 index, 
								bool wasSelected);
	virtual void	SelectionChanged();
	virtual void 	KeyDown(const char *bytes,int32 numBytes);
	virtual void	MouseDown(BPoint pos);
			int32	CalcInsertPosition(int32 count,
										int32 sort_key,
										int32 sort_mode,
										HMailItem *item);
			void	RefreshColumns(entry_ref ref);
			void	SetColumns(int32 flags,
									int32 *display_order,
									int32 sort_key,
									int32 sort_mode,
									float *column_width);	
private:
	BEntry  			*fCurrentFolder;
	BetterScrollView 	*fScrollView;
	bool				fIsQuery;
};
#endif
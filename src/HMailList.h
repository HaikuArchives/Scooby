#ifndef __MAIL_LIST_H__
#define __MAIL_LIST_H__

#include <string>
#include <iostream>
#include "HCoolListView.h"
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
	COL_ATTACHMENT,
	COL_CC,
	COL_SIZE,
	COL_ACCOUNT
};

// column flags
enum{
	COLUMN_SUBJECT 		= 1,
	COLUMN_FROM 		= 1<<1,
	COLUMN_TO 			= 1<<2,
	COLUMN_WHEN 		= 1<<3,
	COLUMN_PRIORITY 	= 1<<4,
	COLUMN_ATTACHMENT 	= 1<<5,
	COLUMN_CC			= 1<<6,
	COLUMN_SIZE 		= 1<<7,
	COLUMN_ACCOUNT		= 1<<8
};

//! Mail list view.
class HMailList	:public	 HCoolListView {
public:
				//!Constructor.
					HMailList(BRect frame,BetterScrollView **scroll,const char* title);
				//!Destructor.
	virtual			~HMailList();
				//!Save column positions.
			void	SaveColumnsAndPos();

			bool	IsColumnShown(ColumnType type);
			void	SetColumnShown(ColumnType type,bool shown);
			void	RemoveMails(BList *list);
			void	AddMail(HMailItem *item);
			bool	HasSomeSelection();
			void	MarkOldSelectionAsRead();
			
			void	SetOldSelection(HMailItem *item){fOldSelection = item;}
	HMailItem*		MailAt(int32 index);
protected:
	//@{
	//!Override function.
	virtual void 	MessageReceived(BMessage *message);	
	virtual	bool	InitiateDrag(BPoint point, 
								int32 index, 
								bool wasSelected);
	virtual void	SelectionChanged();
	virtual void 	KeyDown(const char *bytes,int32 numBytes);
	virtual void	MouseDown(BPoint pos);
	//@}
	//@{
	//!Scripting override.
	virtual BHandler*	ResolveSpecifier(BMessage *message,
									int32 index,
									BMessage *specifier,
									int32 what,
									const char *property);
	virtual	status_t	GetSupportedSuites(BMessage *data);
	//@}
			int32	CalcInsertPosition(int32 count,
										int32 sort_key,
										int32 sort_mode,
										HMailItem *item);
			//!Read column settings.
			status_t ReadSettings(const char* path, BMessage *msg);
			//!Refresh columns display order and width.
			void	RefreshColumns(BMessage *msg);
			//!
			void	SetColumns(int32 flags
								,int32 *display_order
								,int32 sort_key
								,int32 sort_mode
								,float *column_width
								);
			//!Reset scrollbar position.
			void	RefreshScrollPos(BMessage *msg);
			//!Make selected folder path.
			void	MakePath(BPath &basePath,const char* relative);
			//!Handle scripting messages.
			bool	HandleScriptingMessage(BMessage *message);
			//!Handle B_GET_PROPERTY scripting message.
			bool	GetProperty(BMessage *, int32 , const char *, BMessage *);
private:
	BEntry  			*fCurrentFolder;
	BetterScrollView 	*fScrollView;
	int32				fFolderType;
	HMailItem			*fOldSelection;

	typedef	HCoolListView _inherited;
};
#endif
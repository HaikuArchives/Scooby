#ifndef __LOCAL_LIST_H__
#define __LOCAL_LIST_H__

#include "ColumnListView.h"
#include "HFolderItem.h"
#include "HSimpleFolderItem.h"
#include "HCoolListView.h"

#include <Entry.h>

enum{
M_GET_VOLUMES = 'HLGV',
M_LOCAL_SELECTION_CHANGED = 'HLSC',
M_ADD_UNDER_ITEM = 'HLAU',
M_ADD_FOLDER = 'HLAV',
M_ADD_QUERY = 'HLAQ',
M_START_MAIL_BARBER_POLE = 'MSBP',
M_REFRESH_CACHE = 'mREc',
M_MOVE_MAIL = 'MMOV',
M_REMOVE_FROM_NODEMONITOR = 'MREI',
M_ADD_FROM_NODEMONITOR = 'ADIT',
M_OPEN_FOLDER = 'OpFD',
M_ADD_IMAP4_FOLDER = 'aIM4',
M_GATHER_ALL_MAILS = 'mGaM',
M_REMOVE_FOLDER = 'mREF'
};


class HFolderList	:public	 ColumnListView {
public:
						HFolderList(BRect frame,
									BetterScrollView **scroll,
									const char* title);
		virtual			~HFolderList();
				void	DeleteAll();
				void	SetWatching(bool watch) {fWatching = watch;}
				int32	FindFolder(entry_ref ref);
				int32	FindFolder(node_ref ref);
				
				int32 	FindQuery(const char* name);
				
			status_t	AddQuery(entry_ref ref);
			status_t	RemoveQuery(entry_ref &ref);
			status_t	RemoveQuery(node_ref &nref);
				
				void	WatchQueryFolder();
				void	WatchMailFolder();
				void	AddToMailList(HMailItem *item);
				void	RemoveFromMailList(HMailItem *item,bool free = false);

		HFolderItem*	RemoveFolder(int32 index);
				int32	GenarateFolderPathes(BMessage &msg);
				bool	IsGatheredLocalFolders() const {return fGatheredLocalFolders;}

protected:
		virtual void 	MessageReceived(BMessage *message);		
		static	int32 	GetFolders(void *data);
		static	void	GetChildFolders(const BEntry &entry,
										HFolderItem *parentItem,
										HFolderList *list,
										BMessage &out);
		virtual void	SelectionChanged();
		virtual void	Pulse();
		virtual void	MouseDown(BPoint pos);
				void	WhenDropped(BMessage *message);
		virtual void	MouseMoved(BPoint point,
									uint32 transit,
									const BMessage *message);
		status_t		SelectItem(const BPoint point);
				void	SelectWithoutGathering(int32 sel);
				void	NodeMonitor(BMessage *message);
				
				void	ProcessMails(BMessage *message);
				void	GetFolderPath(HFolderItem *item,BMessage &msg);
private:
		BEntry   	*fEntry;
		uint32	 	fOutlineLevel;
		HFolderItem *fLocalItem;
		bool		fCancel;
		thread_id	fThread;
		BList		fPointerList;
		bool		fWatching;
		bool		fSkipGathering;
		CLVColumn	*cFolders;
	// Parent items	
	HSimpleFolderItem	*fLocalFolders;
	HSimpleFolderItem	*fIMAP4Folders;
	HSimpleFolderItem	*fQueryFolders;
		bool		fSkipMoveMail;
		bool		fGatheredLocalFolders;
		bool		fGatherOnStartup;
		typedef ColumnListView	_inherited;
};
#endif
#ifndef __LOCAL_LIST_H__
#define __LOCAL_LIST_H__

#include "ColumnListView.h"
#include "HFolderItem.h"
#include "HSimpleFolderItem.h"
#include "HCoolListView.h"

#include <Entry.h>

enum{
M_GET_FOLDERS = 'HLGV',
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
M_GATHER_ALL_MAILS = 'mGaM',
M_REMOVE_FOLDER = 'mREF'
};

//!Folder list view.
class HFolderList	:public	 ColumnListView {
public:
				//!Constructor.
						HFolderList(BRect frame,
									BetterScrollView **scroll,
									const char* title);
				//!Destructor.
						~HFolderList();
				//!Deselect all items.
				void	DeleteAll();
				//!
				void	SetWatching(bool watch) {fWatching = watch;}
				//@{
				//!Find folder item.
				int32	FindFolder(entry_ref ref);
				int32	FindFolder(node_ref ref);
				//@}
				//!Find query item.
				int32 	FindQuery(const char* name/*<Query item name.*/);
				//!Add query item to this list.
			status_t	AddQuery(entry_ref ref/*<Query file's entry_ref.*/);
				//@{
				//!Remove query item.
			status_t	RemoveQuery(entry_ref &ref);
			status_t	RemoveQuery(node_ref &nref);
				//@}
				//@{
				//!Start node monitoring.
				void	WatchQueryFolder();
				void	WatchMailFolder();
				//@}
				//!Add new mail item to the mail list.
				void	AddToMailList(HMailItem *item);
				//!Remove mail item to the mail list.
				void	RemoveFromMailList(HMailItem *item,bool free = false);
				//!Remove folder item by index.(item are not freed)
		HFolderItem*	RemoveFolder(int32 index);
				//!Generate all folders' absolute pathes.
				int32	GenarateFolderPathes(BMessage &msg);
				//!Returns true if all local folders is loaded.
				bool	IsGatheredLocalFolders() const {return fGatheredLocalFolders;}
				//!Save folder structure cache.
				void	SaveFolderStructure();
				//!Check the account file is IMAP4 or not.
				bool	IsIMAP4Account(entry_ref &ref,BMessage *outSetting=NULL);

				BMessage	*fFoldersCache; //!<Folder cache data.
protected:
		//@{
		//!Override function.
			 	void 	MessageReceived(BMessage *message);		
			 	void	SelectionChanged();
			 	void	Pulse();
			 	void	MouseDown(BPoint pos);
			 	void	MouseMoved(BPoint point,
									uint32 transit,
									const BMessage *message);
		//@}
		static	int32 	GetFolders(void *data);
		static	void	GetChildFolders(const BEntry &entry,
										HFolderItem *parentItem,
										HFolderList *list,
										BMessage &out);
		//!Dropped mail handling.
		/**
			Handing moving mails including IMAP4.
		*/
				void	WhenDropped(BMessage *message);
		//!Select folder item by point.
		status_t		SelectItem(const BPoint point);
		//!Select item without mail gathering.
				void	SelectWithoutGathering(int32 sel);
		//!NodeMonitor handing.
				void	NodeMonitor(BMessage *message);
		//!Get folder's absolute path.
				void	GetFolderPath(HFolderItem *item,BMessage &msg);
		//!Load folders into list.
				bool	LoadFolders(entry_ref &ref //!<Folder's entry_ref to be added.
								,HFolderItem* parent //!<Parent item pointer.
								,int32 indent //!<Current indent level.
								,BMessage &rootFolders //!<Output data for root folders.
								,BMessage &childFolders); //!<Output data for child folders.
				void	LoadIMAP4Account(BMessage &msg); //!<Load IMAP4 account.
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
	//@{
	//! Parent items	
	HSimpleFolderItem	*fLocalFolders;
	HSimpleFolderItem	*fIMAP4Folders;
	HSimpleFolderItem	*fQueryFolders;
	//@}
		bool		fGatheredLocalFolders;
		bool		fGatherOnStartup;
		bool		fUseTreeMode;
		
		typedef ColumnListView	_inherited;
};
#endif
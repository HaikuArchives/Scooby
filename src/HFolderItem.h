#ifndef __HFolderItem_H__
#define __HFolderItem_H__

#include "CLVEasyItem.h"
#include <Entry.h>
#include <String.h>
#include <Handler.h>

enum{
	M_INVALIDATE = 'mINV'
};

enum{
	FOLDER_TYPE = 0,
	QUERY_TYPE,
	IMAP4_TYPE,
	SIMPLE_TYPE
};

class BListView;

class HMailItem;

//! Folder list item.
class HFolderItem : public CLVEasyItem,public BHandler
{
public:
				//!Constructor for local folders.
					HFolderItem(const entry_ref &ref,BListView *owner);
				//!Constructor for remote folder.
					HFolderItem(const char* name,int32 type,BListView *owner);
				//!Destructor.
	virtual			~HFolderItem();
				//!Start gathering mail thread.
	virtual	void	StartGathering();
				//!Returns maillist pointer.
		BList*		MailList() {return &fMailList;}
				//!If mail gethring thread is ended,it returns true.
			bool	IsReady()const {return (fThread == -1)?true:false;}		
				//!If all mails were gathered, it returns true.
			bool	IsDone()const {return fDone;}
				//!
			void	SetDone(bool done){fDone = done;}
				//!Returns unread mail count.
			int32	Unread()const {return fUnread;}
				//!Returns all mail count.
			int32	CountMails() const {return fMailList.CountItems();}
				//!Returns folder's entry_ref.
		entry_ref	Ref(){return fFolderRef;}	
				//!Returns folder's entry_ref.(for NodeMonitor.)
		node_ref	NodeRef() {return fNodeRef;}
				//!Returns folder name.
	const char*		FolderName() {return fName.String();};
				//!Set displayed folder name
			void	SetFolderName(const char* name){fName = name;}
				//!Set folder name label with unread count.
			void	SetUnreadCount(int32 unread);
				//!Free all mail and restart gathering.
	virtual	void	StartRefreshCache();
				//!Invalidate this item.
			void	InvalidateMe();
				//@{
				//!Remove mails from maillist. they are not freed.
			void	RemoveMails(BList* mails);
			void	RemoveMail(HMailItem* mail);
		HMailItem*	RemoveMail(entry_ref& ref);
		HMailItem*	RemoveMail(node_ref& nref);
				//@}
				//!Add mails by BList.
			void	AddMails(BList* mails);
				//!Add new mail item.
			void	AddMail(HMailItem* mail);
				//!Return folder type such as QUERY_TYPE,FOLDER_TYPE and so on.
			int32	FolderType() const{return fFolderType;}
				//!Free all mail items.
			void	EmptyMailList();	
				//!Launch this folder with Tracker.
			void	Launch();
				//!Mail gathering function.
			void	Gather();
				//!Folder sorting function.
	static 	int 	CompareItems(const CLVListItem *a_Item1, 
									const CLVListItem *a_Item2, 
									int32 KeyColumn);
				//!Returns owner listview.
	BListView*		Owner() const{return fOwner;}
				//!Return child item count.
			int32	ChildItems() const {return fChildItems;}
				//!Increment child item count.
			void	IncreaseChildItemCount() {fChildItems++;}
				//!Not implemented yet.
			void	RemoveSettings();
				//!Delete folder.
	virtual bool	DeleteMe();

protected:
				//!Re-create mail cache file.
			void	RefreshCache();
				//!Create cache with thread
			void	StartCreateCache(); 
				//!Not implemented yet.
			void	RemoveCacheFile();
				//!Create cache file and save cache.
			void	CreateCache();
				//!Add new mails to cache file.
			void	AddMailsToCacheFile();
				//!Not used.
	static 	int		CompareFunc(const void* data1,const void* data2);
				//!Read mails from cache.
		status_t	ReadFromCache();
	//@{
	//!Thread entry.
	static	int32 	ThreadFunc(void* data);
	static	int32	CreateCacheThread(void *data);
	static	int32	RefreshCacheThread(void *data);
	//@}
	
	virtual void	MessageReceived(BMessage *message);
				//!NodeMonitor handling function.
			void	NodeMonitor(BMessage *message);
				//!Check whether this item is selected.
			bool	IsSelected();
				//!Find mail item by node_ref.
		HMailItem*	FindMail(node_ref &nref);
protected:
		BList		fMailList;
		bool		fDone;
		int32		fUnread;
		thread_id 	fThread;
		bool		fCancel;
		BString		fName;
		entry_ref  	fFolderRef;
		BListView	*fOwner;
private:
		thread_id	fCacheThread;
		bool		fCacheCancel;
		thread_id	fRefreshThread;
		int32		fFolderType;
		bool		fUseCache;
		node_ref	fNodeRef;
		int32		fChildItems;
};
#endif

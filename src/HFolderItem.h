#ifndef __HFolderItem_H__
#define __HFolderItem_H__

#include "CLVEasyItem.h"
#include <Entry.h>
#include <String.h>

enum{
	M_INVALIDATE = 'mINV'
};


class HMailItem;

class HFolderItem : public CLVEasyItem
{
public:
					HFolderItem(const entry_ref &ref,BListView *owner);
	virtual			~HFolderItem();
			
	virtual	void	StartGathering();
		BList*		MailList() {return &fMailList;}
			bool	IsReady()const {return (fThread == -1)?true:false;}		
			bool	IsDone()const {return fDone;}
			int32	Unread()const {return fUnread;}
			int32	CountMails() const {return fMailList.CountItems();}
	
		entry_ref	Ref(){return fFolderRef;}	
	const char*		Name() {return fName.String();};
			void	SetName(int32 unread);
		
	virtual	void	StartRefreshCache();
			
			
			void	InvalidateMe();
			
			void	RemoveMails(BList* mails);
			void	AddMails(BList* mails);
			
			void	AddMail(HMailItem* mail);
			void	RemoveMail(HMailItem* mail);
		HMailItem*	RemoveMail(entry_ref ref);
		HMailItem*	RemoveMail(node_ref nref);
			
			bool	IsQuery() const{return fIsQuery;}
			void	EmptyMailList();	
			void	Launch();
			void	Gather();
	static 	int 	CompareItems(const CLVListItem *a_Item1, 
									const CLVListItem *a_Item2, 
									int32 KeyColumn);
protected:
			void	RefreshCache();	
			void	StartCreateCache(); // Create cache with thread
			void	RemoveCacheFile();
			void	CreateCache();
			void	AddMailsToCacheFile();
	
	static	int32 	ThreadFunc(void* data);
	static 	int		CompareFunc(const void* data1,const void* data2);
			
		status_t	ReadFromCache();
	static	int32	CreateCacheThread(void *data);
	static	int32	RefreshCacheThread(void *data);
	
protected:
		BList		fMailList;
		bool		fDone;
		int32		fUnread;
		thread_id 	fThread;
		bool		fCancel;
		BString		fName;
		entry_ref  	fFolderRef;
private:
		BListView	*fOwner;
		thread_id	fCacheThread;
		bool		fCacheCancel;
		thread_id	fRefreshThread;
		bool		fIsQuery;
		bool		fUseCache;
};
#endif
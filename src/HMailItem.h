#ifndef __HMailItem_H__
#define __HMailItem_H__

#include "CLVEasyItem.h"
#include <Entry.h>
#include <String.h>
#include <ListView.h>

enum{
	M_REFRESH_MAIL_ITEM = 'mReM'
};

class HMailItem : public CLVEasyItem
{
public:
						HMailItem(const entry_ref &ref);
						HMailItem(const entry_ref &ref,
								  const char* status,
								  const char* subject,
								  const char* from,
								  const char* to,
								  const char* cc,
								  const char* reply,
								  time_t 	  when,
								  const char* priority,
								  int8	enclosure,
								  ino_t	node = 0,
								  BListView* fOwner = NULL,
								  const char* account = NULL,
								  int32	size = 0);
						
						HMailItem(const char* status,
								  const char* subject,
								  const char* from,
								  const char* to,
								  const char* cc,
								  const char* reply,
								  time_t	  when,
								  const char* priority,
								  int8	enclosure,
								  const char* account = NULL,
								  int32 size = 0);
								  
	virtual				~HMailItem();
			void		InitItem();
	static 	int 		CompareItems(const CLVListItem *a_Item1, 
									const CLVListItem *a_Item2, 
									int32 KeyColumn);
			bool		IsRead()const;
	virtual	void		SetRead();
	virtual	void		ResetIcon();
	virtual	void		RefreshStatus();
			void		RefreshTextColor();
	virtual	void		RefreshEnclosureAttr();
	virtual	entry_ref	Ref() {return fRef;}

	static	void		MakeTime(BString &out,time_t &time);

//private:
		entry_ref 	fRef;
		BString		fStatus;
		BString		fSubject;
		BString		fFrom;
		BString		fTo;
		BString		fCC;
		BString 	fReply;
		time_t		fWhen;
		BString		fDate;
		BString		fPriority;
		int8		fEnclosure;
		bool		fDeleteMe;
		node_ref	fNodeRef;
		thread_id	fInitThread;
		BListView	*fOwner;
		BString		fAccount;
		int32		fSize;
protected:
	virtual void 	DrawItemColumn(BView* owner, 
								BRect item_column_rect, 
								int32 column_index, 
								bool complete);
private:
	static int32	RefreshStatusWithThread(void* data);
			
	typedef	CLVEasyItem	_inherited;
};
#endif
#ifndef __HQueryItem_H__
#define __HQueryItem_H__

#include "HFolderItem.h"
#include <ListView.h>
#include <String.h>
#include <Handler.h>
#include <Messenger.h>

class HQueryItem : public HFolderItem
{
public:
					HQueryItem(const entry_ref &ref,
								BListView *target);
					~HQueryItem();
	//@{
	//!Override function.
			void	StartRefreshCache();
			void	StartGathering();
	//@}
protected:
			void	Fetching();
	static	int32	FetchingThread(void* data);
	
private:
		BString		fPredicate;
		BQuery		*fQuery;
		BMessenger  *fMessenger;
};
#endif
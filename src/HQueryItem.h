#ifndef __HQueryItem_H__
#define __HQueryItem_H__

#include "HFolderItem.h"
#include <ListView.h>
#include <String.h>

class HQueryItem : public HFolderItem
{
public:
					HQueryItem(entry_ref ref,
								BListView *target);
	virtual			~HQueryItem();
	
	virtual	void	StartRefreshCache();
	virtual void	StartGathering();
protected:
			void	Fetching();
	static	int32	FetchingThread(void* data);
private:
		BString		fPredicate;
};
#endif
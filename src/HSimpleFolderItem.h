#ifndef __HSIMPLEFOLDERITEM_H__
#define __HSIMPLEFOLDERITEM_H__

#include "HFolderItem.h"

class HSimpleFolderItem :public HFolderItem{
public:
						HSimpleFolderItem(const char* name,
										BListView *list);
			bool		IsAdded() const{return fAdded;};
			void		Added(bool added) {fAdded = added;}
private:
	bool				fAdded;

};
#endif
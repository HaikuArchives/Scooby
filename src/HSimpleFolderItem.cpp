#include "HSimpleFolderItem.h"


/***********************************************************
 * Constructor
 ***********************************************************/
HSimpleFolderItem::HSimpleFolderItem(const char* name,BListView *list)
		:HFolderItem(name,SIMPLE_TYPE,list)
		,fAdded(false)
{
	fDone = true;
	SetSuperItem(true);
	SetExpanded(true);
	SetEnabled(false);
}

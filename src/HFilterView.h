#ifndef __HFILTERVIEW_H__
#define __HFILTERVIEW_H__

#include <View.h>
#include <ListView.h>
#include <MenuField.h>

enum{
	M_DEL_FILTER_MSG = 'MDEL',
	M_ADD_FILTER_MSG = 'MADD',
	M_FILTER_CHG = 'MfCH',
	M_FILTER_SAVE_CHANGED = 'MFIS',
	M_ADD_CRITERIA_MSG = 'MACR',
	M_DEL_CRITERIA_MSG = 'MDCR'
};


class HFilterView :public BView{
public:
					HFilterView(BRect rect);
	virtual			~HFilterView();
			void	InitGUI();
			void	AddFolderItem(BMessage *msg);
			
protected:
	virtual	void	MessageReceived(BMessage *message);
	virtual	void	Pulse();
			void	New();
			void	SetEnableControls(bool enalbe);
			void	SaveItem(int32 index,bool rename=true);
		status_t	OpenItem(const char* name);
		
			
			void	AddCriteria(int32 attr = -1,
							int32 operation = 0,
							const char* attr_value = NULL,
							int32 operation2 = 0);
			void	RemoveCriteria();
			void	RemoveAllCriteria();
			void	RefreshCriteriaScroll();
			
private:
	BListView		*fListView;
	BMenuField		*fActionMenu;
	BMenuField		*fFolderMenu;
	BTextControl	*fNameControl;
};
#endif
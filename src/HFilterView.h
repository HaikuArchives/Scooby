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

//!Incoming mail filter setting view.
class HFilterView :public BView{
public:
			//!Constructor.
					HFilterView(BRect rect);
			//!Destructor.
					~HFilterView();
			//!Initialize all GUI.
			void	InitGUI();
			//!Add folder item to Move menu.
			void	AddFolderItem(BMessage *msg);
			
protected:
	//@{
	//!Override function.
			void	MessageReceived(BMessage *message);
			void	Pulse();
			void	AttachedToWindow();	
	//@}
	//! Create new filter and add it to list.
			void	New();
	//! Enable or disable all controls.
			void	SetEnableControls(bool enable);
	//! Save filter setting.
			void	SaveItem(int32 index //!<List index.
							,bool rename=true //!<Rename filename or not.
							);
	//! Open filter setting.
		status_t	OpenItem(const char* name /*!<Filter name.*/);
	//! Add criteria to the criteria list.
			void	AddCriteria(int32 attr = -1 //!<Mail attribute index to be used filtering.
							,int32 operation = 0 //!<Criteria operator index such as "and, or, contains".
							,const char* attr_value = NULL //<! Criteria value.
							,int32 operation2 = 0 //<! Operator for next criteria.
							);
	//! Remove selected criteria from criteria list.
			void	RemoveCriteria();
	//! Remove all criteria.
			void	RemoveAllCriteria();
	//! Refresh criteria scrollview.
			void	RefreshCriteriaScroll();
private:
	BListView		*fListView;
	BMenuField		*fActionMenu;
	BMenuField		*fFolderMenu;
	BTextControl	*fNameControl;
};
#endif

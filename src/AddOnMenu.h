#ifndef __ADDONMENU_H__
#define __ADDONMENU_H__

#include <Menu.h>
#include <Path.h>

//!Add-ons menu.
class AddOnMenu :public BMenu{
public:
			//! Constructor.
					AddOnMenu(const char *title,		//!< Menu title.
							const char* directory_name, /*!< Addon direcotry name or relative path
														! ~/config/add-ons/(name).*/
							int32	what,				/*!< BMessage's what to be posted to target.
															It contains add-on's entry_ref.
														 */ 
							bool		use_icon=false  /*!< Use menu icons. Default value is false. */);
			//! Build menu items.
			void	Build();	
			//! Remove all menu items and re-build them.
			void	Rebuild();
protected:
			//! Get add-on's file icon. If it failed to get an icon,return NULL.
	BBitmap*			GetIcon(entry_ref &ref /*!< The file ref to be loaded an icon.*/);
private:
			//! Callback func for BList to sort add-ons items by filename.
	static	int			SortItems(const void* data1,const void* data2);
	BPath				fPath;		//!< Add-ons directory path.
	int32				fWhat;  	//!< Add-ons message's what.
	bool				fUseIcon;	//!< Flags for whether use menu icons or not.
};
#endif

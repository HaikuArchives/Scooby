#ifndef __ADDONMENU_H__
#define __ADDONMENU_H__

#include <Menu.h>
#include <Path.h>

class AddOnMenu :public BMenu{
public:
					AddOnMenu(const char *title,
							const char* directory_name, // addon direcotry name or relative path
														// ~/config/add-ons/(name)
							int32	what,
							bool		use_icon=false);
	virtual			~AddOnMenu();
			void	Build();
			void	Rebuild();
protected:
	BBitmap*			GetIcon(entry_ref &ref);
private:
	static	int			SortItems(const void* data1,const void* data2);
	BPath				fPath;	// Addon directory path
	int32				fWhat;  // Addon message's what
	bool					fUseIcon;
};
#endif

#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

#include <View.h>
#include <List.h>
#include "StatusItem.h"

//!Statusbar
class StatusBar :public BView{
public:
		//!Constructor.
					StatusBar(BRect rect,
							const char* name,
							uint32 resize,
							uint32 flags=B_WILL_DRAW);
		//!Destructor.
	virtual			~StatusBar();
	
		//!Add new StatusItem.
			void	AddItem(const char* name //!< Status item name.
							,const char* initialText //!<Initial text.
							,void (*pulseFunc)(StatusItem* item) //!Update callback func.
							);
		//!Remove item by name.
			void	RemoveItem(const char* name/*!Status item name.*/);
		//!Rearrange items width after start item.
			void	RearrangeItems(StatusItem *start,float delta);
			
protected:

private:
	BList			fItemList; //!<Status item pointer list.
};
#endif
#ifndef __STATUSBAR_H__
#define __STATUSBAR_H__

#include <View.h>
#include <List.h>
#include "StatusItem.h"

class StatusBar :public BView{
public:
					StatusBar(BRect rect,
							const char* name,
							uint32 resize,
							uint32 flags=B_WILL_DRAW);
	virtual			~StatusBar();
	
	
			void	AddItem(const char* name,
							const char* initialText,
							void (*pulseFunc)(StatusItem* item));
							
			void	RemoveItem(const char* name);
			
			void	RearrangeItems(StatusItem *start,float delta);
			
protected:

private:
	BList			fItemList;
};
#endif

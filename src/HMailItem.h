#ifndef __HMailItem_H__
#define __HMailItem_H__

#include "CLVEasyItem.h"
#include <Entry.h>
#include <String.h>

class HMailItem : public CLVEasyItem
{
public:
						HMailItem(const entry_ref &ref);
						HMailItem(const entry_ref &ref,
								  const char* status,
								  const char* subject,
								  const char* from,
								  const char* to,
								  const char* cc,
								  time_t 	  when,
								  const char* priority,
								  int8	enclosure);
						
						HMailItem(const char* status,
								  const char* subject,
								  const char* from,
								  const char* to,
								  time_t	  when,
								  const char* priority,
								  int8	enclosure);
								  
	virtual				~HMailItem();
			void		InitItem();
	static 	int 		CompareItems(const CLVListItem *a_Item1, 
									const CLVListItem *a_Item2, 
									int32 KeyColumn);
			bool		IsRead()const;
	virtual	void		SetRead();
	virtual	void		ResetIcon();
	virtual	void		RefreshStatus();
			void		RefreshTextColor();
	virtual	void		RefreshEnclosureAttr();
	virtual	entry_ref			Ref() {return fRef;}
//private:
		entry_ref 	fRef;
		BString		fStatus;
		BString		fSubject;
		BString		fFrom;
		BString		fTo;
		BString		fCC;
		time_t		fWhen;
		BString		fDate;
		BString		fPriority;
		int8		fEnclosure;
		bool		fDeleteMe;
};
#endif
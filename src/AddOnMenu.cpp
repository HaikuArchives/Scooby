#include "AddOnMenu.h"

#include <Directory.h>
#include <Entry.h>
#include <MenuItem.h>
#include <Message.h>
#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/***********************************************************
 * Constructor
 ***********************************************************/
AddOnMenu::AddOnMenu(const char *title,
					const char* directory_name,
					int32	what)
		:BMenu(title)
		,fWhat(what)
{
	::find_directory(B_USER_DIRECTORY,&fPath);
	fPath.Append("config/add-ons");
	fPath.Append(directory_name);
}

/***********************************************************
 * Destructor
 ***********************************************************/
AddOnMenu::~AddOnMenu()
{
}

/***********************************************************
 * Rebuild
 ***********************************************************/
void
AddOnMenu::Rebuild()
{
	// Delete all items
	BMenuItem *item;
	for(;;){
		item = RemoveItem(0L);
		if(!item)
			break;
		delete item;
	}
	// Build items again
	Build();
}

/***********************************************************
 * Build
 ***********************************************************/
void
AddOnMenu::Build()
{
	// Build add addons menus
	if(!fPath.Path())
		return;
	BDirectory 	dir(fPath.Path());
	entry_ref 	ref;
	BEntry 		entry;
	BList		itemList;
	itemList.MakeEmpty();
	while(dir.GetNextEntry(&entry,true) == B_OK)
	{
		if(entry.IsFile() && entry.GetRef(&ref) == B_OK)
		{	
			BMessage *msg = new BMessage(fWhat);
			msg->AddRef("refs",&ref);
			itemList.AddItem(new BMenuItem(ref.name,msg));
		}
	}
	
	// sort items
	itemList.SortItems(SortItems);
	
	int32 count = itemList.CountItems();
	for(int32 i = 0;i < count;i++)
		AddItem((BMenuItem*)itemList.ItemAt(i));
}

/***********************************************************
 * SortItems
 ***********************************************************/
int
AddOnMenu::SortItems(const void *data1,const void *data2)
{
	return strcmp((*(BMenuItem**)data1)->Label(),(*(BMenuItem**)data2)->Label());
}
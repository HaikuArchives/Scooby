#include "AddOnMenu.h"
#include "IconMenuItem.h"

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
					int32	what,
					bool		use_icon)
		:BMenu(title)
		,fWhat(what)
		,fUseIcon(use_icon)
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
	char 		name[B_FILE_NAME_LENGTH];
	char		shortcut = 0;
	BBitmap 	*bitmap(NULL);
	
	while(dir.GetNextEntry(&entry,true) == B_OK)
	{
		if(entry.IsFile() && entry.GetRef(&ref) == B_OK)
		{	
			shortcut = 0;
			bitmap = NULL;
			BMessage *msg = new BMessage(fWhat);
			msg->AddRef("refs",&ref);
			// make name and shortcut
			int32 nameLen = ::strlen(ref.name);
			::strcpy(name,ref.name);
			if(name[nameLen-2] == '-')
			{
				shortcut = name[nameLen-1];
				name[nameLen-2] = '\0';
			}
			if(fUseIcon)
				bitmap = GetIcon(ref);
			itemList.AddItem(new IconMenuItem(name,msg,shortcut,0,bitmap));
		}
	}
	
	// sort items
	itemList.SortItems(SortItems);
	
	int32 count = itemList.CountItems();
	for(int32 i = 0;i < count;i++)
		AddItem((IconMenuItem*)itemList.ItemAt(i));
}

/***********************************************************
 * GetIcon
 ***********************************************************/
BBitmap*
AddOnMenu::GetIcon(entry_ref &ref)
{
	BBitmap bitmap(BRect(0,0,15,15),B_CMAP8);
	BNodeInfo::GetTrackerIcon(&ref,&bitmap,B_MINI_ICON);
	return new BBitmap(&bitmap);
}

/***********************************************************
 * SortItems
 ***********************************************************/
int
AddOnMenu::SortItems(const void *data1,const void *data2)
{
	return strcmp((*(BMenuItem**)data1)->Label(),(*(BMenuItem**)data2)->Label());
}
#include "HAddressView.h"
#include "MenuUtils.h"
#include "ResourceUtils.h"
#include "HApp.h"
#include "HPrefs.h"
#include "ArrowButton.h"
#include "IconMenuItem.h"
#include "Utilities.h"

#include <TextControl.h>
#include <StringView.h>
#include <Query.h>
#include <Menu.h>
#include <MenuField.h>
#include <String.h>
#include <Entry.h>
#include <Window.h>
#include <Debug.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <ClassInfo.h>
#include <List.h>
#include <Directory.h>
#include <Path.h>
#include <Alert.h>
#include <FindDirectory.h>
#include <SupportDefs.h>

typedef struct PersonData{
	char* group;
	char* email;
};

/***********************************************************
 * Constructor
 ***********************************************************/
HAddressView::HAddressView(BRect rect,bool readOnly)
	:BView(rect,"AddressView",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,B_WILL_DRAW)
	,fSubject(NULL)
	,fTo(NULL)
	,fCc(NULL)
	,fBcc(NULL)
	,fReadOnly(readOnly)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

/***********************************************************
 * Destructor
 ***********************************************************/
HAddressView::~HAddressView()
{
	// Save send account
	int32 smtp_account;
	BMenuField *field = cast_as(FindView("FromMenu"),BMenuField);
	BMenu *menu = field->Menu();
	smtp_account = menu->IndexOf(menu->FindMarked());
	((HApp*)be_app)->Prefs()->SetData("smtp_account",smtp_account);
	
	// free memories
	int32 count = fAddrList.CountItems();
	while(count>0)
	{
		char *p = (char*)fAddrList.RemoveItem(--count);
		if(p)
			free(p);
	}
	fTo->SetModificationMessage(NULL);
	fCc->SetModificationMessage(NULL);
	fBcc->SetModificationMessage(NULL);
	fSubject->SetModificationMessage(NULL);
}

/***********************************************************
 * InitGUI
 ***********************************************************/
void
HAddressView::InitGUI()
{
	float divider = StringWidth(_("Subject:")) + 20;
	divider = max_c(divider , StringWidth(_("From:"))+20);
	divider = max_c(divider , StringWidth(_("To:"))+20);
	divider = max_c(divider , StringWidth(_("Bcc:"))+20);
	
	BRect rect = Bounds();
	rect.top += 5;
	rect.left += 20 + divider;
	rect.right = Bounds().right - 5;
	rect.bottom = rect.top + 25;
	
	BTextControl *ctrl;
	ResourceUtils rutils;
	const char* name[] = {"to","subject","from","cc","bcc"};
	
	for(int32 i = 0;i < 5;i++)
	{
		ctrl = new BTextControl(BRect(rect.left,rect.top
								,(i == 1)?rect.right+divider:rect.right
								,rect.bottom)
								,name[i],"","",NULL
								,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,B_WILL_DRAW|B_NAVIGABLE);
		
		if(i == 1)
		{
			ctrl->SetLabel(_("Subject:"));
			ctrl->SetDivider(divider);
			ctrl->MoveBy(-divider,0);
		}else{
			ctrl->SetDivider(0);
		}
		BMessage *msg = new BMessage(M_MODIFIED);
		msg->AddPointer("pointer",ctrl);
		ctrl->SetModificationMessage(msg);
		ctrl->SetEnabled(!fReadOnly);
		AddChild(ctrl);
	
		rect.OffsetBy(0,25);
		switch(i)
		{
		case 0:
			fTo = ctrl;
			break;
		case 1:
			fSubject = ctrl;
			break;
		case 2:
			fFrom = ctrl;
			fFrom->SetEnabled(false);
			fFrom->SetFlags(fFrom->Flags() & ~B_NAVIGABLE);
			break;
		case 3:
			fCc = ctrl;
			break;
		case 4:
			fBcc = ctrl;
			break;
		}
	}
	//
	BRect menuRect= Bounds();
	menuRect.top += 5;
	menuRect.left += 22;
	menuRect.bottom = menuRect.top + 25;
	menuRect.right = menuRect.left + 16;
	
	BMenu *toMenu = new BMenu(_("To:"));
	BMenu *ccMenu = new BMenu(_("Cc:"));
	BMenu *bccMenu = new BMenu(_("Bcc:"));
	BQuery query;
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);
	query.SetVolume(&volume);
	query.SetPredicate("((META:email=*)&&(BEOS:TYPE=application/x-person))");

	if(!fReadOnly && query.Fetch() == B_OK)
	{
		BString addr[4],name,group,nick;
		entry_ref ref;
		BList peopleList;
	
	
		while(query.GetNextRef(&ref) == B_OK)
		{
			BNode node(&ref);
			if(node.InitCheck() != B_OK)
				continue;
			
			ReadNodeAttrString(&node,"META:name",&name);		
			ReadNodeAttrString(&node,"META:email",&addr[0]);
			ReadNodeAttrString(&node,"META:email2",&addr[1]);
			ReadNodeAttrString(&node,"META:email3",&addr[2]);
			ReadNodeAttrString(&node,"META:email4",&addr[3]);
			ReadNodeAttrString(&node,"META:group",&group);
			ReadNodeAttrString(&node,"META:nickname",&nick);
			
			for(int32 i = 0;i < 4;i++)
			{
				if(addr[i].Length() > 0)
				{
					if(nick.Length() > 0)
					{
						nick += " <";
						nick += addr[i];
						nick += ">";
						fAddrList.AddItem(strdup(nick.String()));
					}
					fAddrList.AddItem(strdup(addr[i].String()));
					
					BString title = name;
					title << " <" << addr[i] << ">";
				
					AddPersonToList(peopleList,title.String(),group.String());
				}
			}
		}
		
		// Sort people data
		peopleList.SortItems(HAddressView::SortPeople);
		// Build menus
		BTextControl *control[3] = {fTo,fCc,fBcc};
		BMenu *menus[3] = {toMenu,ccMenu,bccMenu};
		int32 count = peopleList.CountItems();
		PersonData *data;
		bool needSeparator = false;
		bool hasSeparator = false;
		for(int32 k = 0;k < 3;k++)
		{
			for(int32 i = 0;i < count;i++)
			{
				BMessage *msg = new BMessage(M_ADDR_MSG);
				msg->AddPointer("pointer",control[k]);
				data =  (PersonData*)peopleList.ItemAt(i);
				msg->AddString("email",data->email);
				if(needSeparator && !hasSeparator && strlen(data->group) == 0)
				{
					menus[k]->AddSeparatorItem();
					hasSeparator = true;
				}else
					needSeparator = true;
				AddPerson(menus[k],data->email,data->group,msg,0,0);
			}
			hasSeparator = false;
			needSeparator = false;
		}
		// free all data
		while(count > 0)
		{
			data =  (PersonData*)peopleList.RemoveItem(--count);
			free(data->email);
			free(data->group);
			delete data;
		}
	}
	BMenuField *field = new BMenuField(menuRect,"ToMenu","",toMenu,
							B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW);
	field->SetDivider(0);
	field->SetEnabled(!fReadOnly);
	AddChild(field);
	
	rect = menuRect;
	rect.OffsetBy(0,28);
	rect.left = Bounds().left + 5;
	rect.right = rect.left + 16;
	rect.top += 26;
	rect.bottom = rect.top + 16;
	ArrowButton *arrow = new ArrowButton(rect,"addr_arrow"
										,new BMessage(M_EXPAND_ADDRESS));
	AddChild(arrow);
	//==================== From menu
	BMenu *fromMenu = new BMenu(_("From:"));
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	BDirectory dir(path.Path());
	BEntry entry;
	status_t err = B_OK;
	int32 account_count = 0;
	while(err == B_OK)
	{
		if((err = dir.GetNextEntry(&entry)) == B_OK && !entry.IsDirectory())
		{
			char name[B_FILE_NAME_LENGTH+1];
			entry.GetName(name);
			BMessage *msg = new BMessage(M_ACCOUNT_CHANGE);
			msg->AddString("name",name);
			BMenuItem *item = new BMenuItem(name,msg);
			fromMenu->AddItem(item);
			item->SetTarget(this,Window());
			account_count++;
		}
	}
	if(account_count != 0)
	{
		int32 smtp_account;
		((HApp*)be_app)->Prefs()->GetData("smtp_account",&smtp_account);
		BMenuItem *item(NULL);
		if(account_count > smtp_account)
			item = fromMenu->ItemAt(smtp_account);
		if(!item)
			item = fromMenu->ItemAt(0);
		if(item)
		{
			ChangeAccount(item->Label());
			item->SetMarked(true);
		}
	}else{
		(new BAlert("",_("Could not find mail accounts"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_INFO_ALERT))->Go();
		Window()->PostMessage(B_QUIT_REQUESTED);
	}
	fromMenu->SetRadioMode(true);
	
	menuRect.OffsetBy(0,25*2);
	field = new BMenuField(menuRect,"FromMenu","",fromMenu,
							B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW);
	field->SetDivider(0);
	
	AddChild(field);
	//=================== CC menu
	menuRect.OffsetBy(0,25);
	field = new BMenuField(menuRect,"CcMenu","",ccMenu,
							B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW);
	field->SetDivider(0);
	field->SetEnabled(!fReadOnly);
	AddChild(field);

	//=================== BCC menu	
	menuRect.OffsetBy(0,25);
	field = new BMenuField(menuRect,"BccMenu","",bccMenu,
							B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW);
	field->SetDivider(0);
	field->SetEnabled(!fReadOnly);
	AddChild(field);
	

}

/***********************************************************
 * FindAddress
 ***********************************************************/
void
HAddressView::FindAddress(const char* address,BMessage &outlist)
{
	int32 count = fAddrList.CountItems();
	outlist.MakeEmpty();
	
	for(int32 i = 0;i < count;i++)
	{
		const char* p = (const char*)fAddrList.ItemAt(i);
		if(::strncmp(p,address,strlen(address)) == 0)
		{
			outlist.AddString("address",p);
		}
	}
}

/***********************************************************
 * SetReadOnly
 ***********************************************************/
void
HAddressView::SetReadOnly(bool enable)
{
	fCc->SetEnabled(enable);
	fBcc->SetEnabled(enable);
	fSubject->SetEnabled(enable);
	fTo->SetEnabled(enable);
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
HAddressView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_ACCOUNT_CHANGE:
	{
		const char* name;
		if(message->FindString("name",&name) != B_OK)
			break;
		ChangeAccount(name);
		PRINT(("Name:%s\n",name));
		break;
	}
	case M_SEL_GROUP:
	{
		BMenu *menu;
		BTextControl *control;
		if(message->FindPointer("menu",(void**)&menu) != B_OK ||
			message->FindPointer("control",(void**)&control) != B_OK)
			break;
		if(control)
		{
			BString text = control->Text();
			if(text.Length() != 0)
				text += ",";
			int32 count = menu->CountItems();
			for(int32 i = 0;i < count;i++)
			{
				const char* label = menu->ItemAt(i)->Label();
				text += label;
				if(i != count-1)
					text += ",";
			}
			control->SetText(text.String() );
		}
		break;
	}
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 * SetFrom
 ***********************************************************/
void
HAddressView::SetFrom(const char* in_address)
{
	if( ::strlen(in_address) == 0)
		return;
	BString address(in_address);
	
	// Compare existing accounts	
	char name[B_FILE_NAME_LENGTH];
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	BDirectory dir(path.Path());
	status_t err = B_OK;
	BEntry entry;
	while(err == B_OK)
	{
		if( (err = dir.GetNextEntry(&entry)) != B_OK  )
			break;
		BFile file(&entry,B_READ_ONLY);
		if(file.InitCheck() == B_OK && entry.IsFile())
		{
			BMessage msg;
			msg.Unflatten(&file);
			BString myAddress;
		
			if(msg.FindString("address",&myAddress) != B_OK)
				myAddress = "";
			// Change account
			if(address.FindFirst(myAddress) != B_ERROR)
			{
				entry.GetName(name);	
				ChangeAccount(name);
				// Set From menu
				BMenuField *field = cast_as(FindView("FromMenu"),BMenuField);
				BMenu *menu = field->Menu();
				BMenuItem *item = menu->FindItem(name);
				if(item)
					item->SetMarked(true);
				break;
			}
		}
	}
}

/***********************************************************
 * ChangeAccount
 ***********************************************************/
void
HAddressView::ChangeAccount(const char* name)
{
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append(APP_NAME);
	path.Append("Accounts");
	path.Append(name);
	
	BFile file(path.Path(),B_READ_ONLY);
	if(file.InitCheck() == B_OK)
	{
		BMessage msg;
		msg.Unflatten(&file);
		BString name,from,address;
		
		if(msg.FindString("real_name",&name) != B_OK)
			name = "";
		if(msg.FindString("address",&address) != B_OK)
		{
			address = "";
			(new BAlert("",_("Cound not find your email address!\nPlease check your account"),_("OK")))->Go();
			return;
		}
		if(name.Length() > 0)
			from << "\"" <<name << "\" <";
		from += address;
		
		if(name.Length() > 0)
			from << ">";
		fFrom->SetText(from.String());
	}
}

/***********************************************************
 * AccountName
 ***********************************************************/
const char*
HAddressView::AccountName()
{
	BMenuField *field = cast_as(FindView("FromMenu"),BMenuField);
	BMenu *menu = field->Menu();
	BMenuItem *item = menu->FindMarked();
	if(!item)
		return NULL;
	return item->Label();
}

/***********************************************************
 * EnableJump
 ***********************************************************/
void
HAddressView::EnableJump(bool enable)
{
	uint32 flags = fCc->Flags();
	flags = (enable)?flags|B_NAVIGABLE_JUMP:flags | B_NAVIGABLE;
	fCc->SetFlags(flags);
	fBcc->SetFlags(flags);
}

/***********************************************************
 * FocusedView
 ***********************************************************/
BTextControl*
HAddressView::FocusedView() const
{
	int32 count = CountChildren();
	
	BTextControl *child(NULL);
	for(int32 i = 0;i < count;i++)
	{
		child = cast_as(ChildAt(i),BTextControl);
		if(child && child->TextView()->IsFocus())
			return child;
	}
	return NULL;
}

/***********************************************************
 * AddPerson
 ***********************************************************/
void
HAddressView::AddPersonToList(BList &list,const char* email,const char* group)
{
	PersonData *data = new PersonData;
	
	data->email = strdup(email);
	data->group = strdup(group);
	
	list.AddItem(data);
}

/***********************************************************
 * AddPerson
 ***********************************************************/
void
HAddressView::AddPerson(BMenu *menu
							,const char* title
							,const char* group
							,BMessage *msg
							, char shortcut
							, uint32 modifiers)
{
	BMenu *subMenu(NULL);
	HApp *app = (HApp*)be_app;
	MenuUtils utils;
	
	if(::strlen(group) > 0)
	{
		// Find group item
		int32 count = menu->CountItems();
		for(int32 i = 0;i < count;i++)
		{
			BMenuItem *tmpMenu = menu->ItemAt(i);
			if(tmpMenu && tmpMenu->Submenu())
			{
				if(::strcmp(tmpMenu->Label(),group) == 0)
				{
					subMenu = tmpMenu->Submenu();
					break;
				}
			}
		}
		// Add group item
		if(!subMenu)
		{
			subMenu = new BMenu(group);
			BMessage *message = new BMessage(M_SEL_GROUP);
			message->AddString("group",group);
			BTextControl *control;
			msg->FindPointer("pointer",(void**)&control);
			message->AddPointer("control",control);
			message->AddPointer("menu",subMenu);
			BFont font(be_plain_font);
			font.SetSize(10);
			subMenu->SetFont(&font);
			IconMenuItem *iconItem = new IconMenuItem(subMenu,message,0,0
						,app->GetIcon("OpenFolder"));
			iconItem->SetTarget(this,Window());
		
			menu->AddItem(iconItem);
		}
		// Add item
		utils.AddMenuItem(subMenu,title,msg,this,Window(),shortcut,modifiers
						,app->GetIcon("Person"),false);
		
	}else
		utils.AddMenuItem(menu,title,msg,this,Window(),shortcut,modifiers
						,app->GetIcon("Person"),false);
}

/***********************************************************
 * SortPeople
 ***********************************************************/
int
HAddressView::SortPeople(const void *inData1,const void* inData2)
{
	const PersonData* data1 = *(const PersonData**)inData1;
	const PersonData* data2 = *(const PersonData**)inData2;
	
	int32 len1 = ::strlen(data1->group);
	int32 len2 = ::strlen(data2->group);
	
	if(len1 != 0 && len2 == 0)
		return -1;
	else if(len1 == 0 && len2 != 0)
		return 1;		
	else if(len1 != 0 && len2 != 0)
	{
		int32 result = ::strcasecmp(data1->group,data2->group);
		if(result== 0)
			return ::strcasecmp(data1->email,data2->email);
		return result;
	}
	return ::strcasecmp(data1->email,data2->email);
}
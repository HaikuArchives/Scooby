#include "HAddressView.h"
#include "MenuUtils.h"
#include "ResourceUtils.h"
#include "HApp.h"
#include "HPrefs.h"
#include "ArrowButton.h"

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
	const float kDivider = StringWidth("Subject:") +7;
	
	BRect rect = Bounds();
	rect.top += 5;
	rect.left += 20 + kDivider;
	rect.right = Bounds().right - 5;
	rect.bottom = rect.top + 25;
	
	BTextControl *ctrl;
	ResourceUtils rutils;
	const char* name[] = {"to","subject","cc","bcc","from"};
	
	for(int32 i = 0;i < 5;i++)
	{
		ctrl = new BTextControl(BRect(rect.left,rect.top
								,(i == 1)?rect.right+kDivider:rect.right
								,rect.bottom)
								,name[i],"","",NULL
								,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,B_WILL_DRAW|B_NAVIGABLE);
		
		if(i == 1)
		{
			ctrl->SetLabel("Subject:");
			ctrl->SetDivider(kDivider);
			ctrl->MoveBy(-kDivider,0);
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
			fCc = ctrl;
			break;
		case 3:
			fBcc = ctrl;
			break;
		case 4:
			fFrom = ctrl;
			fFrom->SetEnabled(false);
			fFrom->SetFlags(fFrom->Flags() & ~B_NAVIGABLE);
			break;
		}
	}
	//
	BRect menuRect= Bounds();
	menuRect.top += 5;
	menuRect.left += 22;
	menuRect.bottom = menuRect.top + 25;
	menuRect.right = menuRect.left + 16;
	
	BMenu *toMenu = new BMenu("To:");
	BMenu *ccMenu = new BMenu("Cc:");
	BMenu *bccMenu = new BMenu("Bcc:");
	BQuery query;
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);
	query.SetVolume(&volume);
	
	query.PushAttr("META:email");
	query.PushString("*");
	query.PushOp(B_EQ);
	query.PushAttr("META:email2");
	query.PushString("*");
	query.PushOp(B_EQ);
	query.PushOp(B_OR);
	
	if(!fReadOnly && query.Fetch() == B_OK)
	{
		BString addr1,addr2,name;
		entry_ref ref;
		MenuUtils utils;
	
		while(query.GetNextRef(&ref) == B_OK)
		{
			BNode node(&ref);
			if(node.InitCheck() != B_OK)
				continue;
			if(node.ReadAttrString("META:name",&name) != B_OK)
				continue;
			node.ReadAttrString("META:email",&addr1);
			node.ReadAttrString("META:email2",&addr2);
			if(addr1.Length() > 0)
			{
				fAddrList.AddItem(strdup(addr1.String()));
				BString title = name;
				title << " <" << addr1 << ">";
				BMessage *msg = new BMessage(M_ADDR_MSG);
				msg->AddString("email",addr1);
				msg->AddPointer("pointer",fTo);
				utils.AddMenuItem(toMenu,title.String(),msg,this,Window(),0,0
							,rutils.GetBitmapResource('BBMP',"Person"));
				msg = new BMessage(M_ADDR_MSG);
				msg->AddString("email",addr1);
				msg->AddPointer("pointer",fCc);
				utils.AddMenuItem(ccMenu,title.String(),msg,this,Window(),0,0
							,rutils.GetBitmapResource('BBMP',"Person"));
				msg = new BMessage(M_ADDR_MSG);
				msg->AddString("email",addr1);
				msg->AddPointer("pointer",fBcc);
				utils.AddMenuItem(bccMenu,title.String(),msg,this,Window(),0,0
							,rutils.GetBitmapResource('BBMP',"Person"));
			}
			if(addr2.Length() > 0)
			{
				fAddrList.AddItem(strdup(addr2.String()));
				BString title = name;
				title << " <" << addr2 << ">";
				BMessage *msg = new BMessage(M_ADDR_MSG);
				msg->AddPointer("pointer",fTo);
				msg->AddString("email",addr2);
				utils.AddMenuItem(toMenu,title.String(),msg,this,Window(),0,0
							,rutils.GetBitmapResource('BBMP',"Person"));
				msg = new BMessage(M_ADDR_MSG);
				msg->AddPointer("pointer",fCc);
				msg->AddString("email",addr2);
				utils.AddMenuItem(ccMenu,title.String(),msg,this,Window(),0,0
							,rutils.GetBitmapResource('BBMP',"Person"));
				msg = new BMessage(M_ADDR_MSG);
				msg->AddString("email",addr2);
				msg->AddPointer("pointer",fBcc);
				utils.AddMenuItem(bccMenu,title.String(),msg,this,Window(),0,0
							,rutils.GetBitmapResource('BBMP',"Person"));
			}
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
	rect.bottom = rect.top + 16;
	ArrowButton *arrow = new ArrowButton(rect,"addr_arrow"
										,new BMessage(M_EXPAND_ADDRESS));
	AddChild(arrow);
	
	
	menuRect.OffsetBy(0,25*2);
	field = new BMenuField(menuRect,"CcMenu","",ccMenu,
							B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW);
	field->SetDivider(0);
	field->SetEnabled(!fReadOnly);
	AddChild(field);
	
	
	menuRect.OffsetBy(0,25);
	field = new BMenuField(menuRect,"BccMenu","",bccMenu,
							B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW);
	field->SetDivider(0);
	field->SetEnabled(!fReadOnly);
	AddChild(field);
	
	BMenu *fromMenu = new BMenu("From:");
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
		if((err = dir.GetNextEntry(&entry)) == B_OK )
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
	}else
		(new BAlert("","Could not find mail accounts","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_INFO_ALERT))->Go();
		
	fromMenu->SetRadioMode(true);
	
	menuRect.OffsetBy(0,25);
	field = new BMenuField(menuRect,"FromMenu","",fromMenu,
							B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW);
	field->SetDivider(0);
	
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
	default:
		BView::MessageReceived(message);
	}
}

/***********************************************************
 *
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
		BString name,reply,pop_host,pop_user;
		
		if(msg.FindString("pop_host",&pop_host) != B_OK)
			pop_host = "";
		if(msg.FindString("pop_user",&pop_user) != B_OK)
			pop_user = "";
		if(msg.FindString("reply_to",&reply) != B_OK)
			reply = "";
		if(msg.FindString("real_name",&name) != B_OK)
			name = "";
		
		BString from;
		if(name.Length() > 0)
			from << "\"" <<name << "\" <";
		if(reply.Length() > 0)
			from << reply;
		else
			from << pop_user << "@" << pop_host;
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
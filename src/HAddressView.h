#ifndef __HAddressView_H__
#define __HAddressView_H__

#include <View.h>
#include <TextControl.h>
#include <List.h>

enum{
	M_MODIFIED 	= 'SUMD',
	M_ADDR_MSG	='TOMS',
	M_ACCOUNT_CHANGE = 'MACC',
	M_EXPAND_ADDRESS = 'MeXA',
	M_SEL_GROUP = 'mSEG'
};

//! Address view for compose window.
class HAddressView: public BView {
public:
		//! Constructor
					HAddressView(BRect rect //!View frame rectangle.
								,bool readOnly = false //! All TextControls is readonly or not.
								);
		//! Destructor
					~HAddressView();
		//! Set all TetxtControl as readonly.
			void	SetReadOnly(bool enable);
		//! Initialize all GUI.
			void	InitGUI();
		//! Find mail address from person files for auto complete.
			void	FindAddress(const char* addr,BMessage &outlist);
		//! Returns Subject field text.
	const char*		Subject() {return fSubject->Text();}
		//! Returns To field text.
	const char*		To() {return fTo->Text();}
		//! Returns CC field text.
	const char*		Cc() {return fCc->Text();}
		//! Returns From field text.
	const char*		From() {return fFrom->Text();}
		//! Returns BCC field text.
	const char*		Bcc() {return fBcc->Text();}
		//! Returns current account name.
	const char*		AccountName();
		//! Set jump hidden controls.
		void		EnableJump(bool enable);
		//! Returns address list got from person files.
		BList&		AddressList() {return fAddrList;}
		//! Returns current focused TextControl.
	BTextControl*	FocusedView() const;
		//! Set account from "From" address.
		void		SetFrom(const char* from/*<Address to guess account.*/);
		//!	Change current account by account name.
		void		ChangeAccount(const char* name/*<Account name*/);
protected:
		//! Override function.
			void	MessageReceived(BMessage *message);
		//! Add person menu item to menu field.
			void	AddPerson(BMenu *menu //< Menu to be added menu items.
							,const char* title //< MenuItem title.
							,const char* group //< Person group.
							,BMessage *msg		//< MenuItem message.
							, char shortcut = 0 //< MenuItem shortcut.
							, uint32 modifiers = 0 //<Shortcut modifier key.
							);
		//! Add mail address to the E-mail address list.
			void	AddPersonToList(BList &list,const char* email,const char* group);
		//! Sort people by name.
	static	int		SortPeople(const void* data1,const void* data2);	

private:
	BTextControl	*fSubject;
	BTextControl	*fTo;
	BTextControl	*fCc;
	BTextControl	*fBcc;
	BTextControl	*fFrom;
	//! E-mail address list for auto complete.
	BList			fAddrList;
	//! All TextControl are readonly or not.
	bool			fReadOnly;
};
#endif
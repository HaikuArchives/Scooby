#ifndef __HAddressView_H__
#define __HAddressView_H__

#include <View.h>
#include <TextControl.h>
#include <List.h>

class BMenu;

enum{
	M_MODIFIED 	= 'SUMD',
	M_ADDR_MSG	='TOMS',
	M_ACCOUNT_CHANGE = 'MACC',
	M_EXPAND_ADDRESS = 'MeXA',
	M_SEL_GROUP = 'mSEG'
};

typedef struct PersonData{
	char* group;
	char* email;
};

class HAddressView: public BView {
public:
					HAddressView(BRect rect,bool readOnly = false);
	virtual			~HAddressView();
			void	SetReadOnly(bool enable);
			void	InitGUI();
			void	FindAddress(const char* addr,BMessage &outlist);
	const char*		Subject() {return fSubject->Text();}
	const char*		To() {return fTo->Text();}
	const char*		Cc() {return fCc->Text();}
	const char*		Bcc() {return fBcc->Text();}
	const char*		AccountName();
	const char*		From() {return fFrom->Text();}
		
		void		EnableJump(bool enable);
		BList&		AddressList() {return fAddrList;}
	BTextControl*	FocusedView() const;
	
		void		SetFrom(const char* from);
protected:
	virtual	void	MessageReceived(BMessage *message);
			
			void	ChangeAccount(const char* name);

			void	AddPerson(BMenu *menu,const char* title,const char* group,BMessage *msg
							, char shortcut = 0
							, uint32 modifiers = 0);
			void	AddPersonToList(BList &list,const char* email,const char* group);
	static	int		SortPeople(const void* data1,const void* data2);	

private:
	BTextControl	*fSubject;
	BTextControl	*fTo;
	BTextControl	*fCc;
	BTextControl	*fBcc;
	BTextControl	*fFrom;
	BList			fAddrList;
	bool			fReadOnly;
};
#endif

#ifndef __HCLIENTVIEW_H__
#define __HCLIENTVIEW_H__

#include "HProgressBarView.h"
#include <String.h>
#include <Message.h>

class PopLooper;

enum{
	M_POP_ABORT = 'mPaB'
};

class HPopClientView :public HProgressBarView {
public:
					HPopClientView(BRect rect
								,const char* name);
			 		~HPopClientView();
			void	PopConnect(const char* name,
							const char* addr,int16 port,
							const char* login,const char* pass);
			bool	IsRunning() const {return fIsRunning;}
			
			void	Cancel();
			
			void	FilterMail(const char* subject,
							const char* from,
							const char* to,
							const char* cc,
							const char* reply,
							const char* account,
							BString &outpath);

			void	SaveMail(const char* content,
							entry_ref *folder_ref,
							bool *is_delete);
			int16	RetrieveType() const {return fRetrieve;}
protected:
	//@{
	//!Override function.
			void	MessageReceived(BMessage *message);
			void	MouseDown(BPoint point);
	//@}
			bool	Filter(const char* key,
							int32 operation,
							const char* value);
			
			void	SetNextRecvPos(const char* uidl);
			
			void	PlayNotifySound();

			int32 	GetHeaderParam(BString &out,
									const char* content,
									int32 offset);
private:
	PopLooper		*fPopLooper;
	BString			fLogin;
	BString			fPassword;
	int32			fStartPos;
	int32			fStartSize;
	BString			fUidl;
	int16			fRetrieve;
	bool			fIsRunning;
	BMessage		*fPopServers;
	int32			fServerIndex;
	BString			fAccountName;
	int32			fLastSize;
	int32			fDeleteDays;
	BMessage		fDeleteMails;
	bool			fCanUseUIDL;
	bool			fUseAPOP;
	bool			fGotMails;
	int32			fMailCurrentIndex;
	int32			fMailMaxIndex;
	
	typedef		HProgressBarView	_inherited;
};
#endif


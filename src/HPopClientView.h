#ifndef __HCLIENTVIEW_H__
#define __HCLIENTVIEW_H__

#include <View.h>
#include <String.h>
#include <StringView.h>
#include <Message.h>

class PopClient;

class HPopClientView :public BView {
public:
					HPopClientView(BRect rect
								,const char* name);
	virtual 		~HPopClientView();
			void	PopConnect(const char* name,
							const char* addr,int16 port,
							const char* login,const char* pass);
			
			void	StartBarberPole();
			void	StopBarberPole();
			
			void	StartProgress() {fShowingProgress = true;}
			void	StopProgress() {fShowingProgress = false;}
			
			
			void	Update(float delta);
			void	SetValue(float value);
			void	SetMaxValue(float max) { fMaxValue = max;}
			
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
	virtual void	MessageReceived(BMessage *message);
	virtual void	Draw(BRect updateRect);
	virtual	void	Pulse();

			BRect	BarberPoleInnerRect() const;	
			BRect	BarberPoleOuterRect() const;
			bool	Filter(const char* key,
							int32 operation,
							const char* value);
			time_t	MakeTime_t(const char* date);
			
			void	SetNextRecvPos(const char* uidl);
			
			void	PlayNotifySound();

			int32 	GetHeaderParam(BString &out,
									const char* content,
									int32 offset);
private:
	BStringView		*fStringView;
	int32			fLastBarberPoleOffset;
	bool 			fShowingBarberPole;
	bool			fShowingProgress;
	BBitmap			*fBarberPoleBits;
	float			fMaxValue;
	float			fCurrentValue;
	PopClient		*fPopClient;
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
};
#endif
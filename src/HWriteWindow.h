#ifndef __HWRITEWINDOW_H__
#define __HWRITEWINDOW_H__

#include <Window.h>
#include <FilePanel.h>
#include <File.h>

class HAddressView;
class HEnclosureView;
class HMailView;
class HMailItem;

enum{
	M_CREATE_MAIL = 'mCRE',
	M_SEND_NOW = 'SENN',
	M_SEND_LATER = 'SLAT',
	M_ADD_SIGNATURE = 'MADS',
	M_ADD_ENCLOSURE = 'MAEN',
	M_DEL_ENCLOSURE = 'MDEE',
	M_OPEN_DRAFT = 'MOdf',
	M_SAVE_DRAFT = 'MSAd',
	M_OPEN_TEMPLATE = 'MOTM',
	M_SAVE_TEMPLATE = 'MSTM',
	M_EDIT_DRAFTS = 'mEDF',
	M_EDIT_TEMPLATES = 'mETM'
};



class HWriteWindow :public BWindow{
public:
					HWriteWindow(BRect rect,const char* name,
								const char* subject = NULL,
								const char* to = NULL,
								HMailItem *replyItem = NULL,
								bool reply = false,
								bool forward = false);			
protected:
	virtual			~HWriteWindow();
	virtual	void	MessageReceived(BMessage *message);
	virtual bool	QuitRequested();
	virtual void	DispatchMessage(BMessage *message,BHandler *handler);
	virtual void	MenusBeginning();
			void	InitMenu();
			void	InitGUI();
		status_t	SaveMail(bool send_now,entry_ref &ref,bool multipart);
			char*	TimeZoneOffset(time_t *now);
			void	WriteAllPart(BString &str,const char *boundary);
			
			void	WriteReplyStatus();
			
			void	OpenTemplate(entry_ref ref);
			void	SaveAsTemplate();
			
			void	OpenDraft(entry_ref ref);
			void	SaveAsDraft();
			void	RemoveDraft();
			
			void	AddChildItem(BMenu *menu,
								const char* path,
								int32 what,
								bool mod = false);
			void	FindCharset(int32 conversion,BString &charset);
			
			bool	IsHardWrap();
			
			void	GetDraftsPath(BPath &path);
			void	GetTemplatesPath(BPath &path);
			
			void	NodeMonitor(BMessage *message);
			
			void	AddNewChildItem(entry_ref &ref);
			void	RemoveChildItem(node_ref &nref);
private:
	HAddressView	*fTopView;
	HMailView		*fTextView;
	HEnclosureView	*fEnclosureView;
	BFilePanel		*fFilePanel;
	bool			fReply;
	bool			fForward;
	HMailItem		*fReplyItem;
	BFile			*fReplyFile;
	BEntry			*fDraftEntry;
	bool			fSkipComplete;
	bool			fSent;
};
#endif
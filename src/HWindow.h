#ifndef __H_WINDOW_H__
#define __H_WINDOW_H__

#include <Window.h>
#include <FilePanel.h>

#define TRASH_FOLDER	"Trash"

class HMailView;
class HMailItem;
class HMailCaption;
class HMailList;
class HFolderList;
class SplitPane;
class HStatusView;
class HDetailView;
class HPopClientView;
class HSmtpClientView;
class HDeskbarView;
class HHtmlMailView;

enum{
	DESKBAR_NORMAL_ICON = 0,
	DESKBAR_NEW_ICON,
};

enum{
	M_LAUNCH_MSG = 'HWLM',
	M_NEW_DIRECOTRY = 'HWND',
	M_DELETE_FILE = 'HWDF',
	M_DELETE_DIR = 'HWDD',
	M_RENAME_FILE = 'HWRF',
	M_ATTR_MSG = 'MATT',
	M_NEW_MSG = 'NEWM',
	M_OPEN_QUERY = 'MOPQ',
	M_POP_CONNECT = 'pOCO',
	M_PREF_MSG = 'MPRE',
	M_EMPTY_TRASH = 'emTr',
	M_REPLY_MESSAGE = 'repM',
	M_FORWARD_MESSAGE= 'forM',
	M_CHECK_FROM = 'chFM',
	M_PRINT_MESSAGE = 'mpRI',
	M_PAGE_SETUP_MESSAGE = 'mPAG',
	M_SEND_PENDING_MAILS = 'mSPM',
	M_POP_CHECK = 'pCHC',
	M_ADD_MAIL_TO_LIST = 'mAdL',
	M_STOP_MAIL_BARBER_POLE = 'mSoB',
	M_FILTER_MAIL = 'mFiM',
	M_INVALIDATE_MAIL = 'mIVm',
	M_DELETE_FOLDER = 'mDEF',
	M_CREATE_FOLDER_DIALOG = 'mCED',
	M_IMPORT_PLAIN_TEXT_MAIL = 'mPTM',
	M_CONVERT_PLAIN_TO_MAIL = 'mCVP',
	M_IMPORT_MBOX = 'mMBX',
	M_CONVERT_MBOX_TO_MAILS = 'mCMm',
	M_ADD_TO_BLACK_LIST = 'mAtB',
	M_CHANGE_MAIL_STATUS = 'mChM'
};

class HWindow: public BWindow {
public:
						HWindow(BRect rect,
								const char* name);


				int32	FolderSelection();
				
				void	MakeWriteWindow(const char* subject = NULL,
									const char* to = NULL,
									const char* cc = NULL,
									const char* bcc = NULL,
									const char* body = NULL,
									const char* enclosure = NULL,
									HMailItem *replyItem = NULL,
									bool reply = false,
									bool forward = false);
									
				void	MakeReadWindow(entry_ref ref,BMessenger *messenger = NULL);
				void	RefsReceived(BMessage *message);
				
				int32	CurrentDeskbarIcon()const {return fCurrentDeskbarIcon;}
				void	ChangeDeskbarIcon(int32 icon) {fCurrentDeskbarIcon = icon;}

				void	ShowOpenPanel(int32 what);
				
		HFolderList*	FolderList() {return fFolderList;}
		HMailList*		MailList() {return fMailList;}
protected:		
		virtual			~HWindow();

		virtual void	MessageReceived(BMessage *message);
		virtual bool	QuitRequested();
		virtual void	MenusBeginning();
		virtual void	DispatchMessage(BMessage *message,BHandler *handler);
				void	InitGUI();
				void	InitMenu();
				
				void	PopConnect();
				void	CheckFrom(entry_ref ref);
				void	SendPendingMails();
					
				void	AddToPeople();
				
				void	MoveMails(BMessage *message);
				void	DeleteMails();
				
				void	ReplyMail(HMailItem *item,bool reply_all = false);
				void	ForwardMail(HMailItem *item);
				
				void	InvokeMailItem();
				
			status_t	AddPopServer(entry_ref ref,BMessage &sendMsg);
			
				void	InstallToDeskbar();
				void	RemoveFromDeskbar();
			
				void	AddCheckFromItems();
				void	EmptyTrash();
				
				void	FilterMails(HMailItem *item);
				void	DeleteFolder(int32 sel);
				
				void	PrintMessage(BMessage *message);
				
				void	Plain2BeMail(const char* path);
				void	MBox2BeMail(const char* path);
				
				void	AddToBlackList(int32 sel);
				
private:
		HMailList*		fMailList;
		HMailCaption*	fMailCaption;
		HFolderList*    fFolderList;
		BView*			fMailView;
		SplitPane*		fHSplitter;
		SplitPane*      fVSplitter;
		HStatusView*	fStatusView;
		HDetailView*	fDetailView;
		HPopClientView*	fPopClientView;
		time_t			fCheckIdleTime;
		HSmtpClientView* fSmtpClientView;
		int32			fCurrentDeskbarIcon;
		BFilePanel		*fOpenPanel;
};
#endif		
				

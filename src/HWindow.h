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
class LEDAnimation;

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
	M_CHANGE_MAIL_STATUS = 'mChM',
	M_CREATE_FILTER = 'mCFt'
};

//! Main window.
class HWindow: public BWindow {
public:
					//!Constructor.
						HWindow(BRect rect,
								const char* name);
				
				void Show();

					//!Returns folder selection index.
				int32	FolderSelection();
					//!Make compose window and show.
				void	MakeWriteWindow(const char* subject = NULL,
									const char* to = NULL,
									const char* cc = NULL,
									const char* bcc = NULL,
									const char* body = NULL,
									const char* enclosure = NULL,
									HMailItem *replyItem = NULL,
									bool reply = false,
									bool forward = false);
					//!Make read window and show.					
				void	MakeReadWindow(entry_ref ref,BMessenger *messenger = NULL);
					//!Handle B_REFS_RECEIVED message.
				void	RefsReceived(BMessage *message);
					//!Start playing keyboard LED animation.
				void	PlayLEDAnimaiton();
					//!Returns current deskbar icon state.
				int32	CurrentDeskbarIcon()const {return fCurrentDeskbarIcon;}
					//!Store current deskbar icon state.
				void	ChangeDeskbarIcon(int32 icon) {fCurrentDeskbarIcon = icon;}
					//!Show open file panel.
				void	ShowOpenPanel(int32 what);
					//!Returns folder list pointer.
		HFolderList*	FolderList() {return fFolderList;}
					//!Returns mail list pointer.
		HMailList*		MailList() {return fMailList;}
protected:		
				//!Destructor.
						~HWindow();
		//@{
		//!Override function.
			 	void	MessageReceived(BMessage *message);
			 	bool	QuitRequested();
			 	void	MenusBeginning();
			 	void	DispatchMessage(BMessage *message,BHandler *handler);
		//@}
				//!Initialize all GUI.
				void	InitGUI();
				//!Initialize menubar.
				void	InitMenu();
				//!Connect and fetch mail from all accounts.
				void	PopConnect();
				//!Connect and fetch mail from selected account.
				void	CheckFrom(entry_ref &ref/*!<Account file's entry_ref*/);
				//!Send mails which have "Pending" status.
				void	SendPendingMails();
				//!Add new person file.
				void	AddToPeople();
				//!Move mails to another folder.
				void	MoveMails(BMessage *message);
				//!Move mail to trash folder.
				void	DeleteMails();
				//!Create reply mails.
				void	ReplyMail(HMailItem *item,bool reply_all = false);
				//!Create forword mails.
				void	ForwardMail(HMailItem *item);
				//!This function will be call when mail list item was double clicked.
				void	InvokeMailItem();
				//!Add one POP3 server to check target.
			status_t	AddPopServer(entry_ref &ref,BMessage &sendMsg);
				//!Install the deskbar icon.
				void	InstallToDeskbar();
				//!Remove the deskar icon.
				void	RemoveFromDeskbar();
				//!Rebuild Check from menu.
				void	AddCheckFromItems();
				//!Empty mail trash folder.
				void	EmptyTrash();
				//!Move mail with filter.
				void	FilterMails(HMailItem *item);
				//!Delete folder.
				void	DeleteFolder(int32 sel);
				//!Print mail message.
				void	PrintMessage(BMessage *message);
				//!Convert plain text file to BeOS style E-mail file.
				void	Plain2BeMail(const char* path);
				//!Convert mbox format file to BeOS style E-mail files.
				void	MBox2BeMail(const char* path);
				//!Add selected mail address to blacklist.
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
		LEDAnimation	*fLEDAnimation;
};
#endif		

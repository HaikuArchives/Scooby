#ifndef __H_WINDOW_H__
#define __H_WINDOW_H__

#include <Window.h>

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
	M_REMOVE_MAIL_FROM_LIST = 'mRmL'
};

class HWindow: public BWindow {
public:
						HWindow(BRect rect,
								const char* name,
								const char* mail_addr=NULL);


				int32	FolderSelection();
				
				void	MakeWriteWindow(const char* subject = NULL,
									const char* to = NULL,
									HMailItem *replyItem = NULL,
									bool reply = false,
									bool forward = false);
									
				void	MakeReadWindow(entry_ref ref,BMessenger *messenger = NULL);
				void	RefsReceived(BMessage *message);
				
				int32	CurrentDeskbarIcon()const {return fCurrentDeskbarIcon;}
				void	ChangeDeskbarIcon(int32 icon) {fCurrentDeskbarIcon = icon;}
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
				void	MoveFile(entry_ref file_ref,const char* dest_dir);
				void	ReplyMail(HMailItem *item,bool reply_all = false);
				void	ForwardMail(HMailItem *item);
				
				void	InvokeMailItem();
				
			status_t	AddPopServer(entry_ref ref,BMessage &sendMsg);
			
				void	InstallToDeskbar();
				void	RemoveFromDeskbar();
			
				void	AddCheckFromItems();
				void	EmptyTrash();
private:
		HMailList*		fMailList;
		HMailCaption*	fMailCaption;
		HFolderList*    fFolderList;
		HMailView*		fMailView;
		SplitPane*		fHSplitter;
		SplitPane*      fVSplitter;
		HStatusView*	fStatusView;
		HDetailView*	fDetailView;
		HPopClientView*	fPopClientView;
		time_t			fCheckIdleTime;
		HSmtpClientView* fSmtpClientView;
		int32			fCurrentDeskbarIcon;
};
#endif		
				

#ifndef __HHTMLMAILVIEW_H__
#define __HHTMLMAILVIEW_H__

#include <View.h>
#include <ScrollView.h>
#include <ListView.h>
#include <String.h>
#include <FilePanel.h>

class HHtmlView;
class CTextView;
class HAttachmentList;
class MySavePanel;

class HHtmlMailView :public BView{
public:
					HHtmlMailView(BRect rect,const char* name,
									uint32 reisze = B_FOLLOW_LEFT|B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
					~HHtmlMailView();

			void	LoadMessage(BFile* file);
			void	SetContent(BFile *file);
			void SetEncoding(const char* encoding);
			bool	IsShowingHeader() {return false;}
			bool	IsShowingRawMessage() {return false;}
protected:
			void	MessageReceived(BMessage *message);
			void	Plain2Html(BString &content,const char* encoding,const char* transfer_encoding);
			void	ClearList();
			void	ParseAllParts(const char* content,const char* boundary,int32 header_len);
			void	AddPart(const char* part,int32 file_offset);
			
			void	OpenAttachment(int32 i);
			void	SaveAttachment(int32 i,entry_ref dir,const char* name,bool rename = true);
			bool	IsURI(char c);
			
			void	ResetScrollBar();
			void	ResetAttachmentList();
			
			void	ConvertLinkToBlankWindow(BString &str);
			void	ConvertToHtmlCharactor(char c,char* out,bool *translate_space);
private:
	HHtmlView*		fHtmlView;
	CTextView*		fHeaderView;
	HAttachmentList*		fAttachmentList;
	BFile*			fFile;
	MySavePanel*	fFilePanel;
};

class MySavePanel :public BFilePanel {
public:
						MySavePanel(BView *view);
						~MySavePanel();
protected:
	 	 void	SendMessage(const BMessenger*, BMessage*);
private:
	BView*			fTargetView;
};
#endif
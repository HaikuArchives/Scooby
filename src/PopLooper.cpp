#include "PopLooper.h"
#include "PopClient.h"
#include "Encoding.h"
#include "md5.h"
#include "HApp.h"
#include "HFile.h"
#include "HPopClientView.h"
#include "TrackerString.h"

#include <Debug.h>
#include <String.h>
#include <stdlib.h>
#include <E-mail.h>
#include <File.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Message.h>
#include <Autolock.h>
#include <Alert.h>
#include <FindDirectory.h>


#define xEOF    236
#define CRLF "\r\n"
#define MAX_RECIEVE_BUF_SIZE 1024000


const bigtime_t kTimeout = 1000000*180; //timeout 180 secs
	
/***********************************************************
 * Constructor
 ***********************************************************/
PopLooper::PopLooper(BHandler *handler,BLooper *looper)
	:BLooper("PopLooper")
	,fPopClient(NULL)
	,fHandler(handler)
	,fLooper(looper)
{
	Run();
	fBlackList.MakeEmpty();
}

/***********************************************************
 * Destructor
 ***********************************************************/
PopLooper::~PopLooper()
{
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
PopLooper::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	// Stat
	case H_STAT_MESSAGE:
		{
			int32 mails,bytes;
			if(fPopClient->Stat(&mails,&bytes) != B_OK)
				PostError(fPopClient->Log());
			else{
				message->AddInt32("mails",mails);
				message->AddInt32("bytes",bytes);
				fLooper->PostMessage(message,fHandler);	
			}
			break;
		}
	// LIST
	case H_LIST_MESSAGE:
		{
			BString list;
			int32 index;
			if(message->FindInt32("index",&index) != B_OK)
				index = 0;
			if(fPopClient->List(index,list) != B_OK)
				PostError(fPopClient->Log());
			else{
				message->AddString("list",list);
				fLooper->PostMessage(message,fHandler);	
			}
			break;
		}
	// Retr
	case H_RETR_MESSAGE:
		{
			int32 count;
			type_code type;
			message->GetInfo("index",&type,&count);
		
			int32 index;
			BString content;
			for(int32 i = 0;i < count;i++)
			{
				if(message->FindInt32("index",i,&index) != B_OK)
					continue;
				if( Retr(index,content) != B_OK)
				{
					PostError(fLog.String());
					break;	
				}else{
					BMessage msg(H_RETR_MESSAGE);
					msg.AddString("content",content);
					msg.AddInt32("index",index);
					msg.AddBool("end",(i == count-1)?true:false);
					fLooper->PostMessage(&msg,fHandler);
				}
				
			}
			break;
		}
	// Del
	case H_DELETE_MESSAGE:
		{
			int32 count;
			type_code type;
			message->GetInfo("index",&type,&count);
			int32 index;
			
			while(count>0)
			{
				if(message->FindInt32("index",--count,&index) == B_OK)
				{
					if( fPopClient->Delete(index) != B_OK)
						PostError(fPopClient->Log());
					else{
						BMessage msg(H_DELETE_MESSAGE);
						msg.AddInt32("index",index);
						msg.AddBool("end",(count == 0)?true:false);
						fLooper->PostMessage(&msg,fHandler);
					}
				}
			}
			break;
		}	
	// Last
	case H_LAST_MESSAGE:
		{
			int32 index;
			
			if( fPopClient->Last(&index) != B_OK)
					PostError(fPopClient->Log());
			else{
				message->AddInt32("index",index);
				fLooper->PostMessage(message,fHandler);
			}
			break;
		}
	// login
	case H_LOGIN_MESSAGE:
		{
			const char* login;
			const char* password;
			bool apop;
			if(message->FindString("login",&login) == B_OK &&
				message->FindString("password",&password) == B_OK &&
				message->FindBool("apop",&apop) == B_OK)
			{
				if(fPopClient->Login(login,password,apop) != B_OK)
				{
					PostError(fPopClient->Log());
				}else
					fLooper->PostMessage(message,fHandler);
			}
			break;
		}
	// connect
	case H_CONNECT_MESSAGE:
		{
			fPopClient = new PopClient();
			
			const char *addr;
			int16 port;
			if(message->FindString("address",&addr) == B_OK &&
				message->FindInt16("port",&port) == B_OK)
			{
				if(fPopClient->Connect(addr,port) != B_OK)
				{
					PostError(fPopClient->Log());
				}else
					fLooper->PostMessage(message,fHandler);
			}
			break;
		}
	// reset
	case H_RESET_MESSAGE:
	{
		if( fPopClient->Rset() != B_OK)
			PostError(fPopClient->Log());
		else
			fLooper->PostMessage(message,fHandler);
		break;
	}
	// uidl
	case H_UIDL_MESSAGE:
	{
		BString list;
		int32 index;
		if(message->FindInt32("index",&index) != B_OK)
			index = 0;
		if(fPopClient->Uidl(index,list) != B_OK)
		{
			fLooper->PostMessage(message,fHandler);
		}else{
			message->AddString("list",list);
			fLooper->PostMessage(message,fHandler);	
		}
		break;
	}
	default:
		BLooper::MessageReceived(message);
	}
}

/***********************************************************
 * PostError
 ***********************************************************/
void
PopLooper::PostError(const char* log)
{
	BMessage msg(H_ERROR_MESSAGE);
	msg.AddString("log",log);
	fLooper->PostMessage(&msg,fHandler);
}

/***********************************************************
 * Retr
 ***********************************************************/
status_t
PopLooper::Retr(int32 index,BString &content)
{
	int32 size = 0;
	BString size_list;
	
	content = "";
	PRINT(("BLACKLIST COUNT:%d\n",fBlackListCount));
	// Spam filter
	if(index != 0 && fBlackListCount > 0)
	{
		BString topOutput;
		if(fPopClient->Top(index,0,topOutput) != B_OK)
			PRINT(("%s\n",fLog.String() ));	
		else{
			if( IsSpam(topOutput.String()) )
			{
				if(((HPopClientView*)fHandler)->RetrieveType() > 0)
					fPopClient->Delete(index);
				return B_OK;
			}
		}
	}
	// Get mail content size 
	if(index != 0)
	{	
		if(fPopClient->List(index,size_list) != B_OK) {
			PRINT(("%s\n",fLog.String() ));
			size = 1;
		}else{
			int32 i = size_list.FindLast(" ");
			size = atol(&size_list[++i]);
		}
	}
	// Send Retr command
	BString cmd = "RETR ";
	cmd << index << CRLF;
	if( fPopClient->SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("%s\n",cmd.String() ));
		PRINT(("Retr error:%s %d\n",__FILE__,__LINE__));
		return B_ERROR;
	}
	
	// Get mail content
	int32 r;
	
	size += 3;
	BMessage msg(H_SET_MAX_SIZE);
	msg.AddInt32("max_size",size);
	fLooper->PostMessage(&msg,fHandler);
	msg.what = H_RECEIVING_MESSAGE;
	msg.AddInt32("index",index);
	msg.AddInt32("size",0);
	
	char *buf = new char[MAX_RECIEVE_BUF_SIZE+1];
	if(!buf)
	{
		(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return B_ERROR;
	}
	int32 content_len = 0;
	//PRINT(("buf_size:%d\n",buf_size));
	while(1)
	{
		if(fPopClient->IsDataPending(kTimeout))
		{
			r = fPopClient->Receive(buf,MAX_RECIEVE_BUF_SIZE);
			if(r <= 0)
			{
				PRINT(("Receive Err:%d %d %s\n",r,size,buf));
				return B_ERROR;
			}
			size -= r;
			content_len += r;
			buf[r] = '\0';
			content += buf;
			msg.ReplaceInt32("size",r);
			fLooper->PostMessage(&msg,fHandler);
			
			if(content_len > 5 &&
			        content[content_len-1] == '\n' && 
					content[content_len-2] == '\r' &&
					content[content_len-3] == '.'  &&
					content[content_len-4] == '\n' &&
					content[content_len-5] == '\r' )
				break;
		}
	}
	delete[] buf;
	content.Truncate(content.Length()-5);
	content.ReplaceAll("\n..","\n.");
	return B_OK;
}

/***********************************************************
 * IsSpam
 ***********************************************************/
bool
PopLooper::IsSpam(const char* header)
{
	using namespace BPrivate;
	
	TrackerString from("");
	// Find from field
	char *p;
	
	if((p = ::strstr(header,"\nFrom:")) )
	{
		p+=6;
		if(*p == ' ')
			p++;
		while(1)
		{
			if(*p == '<')
			{
				p++;
				if(*p != ' ')
				{
					from = "";
					from += *p;
				}
			}else{
				if(*p != ' ')
					from += *p;
			}
			p++;
			if(*p == '\r' || *p == '\n' || *p == '>')
				break;
			if(*p == ' ' && from.FindFirst("@") != B_ERROR)
				break;
		}
	}else
		return false;
	PRINT(("From:%s\n",from.String()));
	// Compare to blacklist	
	for(int32 i = 0;i < fBlackListCount;i++)
	{
		if(from.Matches((const char*)fBlackList.ItemAt(i),false))
		{
			PRINT(("SPAM:%s\n",(char*)fBlackList.ItemAt(i)));
			return true;
		}
	}
	return false;
}

/***********************************************************
 * InitBlackList
 ***********************************************************/
void
PopLooper::InitBlackList()
{
	int32 count = fBlackList.CountItems();
	// Free old list
	while(count > 0)
		free( (char*)fBlackList.RemoveItem(--count) );
	// Load list
	BPath path;
	::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	path.Append( APP_NAME );
	path.Append("BlackList");
	
	
	HFile file(path.Path(),B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;
	BString line;
	
	while(file.GetLine(&line) >= 0)
	{
		// Strip line feed
		line.RemoveAll("\n");
			
		if(line.Length() > 0)
		{
			// Add address to list
			fBlackList.AddItem(::strdup(line.String()));
		}
	}
	fBlackListCount = fBlackList.CountItems();
}

/***********************************************************
 * ForceQuit
 ***********************************************************/
void
PopLooper::ForceQuit()
{
	if(!fPopClient)
		return;
	int sd = fPopClient->Socket();
#ifndef BONE
	::closesocket(sd);
#else
	::close(sd);
#endif
}

/***********************************************************
 * Quit
 ***********************************************************/
bool
PopLooper::QuitRequested()
{
	if(fPopClient)
	{
		fPopClient->PopQuit();
		delete fPopClient;
	}
	// Free Blacklist
	int32 count = fBlackList.CountItems();
	while(count > 0)
		free( (char*)fBlackList.RemoveItem(--count) );

	fLooper->PostMessage(M_QUIT_FINISHED,fHandler);
	return BLooper::QuitRequested();
}

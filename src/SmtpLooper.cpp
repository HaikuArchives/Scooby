#include "SmtpLooper.h"
#include "HApp.h"
#include "HMailItem.h"
#include "Encoding.h"
#include "Utilities.h"
#include "SmtpClient.h"

#include <Debug.h>
#include <Handler.h>
#include <Message.h>
#include <Entry.h>
#include <File.h>
#include <E-mail.h>
#include <Path.h>
#include <Beep.h>
#include <Alert.h>

#define CRLF "\r\n"
#define xEOF    236

#define SMTP_RESPONSE_SIZE 8192

/***********************************************************
 * Constructor
 ***********************************************************/
SmtpLooper::SmtpLooper(BHandler *handler,BLooper *looper)
	:BLooper()
	,fSmtpClient(NULL)
	,fHandler(handler)
	,fLooper(looper)
{
	Run();
}

/***********************************************************
 * Destructor
 ***********************************************************/
SmtpLooper::~SmtpLooper()
{
}


/***********************************************************
 * MessageReceived
 ***********************************************************/
void
SmtpLooper::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	case M_SMTP_CONNECT:
	{
		HMailItem *item(NULL);
		int32 count;
		type_code type;
		message->GetInfo("pointer",&type,&count);
		for(int32 i = 0;i < count;i++)
		{
			message->FindPointer("pointer",i,(void**)&item);
			SendMail(item);
		}
		break;
	}
	default:
		BLooper::MessageReceived(message);
	}
}

/***********************************************************
 * SendMail
 ***********************************************************/
status_t
SmtpLooper::SendMail(HMailItem *item)
{
	entry_ref ref = item->Ref();
	BFile file(&ref,B_READ_ONLY);
	BPath path(&ref);
	PRINT(("Path:%s\n",path.Path()));
	if(file.InitCheck() == B_OK)
	{
		BString smtp_server("");
		if(ReadNodeAttrString(&file,B_MAIL_ATTR_SMTP_SERVER,&smtp_server) != B_OK)
		{
			PRINT(("ERR:SMTP_SERVER\n"));
			return B_ERROR;
		}
		
		PRINT(("SMTP:%s\n",smtp_server.String()));
		
		delete fSmtpClient;
		fSmtpClient = new SmtpClient();
		if(fSmtpClient->Connect(smtp_server.String()) != B_OK)
		{
			PRINT(("ERR:CONNECT ERROR\n"));
			PostError("Cound not connect to smtp server");
			return B_ERROR;		
		}

		int32 header_len;
		
		file.ReadAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&header_len,sizeof(int32));
		
		off_t size;
		file.GetSize(&size);
		char *buf = new char[size+1];
		if(!buf)
		{
			(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
			return B_ERROR;
		}
		size = file.Read(buf,size);
		buf[size] = '\0';
		// parse header
		BString from,to,cc,bcc;
		ReadNodeAttrString(&file,B_MAIL_ATTR_FROM,&from);
		ReadNodeAttrString(&file,B_MAIL_ATTR_TO,&to);
		ReadNodeAttrString(&file,B_MAIL_ATTR_CC,&cc);
		ReadNodeAttrString(&file,B_MAIL_ATTR_BCC,&bcc);
		if(cc.Length() > 0)
			to << "," << cc;
		if(bcc.Length() > 0)
			to << "," << bcc;
		BString attrStatus("");
		if(SendMail(from.String(),to.String(),buf) == B_OK)
			attrStatus = "Sent";
		else{
			attrStatus = "Error";
			beep();
			(new BAlert("","Failed to send mails","OK",NULL,NULL
						,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go(); 
		}
		file.WriteAttrString(B_MAIL_ATTR_STATUS,&attrStatus);
		item->RefreshStatus();	
		delete[] buf;
		fSmtpClient->SmtpQuit();
		fLooper->PostMessage(M_SMTP_END,fHandler);
		PRINT(("Posted\n"));
	}DEBUG_ONLY(
	else {
		PRINT(("INIT ERROR\n"));
	}
	);
	return B_OK;
}


/***********************************************************
 * SendMail
 ***********************************************************/
status_t
SmtpLooper::SendMail(const char* from,
				const char* to,
				const char* content)
{
	// Set mail from
	BString cmd = "MAIL FROM: ";

	SmtpClient::ParseAddress(from,cmd);
	if(fSmtpClient->SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err: mail from\n"));
		PostError(fSmtpClient->Log());
		return B_ERROR;
	}
	// Set rcpt to
	int32 len = strlen(to);
	BString addr("");
	for(int32 i = 0;i < len;i++)
	{
		char c = to[i];
		if(c != ',')
			addr += (char)c;
		if(c == ','||i == len-1)
		{
			if(addr.Length() == 0)
				continue;
			cmd = "RCPT TO: ";
			const char* kText = addr.String();
			
			SmtpClient::ParseAddress(kText,cmd);
			PRINT(("%s\n",cmd.String() ));
			if(fSmtpClient->SendCommand(cmd.String()) != B_OK)
			{
				PRINT(("Err: rcpt\n"));
				PostError(fSmtpClient->Log());
				return B_ERROR;
			}
			addr ="";
		}
	}
	// Data
	cmd = "DATA\r\n";
	if(fSmtpClient->SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err: data\n"));
		PostError(fSmtpClient->Log());
		return B_ERROR;
	}
	// Content
	len = ::strlen(content);
	BString line("");
	BMessage msg(M_SET_MAX_SIZE);
	msg.AddInt32("max_size",len);
	fLooper->PostMessage(&msg,fHandler);
	// send content
	int32 send_len = 0;
	msg.MakeEmpty();
	msg.what = M_SEND_MAIL_SIZE;
	msg.AddInt32("size",send_len);
	for(int32 i = 0;i < len;i++)
	{
		line += content[i];
		if(content[i] == '\n' || i == len-1)
		{
			int32 line_len = line.Length();
			if(line_len == 0 && i == len-1)
				break;
			
			if(line[0] == '.')
			{
				line.Insert('.',1,0);
				line_len = line.Length();
			}
			// if line is not end with carrige return
			if(i == len-1 && line.FindLast( "\r\n" ) == B_ERROR)
			{
				line += "\r\n";
				line_len = line.Length();
			}
			send_len = fSmtpClient->Send(line.String(),line_len);
			msg.ReplaceInt32("size",send_len);
			fLooper->PostMessage(&msg,fHandler);
			line="";
		}
	}

	cmd = ".\r\n";
	if( fSmtpClient->SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err:Send content\n"));
		PostError(fSmtpClient->Log());
		return B_ERROR;
	}
	return B_OK;
}

/***********************************************************
 * PostError
 ***********************************************************/
void
SmtpLooper::PostError(const char* log)
{
	BMessage msg(M_SMTP_ERROR);
	msg.AddString("log",log);
	fLooper->PostMessage(&msg,fHandler);
}

/***********************************************************
 * ForceQuit
 ***********************************************************/
void
SmtpLooper::ForceQuit()
{
	if(!fSmtpClient)
		return;
	int sd = fSmtpClient->Socket();
#ifndef BONE
	::closesocket(sd);
#else
	::close(sd);
#endif
}

/***********************************************************
 * QuitRequested
 ***********************************************************/
bool
SmtpLooper::QuitRequested()
{
	if(fSmtpClient)
	{
		fSmtpClient->SmtpQuit();
		fSmtpClient->Close();
		delete fSmtpClient;
	}
	return BLooper::QuitRequested();
}
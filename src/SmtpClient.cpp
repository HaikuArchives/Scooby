#include "SmtpClient.h"

#include "HMailItem.h"
#include "Encoding.h"

#include <Debug.h>
#include <Handler.h>
#include <Message.h>
#include <Entry.h>
#include <File.h>
#include <E-mail.h>
#include <Path.h>
#include <Beep.h>
#include <Alert.h>

#define CRLF "\r\n";

/***********************************************************
 * Constructor
 ***********************************************************/
SmtpClient::SmtpClient(BHandler *handler,BLooper *looper)
	:BLooper()
	,fEndpoint(NULL)
	,fHandler(handler)
	,fLooper(looper)
{
	Run();
}

/***********************************************************
 * Destructor
 ***********************************************************/
SmtpClient::~SmtpClient()
{
	if(fEndpoint)
		fEndpoint->Close();
	delete fEndpoint;
}


/***********************************************************
 * MessageReceived
 ***********************************************************/
void
SmtpClient::MessageReceived(BMessage *message)
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
 * Connect
 ***********************************************************/
status_t
SmtpClient::Connect(const char* address,int16 port)
{
	if(fEndpoint)
		SmtpQuit();
	fEndpoint = new BNetEndpoint();
	if( fEndpoint->Connect(address,port) != B_OK)
	{
		PRINT(("Err:Unknown host\n"));
		return B_ERROR;	
	}
	
	BString line;
	ReceiveLine(line);
	
	BString cmd = "HELO ";
	cmd << address << CRLF;
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err:%s\n",fLog.String()));
		return B_ERROR;
	}
	PRINT(("SMTP:Connect success\n"));
	return B_OK;
}

/***********************************************************
 * ReadData
 ***********************************************************/
int32
SmtpClient::ReceiveLine(BString &line)
{
	bigtime_t timeout = 1000000*60; //timeout 60 secs
	int32 len = 0,rcv;
	char c = 0;
	line = "";
	if(fEndpoint->IsDataPending(timeout))
	{
		while(c != '\n')
		{
			rcv = fEndpoint->Receive(&c,1);
			if(rcv <=0)
				break;
			len += rcv;
			line << c;
			if(c == '\n')
				break;
		}			
	}
	return len;
}


/***********************************************************
 * Command
 ***********************************************************/
status_t
SmtpClient::SendCommand(const char* cmd)
{
	int32 len;
 	if( fEndpoint->Send(cmd, ::strlen(cmd)) == B_ERROR)
		return B_ERROR;
	fLog = "";
	// Receive
	len = ReceiveLine(fLog);
	
	if(len <= 0)
		return B_ERROR;
	PRINT(("%s\n",fLog.String()));
	char top = fLog[0];
	int32 num = top - '0';
	//PRINT(("ReplyNumber: %d\n",num));
	if(num >= 5)
		return B_ERROR;
	return B_OK;
}

/***********************************************************
 * SendMail
 ***********************************************************/
status_t
SmtpClient::SendMail(HMailItem *item)
{
	entry_ref ref = item->Ref();
	BFile file(&ref,B_READ_ONLY);
	BPath path(&ref);
	PRINT(("Path:%s\n",path.Path()));
	if(file.InitCheck() == B_OK)
	{
		BString smtp_server("");
		if(file.ReadAttrString(B_MAIL_ATTR_SMTP_SERVER,&smtp_server) != B_OK)
		{
			PRINT(("ERR:SMTP_SERVER\n"));
			return B_ERROR;
		}
		
		PRINT(("SMTP:%s\n",smtp_server.String()));
		if(Connect(smtp_server.String()) != B_OK)
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
		size = file.Read(buf,size);
		buf[size] = '\0';
		// parse header
		BString from,to,cc,bcc;
		file.ReadAttrString(B_MAIL_ATTR_FROM,&from);
		file.ReadAttrString(B_MAIL_ATTR_TO,&to);
		file.ReadAttrString(B_MAIL_ATTR_CC,&cc);
		file.ReadAttrString(B_MAIL_ATTR_BCC,&bcc);
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
		SmtpQuit();
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
SmtpClient::SendMail(const char* from,
				const char* to,
				const char* content)
{
	// Set mail from
	BString cmd = "MAIL FROM: ";

	ParseAddress(from,cmd);
	if(SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err: mail from\n"));
		return B_ERROR;
	}
	// Set rcpt to
	int32 len = strlen(to);
	BString addr("");
	for(int32 i = 0;i < len;i++)
	{
		char c = to[i];
		if(c != ',')
			addr << (char)c;
		if(c == ','||i == len-1)
		{
			if(addr.Length() == 0)
				continue;
			cmd = "RCPT TO: ";
			const char* kText = addr.String();
			
			ParseAddress(kText,cmd);
			if(SendCommand(cmd.String()) != B_OK)
			{
				PRINT(("Err: rcpt\n"));
				return B_ERROR;
			}
			addr ="";
		}
	}
	// Data
	cmd = "DATA\r\n";
	if(SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err: data\n"));
		return B_ERROR;
	}
	// Content
	len = ::strlen(content);
	BString line("");
	// send content
	for(int32 i = 0;i < len;i++)
	{
		line << content[i];
		if(content[i] == '\n' || i == len-1)
		{
			int32 line_len = line.Length();
			if(line_len == 0 && i == len-1)
				break;
			
			if(line.Compare(".\r\n") == 0)
			{
				line.Insert('.',1,0);
				line_len = line.Length();
			}
			// if line is not end with carrige return
			if(i == len-1 && line.FindLast( "\r\n" ) == B_ERROR)
			{
				line << "\r\n";
				line_len = line.Length();
			}
			fEndpoint->Send(line.String(),line_len);
			line="";
		}
	}

	cmd = ".\r\n";
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err:Send content\n"));
		return B_ERROR;
	}
	PRINT(("OUT\n"));
	return B_OK;
}

/***********************************************************
 * ParseAddress
 ***********************************************************/
void
SmtpClient::ParseAddress(const char* in,BString& out)
{
	char *p;
	
	if ( (p = strstr(in, "<")) )
	{
		while( p)
		{
			out << (char)*p;
			if(*p++ == '>')
				break;
		}
		out << CRLF;
	}else{
		out << "<" << in << ">"<< CRLF;
	}
}

/***********************************************************
 * PostError
 ***********************************************************/
void
SmtpClient::PostError(const char* log)
{
	BMessage msg(M_SMTP_ERROR);
	msg.AddString("log",log);
	fLooper->PostMessage(&msg,fHandler);
}

/***********************************************************
 * SmtpQuit
 ***********************************************************/
status_t
SmtpClient::SmtpQuit()
{
	// Send Quit
	BString cmd = "QUIT\r\n";
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err:Quit\n"));
		return B_ERROR;
	}
	fEndpoint->Close();
	delete fEndpoint;
	fEndpoint = NULL;
	return B_OK;
}
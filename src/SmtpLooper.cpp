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
#include <FindDirectory.h>

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
	Encoding encode;
	int32 encoding;
	
	if(file.InitCheck() == B_OK)
	{
		BString smtp_server(""),account("");
		bool smtp_auth=false;
	
		if(ReadNodeAttrString(&file,B_MAIL_ATTR_SMTP_SERVER,&smtp_server) != B_OK)
		{
			PRINT(("ERR:SMTP_SERVER\n"));
			fLooper->PostMessage(M_SMTP_END,fHandler);
			return B_ERROR;
		}
		file.ReadAttr(B_MAIL_ATTR_SMTP_AUTH,B_BOOL_TYPE,0,&smtp_auth,sizeof(bool));
		ReadNodeAttrString(&file,B_MAIL_ATTR_ACCOUNT,&account);
		PRINT(("SMTP:%s\n",smtp_server.String()));
		
		delete fSmtpClient;
		fSmtpClient = new SmtpClient();
		if(fSmtpClient->Connect(smtp_server.String(),25,smtp_auth) != B_OK)
		{
			PRINT(("ERR:CONNECT ERROR\n"));
			PostError("Cound not connect to smtp server");
			fLooper->PostMessage(M_SMTP_END,fHandler);
			return B_ERROR;		
		}
		// SMTP auth -------------
		if(smtp_auth)
		{
			BPath path;
			::find_directory(B_USER_SETTINGS_DIRECTORY,&path);
			path.Append(APP_NAME);
			path.Append("Accounts");
			path.Append(account.String());
			
			BFile file;
			BMessage setting;
			if(file.SetTo(path.Path(),B_READ_ONLY) != B_OK)
			{
				Alert(B_STOP_ALERT,_("Account file %s is corrupted"),account.String());
				fLooper->PostMessage(M_SMTP_END,fHandler);
				return B_ERROR;
			}
			setting.Unflatten(&file);
			const char* login,*password;
			setting.FindString("pop_user",&login);
			setting.FindString("pop_password",&password);
			int32 len = strlen(password);
			char *pass = new char[len+1];
			for(int32 k = 0;k < len;k++)
				pass[k] = (char)(255-password[k]);
			pass[len] = '\0';
			status_t err = fSmtpClient->Login(login,pass);
			delete[] pass;
			if(err != B_OK)
			{
				Alert(B_STOP_ALERT,fSmtpClient->Log());
				fLooper->PostMessage(M_SMTP_END,fHandler);
				return err;
			}
		}
		//---------------------------------
		int32 header_len;
		
		file.ReadAttr(B_MAIL_ATTR_HEADER,B_INT32_TYPE,0,&header_len,sizeof(int32));
		
		off_t size;
		file.GetSize(&size);
		char *buf = new char[size+1];
		if(!buf)
		{
			Alert(B_STOP_ALERT,_("Memory was exhausted"),B_STOP_ALERT);
			fLooper->PostMessage(M_SMTP_END,fHandler);
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
		// Convert to mime-B
		if(file.ReadAttr(B_MAIL_ATTR_ENCODING,B_INT32_TYPE,0,&encoding,sizeof(int32))>0)
		{
			encode.UTF82Mime(to,encoding);
			encode.UTF82Mime(from,encoding);
		}
		//
		if(fSmtpClient->SendMail(from.String(),to.String(),buf,SmtpTotalSize,SmtpSentSize,this) == B_OK)
			attrStatus = "Sent";
		else{
			attrStatus = "Error";
			beep();
			Alert(B_STOP_ALERT,"Failed to send mails"); 
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
#if B_BEOS_BONE
	::close(sd);
#else
	::closesocket(sd);
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

/***********************************************************
 * SmtpTotalSize
 ***********************************************************/
void SmtpTotalSize(int32 size,void *cookie)
{
	BLooper *looper = ((SmtpLooper*)cookie)->fLooper;
	BHandler *handler = ((SmtpLooper*)cookie)->fHandler;
	
	BMessage msg(M_SET_MAX_SIZE);
	msg.AddInt32("max_size",size);
	looper->PostMessage(&msg,handler);
}

/***********************************************************
 * SmtpSentSize
 ***********************************************************/
void SmtpSentSize(int32 size,void *cookie)
{
	BLooper *looper = ((SmtpLooper*)cookie)->fLooper;
	BHandler *handler = ((SmtpLooper*)cookie)->fHandler;
	
	BMessage msg(M_SEND_MAIL_SIZE);
	msg.AddInt32("size",size);
	looper->PostMessage(&msg,handler);
}

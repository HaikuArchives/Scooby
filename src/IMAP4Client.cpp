#include "IMAP4Client.h"
#include "HApp.h"
#include <stdlib.h>
#include <Alert.h>
#include <Debug.h>
#define CRLF "\r\n";

const bigtime_t kIMAP4ClientTimeout = 1000000*60; // 60 sec

/***********************************************************
 * Constructor.
 ***********************************************************/
IMAP4Client::IMAP4Client()
	:BNetEndpoint(SOCK_STREAM)
	,fCommandCount(0)
	,fIdleTime(0)
{
//	BNetDebug::Enable(true);
	SetTimeout(kIMAP4ClientTimeout);
}

/***********************************************************
 * Destructor
 ***********************************************************/
IMAP4Client::~IMAP4Client()
{

}

/***********************************************************
 * Connect
 ***********************************************************/
status_t
IMAP4Client::Connect(const char* addr,int port)
{
	_inherited::Connect(addr,port);
	
	BString out("");
	if( ReceiveLine(out) <= 0 )
		return B_ERROR;
	// for re-connect
	fAddress = addr; fPort = port;
	//
	return B_OK;
}

/***********************************************************
 * Reconnect
 ***********************************************************/
status_t
IMAP4Client::Reconnect()
{
	fCommandCount = 0;
	status_t err;
	err = Connect(fAddress.String(),fPort);
	if(err != B_OK)
		return B_ERROR;
	err = Login(fLogin.String(),fPassword.String());
	if(err != B_OK)
		return B_ERROR;
	err = Select(fFolderName.String());
	if(err != B_OK)
		return B_ERROR;
	return B_OK;
}

/***********************************************************
 * Login
 ***********************************************************/
status_t
IMAP4Client::Login(const char* login,const char* password)
{
	// for re-connect
	fLogin = login; fPassword = password;
	//
	BString cmd("LOGIN "),out("");
	cmd += login;
	cmd += " ";
	cmd += password;

	if( SendCommand(cmd.String()) != B_OK)
		return B_ERROR;
	if( ReceiveLine(out) > 6)
	{
		if( out[4] == 'O' && out[5] == 'K')
			return B_OK;
		if( out[4] == 'B' && out[5] == 'A' && out[5] == 'D')
			return B_ERROR;
	}	
	return B_ERROR;
}

/***********************************************************
 * List
 ***********************************************************/
status_t
IMAP4Client::List(const char* folder_name)
{
	BString cmd("LIST ");
	cmd << "\"" << folder_name << "/\" ";
	cmd += "\"%\"";
	
	BString out;
	//SendCommand(cmd.String());
	SendCommand("LIST \"\" \"INBOX\"\r\n");
	ReceiveLine(out);
	char c = 0;
	while(1)
	{
		Receive(&c,1);
		PRINT(("%c",c));
	}
}

/***********************************************************
 * Select
 ***********************************************************/
int32
IMAP4Client::Select(const char* folder_name)
{
	int32 r = 0,mail_count = -1;
	
	BString cmd("SELECT ");
	cmd << "\"" << folder_name << "\"";
	
	BString out;
	if( SendCommand(cmd.String()) == B_OK)
	{
		while(1)
		{
			r = ReceiveLine(out);
			if(r <=0)
				break;
			if( out[4] == 'O' && out[5] == 'K')
				break;
			if( out[4] == 'B' && out[5] == 'A' && out[6] == 'D')
				break;
			if(out.FindFirst("EXISTS")!= B_ERROR)
			{
				const char* p = out.String();
				p += 2;
				//PRINT(("%s\n",p));
				mail_count = atoi(p);
			}
		}
	}
	// for re-connect
	fFolderName = folder_name;
	//
	return mail_count;
}

/***********************************************************
 * Store
 ***********************************************************/
status_t
IMAP4Client::Store(int32 index,const char* flags,bool add)
{
	BString cmd("STORE ");
	cmd << index << " ";
	
	if(add)
		cmd += "+FLAGS.SILENT ";
	else
		cmd += "-FLAGS.SILENT ";
	cmd << "(" << flags << ")";
	
	BString out;
	if(SendCommand(cmd.String()) == B_OK)
	{
		ReceiveLine(out);
		//PRINT(("%s\n",out.String() ));
	}
	return B_ERROR;	
}

/***********************************************************
 * MarkAsRead
 ***********************************************************/
status_t
IMAP4Client::MarkAsRead(int32 index)
{
	// Check connection
	if(!IsAlive())
	{
		PRINT(("Re-connect\n"));
		status_t err = Reconnect();
		if(err != B_OK)
			return B_ERROR;
	}
	//
	return Store(index,"\\Seen");	
}

/***********************************************************
 * MarkAsDelete
 ***********************************************************/
status_t
IMAP4Client::MarkAsDelete(int32 index)
{
	// Check connection
	if(!IsAlive())
	{
		PRINT(("Re-connect\n"));
		status_t err = Reconnect();
		if(err != B_OK)
			return B_ERROR;
	}
	//
	return Store(index,"\\Deleted");	
}


/***********************************************************
 * FetchField
 ***********************************************************/
status_t
IMAP4Client::FetchFields(int32 index,
					BString &subject,
					BString &from,
					BString &to,
					BString &cc,
					BString &reply,
					BString &date,
					BString &priority,
					bool	&read,
					bool	&attachment)
{
	// Check connection
	if(!IsAlive())
	{
		PRINT(("Re-connect\n"));
		status_t err = Reconnect();
		if(err != B_OK)
			return B_ERROR;
	}
	//
	BString cmd("FETCH ");
	cmd << index << " (FLAGS BODY[HEADER.FIELDS (Subject From To Reply-To Date X-Priority Content-Type)])";
	subject = from = to = date = cc = reply = "";
	attachment = read = false;
	priority = "3 (Normal)";	
	BString line;

	int32 r,state;
	int32 session = fCommandCount+1;

	char first_line[50];
	::sprintf(first_line,"* %ld FETCH",index);

	if( SendCommand(cmd.String()) == B_OK)
	{
		while(1)
		{
			r = ReceiveLine(line);
			if(r <=0)
				break;
			//PRINT(("%s\n",line.String() ));
			line.ReplaceAll("\r\n","");
			const char* p = line.String();
			// Check flags
			if(strncmp(first_line,p,strlen(first_line)) == 0)
			{
				read = (line.FindFirst("\\Seen") != B_ERROR)?true:false;
			}
			// Subject
			if(strncmp("Subject:",p,8) == 0)
				subject = &p[9];
			// Date
			else if(strncmp("Date:",p,5) == 0)
				date = &p[6];
			// From
			else if(strncmp("From:",p,5) == 0)
				from = &p[6];
			// To
			else if(strncmp("To:",p,3) == 0)
				to = &p[4];
			// Cc,
			else if(strncmp("Cc:",p,3) == 0)
				cc = &p[4];
			// Reply
			else if(strncmp("Reply-To:",p,9) == 0)
				reply = &p[10];
			// Priority
			else if(strncmp("X-Priority:",p,11) == 0)
				priority = &p[12];
			// Content-Type
			else if(strncmp("Content-Type:",p,13) == 0)
			{
				p += 14;
				attachment = (strncmp(p,"multipart",9) == 0)?true:false;	
			}
			state = CheckSessionEnd(line.String(),session);		
			switch(state)
			{
			case IMAP_SESSION_OK:
				return B_OK;
				break;
			case IMAP_SESSION_BAD:
				return B_ERROR;
				break;
			}
		}
	}
	return B_ERROR;
}

/***********************************************************
 * FetchBody
 ***********************************************************/
status_t
IMAP4Client::FetchBody(int32 index,BString &outBody)
{
	// Check connection
	if(!IsAlive())
	{
		PRINT(("Re-connect\n"));
		status_t err = Reconnect();
		if(err != B_OK)
			return B_ERROR;
	}
	//
	
	BString cmd("FETCH ");
	cmd << index << " BODY[]";
	
	outBody = "";
	BString line;
	
	char first_line[50];
	::sprintf(first_line,"* %ld FETCH",index);
	char end_line[50];
	::sprintf(end_line,"\n%.3ld OK FETCH completed",index);
	
	int32 pos,r=0,state,content_size=0;
	int32 session = fCommandCount+1;
	
	if( SendCommand(cmd.String()) == B_OK)
	{
		while(line.FindFirst("FETCH") == B_ERROR)
			r = ReceiveLine(line);
		if(r <=0)
			return B_ERROR;
			
		PRINT(("%s\n",line.String()));
		state = CheckSessionEnd(line.String(),session);
				
		switch(state)
		{
		case IMAP_SESSION_OK:
			return B_OK;
		case IMAP_SESSION_BAD:
			return B_ERROR;
		}
		// Check content size
		if(strncmp(first_line,line.String(),strlen(first_line)) == 0)
		{
			if((pos = line.FindLast("{")) != B_ERROR)
			{
				const char* p = line.String();
				p += pos+1;
				content_size = atol(p);
			}		
		}
		PRINT(("%d\n",content_size));
			
		char *buf = new char[content_size+1];
		
		if(!buf)
		{
			(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
			return B_ERROR;
		}
		
		while(content_size>0)
		{
			r = Receive(buf,(content_size<=0)?1:content_size);
			buf[r] = '\0';
			outBody += buf;
			content_size -= r;
		}
		delete[] buf;
		// Get Last line
		r = ReceiveLine(line);
		if(r <=0)
			return B_ERROR;
		state = CheckSessionEnd(line.String(),session);
				
		switch(state)
		{
		case IMAP_SESSION_OK:
			return B_OK;
		case IMAP_SESSION_BAD:
			return B_ERROR;
		}
	}
	return B_ERROR;
}

/***********************************************************
 * Noop
 ***********************************************************/
status_t
IMAP4Client::Noop()
{
	PRINT(("Send Noop\n"));
	BString out;
	int32 index = fCommandCount+1;
	
	if( SendCommand("NOOP") != B_OK)
		return B_ERROR;
	char end_line[50];
	::sprintf(end_line,"%.3ld OK NOOP",index);
	
	while(1)
	{
		ReceiveLine(out);
		if(::strncmp(out.String(),end_line,strlen(end_line))==0)
			break;
	}
	
	if( out[4] == 'O' && out[5] == 'K')
		return B_OK;
	return B_ERROR;
}

/***********************************************************
 * IsAlive
 ***********************************************************/
bool
IMAP4Client::IsAlive()
{
	time_t now = time(NULL);
	// If idle time is more than 10 min
	// Check connection with NOOP command
	if(difftime(now,fIdleTime) <= 600)
		return true;
	return (Noop() == B_OK)?true:false;
}

/***********************************************************
 * Logout
 ***********************************************************/
void
IMAP4Client::Logout()
{
	SendCommand("LOGOUT");
	Close();
}

/***********************************************************
 * SendCommand
 ***********************************************************/
status_t
IMAP4Client::SendCommand(const char* command)
{
	BString out("");
	status_t err = B_ERROR;
	char *cmd = new char[strlen(command) + 7];
	if(!cmd)
	{
		(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return B_ERROR;
	}
	::sprintf(cmd,"%.3ld %s\r\n",++fCommandCount,command);
	int32 cmd_len = strlen(cmd);
	//PRINT(("%s",cmd));
	// Reset idle time
	fIdleTime = time(NULL);
	//
	if( Send(cmd,cmd_len) == cmd_len)
		err = B_OK;
	delete[] cmd;
	return err;
}

/***********************************************************
 * ReceivePendingData
 ***********************************************************/
int32
IMAP4Client::ReceiveLine(BString &out)
{
	char c = 0;
	int32 len = 0,r;
	out = "";
	if(IsDataPending(kIMAP4ClientTimeout))
	{
		while(c != '\n')
		{
			r = Receive(&c,1);
			if(r <= 0)
				break;
			out += c;
			len += r;
		}
	}else{
		(new BAlert("",_("IMAP4 socket timeout."),_("OK")))->Go();
	}
	//PRINT(("%s\n",out.String()));
	return len;
}

/***********************************************************
 * CheckSessionEnd
 ***********************************************************/
int32
IMAP4Client::CheckSessionEnd(const char* str,int32 session)
{
	char session_end[5];
	::sprintf(session_end,"%.3ld ",session);
	
	if( ::strncmp(session_end,str,4) == 0)
	{
		if( str[4] == 'O' && str[5] == 'K')
			return IMAP_SESSION_OK;
		if( str[4] == 'B' && str[5] == 'A' && str[6] == 'D')
			return IMAP_SESSION_BAD;
	}	
	return IMAP_SESSION_CONTINUED;
}
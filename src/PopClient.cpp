#include "PopClient.h"
#include "Encoding.h"
#include "md5.h"

#include <Debug.h>
#include <String.h>
#include <stdlib.h>
#include <E-mail.h>
#include <File.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Message.h>
#include <Autolock.h>

#define CRLF "\r\n"

const bigtime_t kTimeout = 1000000*180; //timeout 180 secs
	

/***********************************************************
 * Constructor
 ***********************************************************/
PopClient::PopClient(BHandler *handler,BLooper *looper)
	:BLooper("PopClient")
	,fEndpoint(NULL)
	,fHandler(handler)
	,fLooper(looper)
{
	Run();
}

/***********************************************************
 * Destructor
 ***********************************************************/
PopClient::~PopClient()
{
	delete fEndpoint;
}

/***********************************************************
 * MessageReceived
 ***********************************************************/
void
PopClient::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	// Stat
	case H_STAT_MESSAGE:
		{
			int32 mails,bytes;
			if(Stat(&mails,&bytes) != B_OK)
				PostError();
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
			if(List(index,list) != B_OK)
				PostError();
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
			/*bool is_delete;
			int32 days;
			if(message->FindBool("delete",&is_delete) != B_OK)
				is_delete = false;
			if(message->FindBool("delete_day",&days) != B_OK)
				days = 0;
			*/
			int32 index;
			BString content;
			for(int32 i = 0;i < count;i++)
			{
				if(message->FindInt32("index",i,&index) != B_OK)
					continue;
				if( Retr(index,content) != B_OK)
					PostError();
				else{
					BMessage msg(H_RETR_MESSAGE);
					msg.AddString("content",content);
					msg.AddInt32("index",index);
					fLooper->PostMessage(&msg,fHandler);
				}
				
			}
			// delete server mails
			/*if(!is_delete)
				break;
			message->what = H_DELETE_MESSAGE;
			PostMessage(message);
			*/
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
					if( Delete(index) != B_OK)
						PostError();
					else{
						BMessage msg(H_DELETE_MESSAGE);
						msg.AddInt32("index",index);
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
			
			if( Last(&index) != B_OK)
					PostError();
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
				if(Login(login,password,apop) != B_OK)
				{
					PostError();
				}else
					fLooper->PostMessage(message,fHandler);
			}
			break;
		}
	// connect
	case H_CONNECT_MESSAGE:
		{
			const char *addr;
			int16 port;
			if(message->FindString("address",&addr) == B_OK &&
				message->FindInt16("port",&port) == B_OK)
			{
				if(Connect(addr,port) != B_OK)
				{
					PostError();
				}else
					fLooper->PostMessage(message,fHandler);
			}
			break;
		}
	// reset
	case H_RESET_MESSAGE:
	{
		if( Rset() != B_OK)
			PostError();
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
		if(Uidl(index,list) != B_OK)
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
 * Connect
 ***********************************************************/
status_t
PopClient::Connect(const char* address,
					int16 port)		
{
	fHost=address;
	fPort=port;
	
	
	if(fEndpoint)
		PopQuit();
	fLog = "";
	fEndpoint = new BNetEndpoint();
	fEndpoint->SetTimeout(kTimeout);
	status_t err;
	err = fEndpoint->Connect(fHost.String(),fPort);
	if(err != B_OK)
		return err;
		
	BString line;
	if( ReceiveLine(line) <= 0)
		return B_ERROR;

	if(strncmp(line.String(),"+OK",3) != 0)
		return B_ERROR;
	PRINT(("%s\n",line.String()));
	fLog = line;
	return B_OK;
}

/***********************************************************
 * PostError
 ***********************************************************/
void
PopClient::PostError()
{
	BMessage msg(H_ERROR_MESSAGE);
	msg.AddString("log",fLog);
	fLooper->PostMessage(&msg,fHandler);
}

/***********************************************************
 * Login
 ***********************************************************/
status_t
PopClient::Login(const char* user,const char* password,bool apop)
{
	status_t err;
	if(apop)
	{
		PRINT(("Enter APOP\n"));
		int32 index = fLog.FindFirst("<");
		if(index == B_ERROR)
			goto normal;
		int32 end = fLog.FindFirst(">",index);
		BString timestamp("");
		fLog.CopyInto(timestamp,index,end-index+1);
		timestamp += password;
		char *md5sum = MD5Digest((unsigned char*)timestamp.String());
		
		//md5_buffer(timestamp.String(),timestamp.Length(),buf);
		BString cmd = "APOP ";
		cmd << user << " " << md5sum << CRLF;
		free(md5sum);
		err = SendCommand(cmd.String());
		if(err != B_OK)
		{
			PRINT(( "ERR:%s\n",fLog.String()));
			return err;
		}	
		return B_OK;
	}
normal:
	// Send Username
	BString cmd = "USER ";
	cmd << user << CRLF;
	err = SendCommand(cmd.String());
	if(err != B_OK)
	{
		PRINT(( "ERR:%s\n",fLog.String()));
		return err;
	}	
	// Send Password
	cmd = "PASS ";
	cmd << password << CRLF;
	
	err = SendCommand(cmd.String());
	if(err != B_OK)
	{
		PRINT(( "ERR:%s\n",fLog.String()));
		return err;
	}
	
	return B_OK;
}

/***********************************************************
 * Stat
 ***********************************************************/
status_t
PopClient::Stat(int32 *mails,int32 *numBytes)
{
	BString cmd = "STAT";
	cmd += CRLF;
	if( SendCommand(cmd.String()) != B_OK)
		return B_ERROR;
	
	const char* log = fLog.String();
	*mails = atol(&log[4]);
	int32 i = fLog.FindLast(" ");
	if(i != B_ERROR)
		*numBytes = atol(&log[i]);
	return B_OK;
}

/***********************************************************
 * LIST
 ***********************************************************/
status_t
PopClient::List(int32 index,BString &size_list)
{
	BString cmd = "LIST";
	if(index != 0)
		cmd << " " << index;
	cmd += CRLF;
	
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("LIST error:%s %d\n",__FILE__,__LINE__));
		return B_ERROR;
	}
	size_list = "";
	int32 r = 0;
	if(index != 0)
	{
		const char* log = fLog.String();
		size_list = &log[4];
	}else{
		while(1)
		{
			r = ReceiveLine(cmd);
			if( r <= 0)
				break;
			int32 len = cmd.Length();
			const char* log = cmd.String();
			if(len >2)
				if( ::strcmp(&log[len-3],".\r\n") == 0)
					break;
			size_list += cmd;
		}
	}
	return B_OK;
}


/***********************************************************
 * LIST
 ***********************************************************/
status_t
PopClient::Uidl(int32 index,BString &outlist)
{
	BString cmd = "UIDL";
	if(index != 0)
		cmd << " " << index;
	cmd += CRLF;
	
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("UIDL error:%s %d\n",__FILE__,__LINE__));
		return B_ERROR;
	}
	outlist = "";
	int32 r = 0;
	
	if(index != 0)
	{
		const char* log = fLog.String();
		outlist = &log[4];
	}else{
		while(1)
		{
			r = ReceiveLine(cmd);
			if( r <= 0)
				break;
			int32 len = cmd.Length();
			const char* log = cmd.String();
			if(len >2 && ::strcmp(&log[len-3],".\r\n") == 0)
					break;
			outlist += cmd;	
		}
	}
	return B_OK;
}


/***********************************************************
 * Retr
 ***********************************************************/
status_t
PopClient::Retr(int32 index,BString &content)
{
	BString cmd = "RETR ";
	cmd << index << CRLF;
	PRINT(("%s\n",cmd.String() ));
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Retr error:%s %d\n",__FILE__,__LINE__));
		return B_ERROR;
	}
	const char* log = fLog.String();
	int32 size = atol(&log[4]);
	int32 r;
	//PRINT(("Content size:%d\n",size));
	size += 5;
	BMessage msg(H_SET_MAX_SIZE);
	msg.AddInt32("max_size",size);
	fLooper->PostMessage(&msg,fHandler);
	
	msg.MakeEmpty();
	msg.what = H_RECEIVING_MESSAGE;
	msg.AddInt32("index",index);
	msg.AddInt32("size",0);
	content = "";
	char *buf = new char[size+1];
	int32 content_len = 0;
	while(1)
	{
		if(fEndpoint->IsDataPending(kTimeout))
		{
			r = fEndpoint->Receive(buf,size);
			if(r <= 0)
				return B_ERROR;
			size -= r;
			content_len += r;
			buf[r] = '\0';
			content += buf;
			msg.ReplaceInt32("size",r);
			fLooper->PostMessage(&msg,fHandler);
			
			if(content_len > 4 &&
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
 * Delete
 ***********************************************************/
status_t
PopClient::Delete(int32 index)
{
	BString cmd = "DELE ";
	cmd << index << CRLF;
	PRINT(("%s\n",cmd.String() ));
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("DELE error:%s %d\n",__FILE__,__LINE__));
		return B_ERROR;
	}
	return B_OK;
}

/***********************************************************
 * Last
 ***********************************************************/
status_t
PopClient::Last(int32 *index)
{
	BString cmd = "LAST";
	cmd += CRLF;
	if( SendCommand(cmd.String()) != B_OK)
		return B_ERROR;
	
	const char* log = fLog.String();
	*index = atol(&log[4]);
	return B_OK;
}

/***********************************************************
 * Rset
 ***********************************************************/
status_t
PopClient::Rset()
{
	BString cmd = "RSET";
	cmd += CRLF;
	
	if( SendCommand(cmd.String()) != B_OK)
		return B_ERROR;
	return B_OK;	
}

/***********************************************************
 * Quit
 ***********************************************************/
status_t
PopClient::PopQuit()
{
	if(!fEndpoint)
		return B_OK;
	BString cmd = "QUIT";
	cmd += CRLF;
	
	if( SendCommand(cmd.String()) != B_OK)
		return B_ERROR;
	ReceiveLine(cmd);
	fEndpoint->Close();
	delete fEndpoint;
	fEndpoint = NULL;
	
	return B_OK;	
}

/***********************************************************
 * Command
 ***********************************************************/
status_t
PopClient::SendCommand(const char* cmd)
{
	int32 len;
	if(!fEndpoint)
		return B_ERROR;
	
	if( fEndpoint->Send(cmd, ::strlen(cmd)) == B_ERROR)
		return B_ERROR;
	fLog = "";
	// Receive
	
	len = ReceiveLine(fLog);
	
	if(len <= 0)
		return B_ERROR;
	PRINT(("%s",fLog.String()));
	if(strncmp(fLog.String(),"+OK",3) != 0)
		return B_ERROR;
	return B_OK;
}


/***********************************************************
 * ReadData
 ***********************************************************/
int32
PopClient::ReceiveLine(BString &line)
{
	if(!fEndpoint)
		return B_ERROR;
	int32 len = 0,rcv;
	char c = 0;
	line = "";
	
	while(c != '\n')
	{
		if(fEndpoint->IsDataPending(kTimeout))
		{
			rcv = fEndpoint->Receive(&c,1);
			if(rcv <=0)
				break;
			len += rcv;
			line << c;
		}else
			break;			
	}
	return len;
}

/***********************************************************
 * MakeTime_t
 ***********************************************************/
time_t
PopClient::MakeTime_t(const char* date)
{
	const char* mons[] = {"Jan","Feb","Mar","Apr","May"
						,"Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	const char* wdays[] = {"Sun","Mon","Tue","Wed","Thu"
						,"Fri","Sat"};
	
	char swday[5];
	int wday;
	int day;
	char smon[4];
	int mon;
	char stime[9];
	int year;
	int hour;
	int min;
	int sec;
	int gmt_off = 0;
	char offset[5];
	::sscanf(date,"%s%d%s%d%s%s",swday,&day,smon,&year,stime,offset);
	
	// parse time
	hour = atoi(stime);
	min = atoi(stime+3);
	sec = atoi(stime+6);
	PRINT(("TIME:%s\n",stime));
	PRINT(("H:%d M:%d S:%d\n",hour,min,sec ));
	// month
	for(mon = 0;mon < 12;mon++)
	{
		if(strncmp(mons[mon],smon,3) == 0)
			break;
	}
	PRINT(("MONTH:%s\n",smon));
	PRINT(("M:%d\n",mon));
	// week of day
	for(wday = 0;wday < 12;wday++)
	{
		if(strncmp(wdays[wday],swday,3) == 0)
			break;
	}
	PRINT(("WEEK:%s\n",swday));
	PRINT(("W:%d\n",wday));
	// offset 
	char op = offset[0];
	int off = atoi(offset+1);
	if(op == '+')
		gmt_off  = (off/100)*60*60;
	if(op == '-')
		gmt_off  = -(off/100)*60*60;
	PRINT(("GMT_OFF:%s\n",offset));
	PRINT(("G:%d\n",gmt_off));
	struct tm btime;
	btime.tm_sec = sec;
	btime.tm_min = min;
	btime.tm_hour = hour;
	btime.tm_mday = day;
	btime.tm_mon = mon;
	btime.tm_year = year-1900;
	btime.tm_wday = wday;
	btime.tm_gmtoff = gmt_off;
	return mktime(&btime);	
}

/***********************************************************
 * MD5Digest (unsigned char *s)
 ***********************************************************/
char*
PopClient::MD5Digest (unsigned char *s)
{
	int i;
	MD5_CTX context;
	unsigned char digest[16];
	char ascii_digest [33];
	
	MD5Init(&context);
	MD5Update(&context, s, strlen((char*)s));
	MD5Final(digest, &context);
  	
  	for (i = 0;  i < 16;  i++) 
    	sprintf(ascii_digest+2*i, "%02x", digest[i]);
 
	return strdup(ascii_digest);
}

/***********************************************************
 * Quit
 ***********************************************************/
bool
PopClient::QuitRequested()
{
	PopQuit();
	fLooper->PostMessage(M_QUIT_FINISHED,fHandler);
	return BLooper::QuitRequested();
}
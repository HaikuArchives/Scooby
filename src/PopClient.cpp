#include "PopClient.h"
#include "Encoding.h"
#include "md5.h"
#include "HApp.h"
#include "HFile.h"
#include "HPopClientView.h"

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
PopClient::PopClient(BHandler *handler,BLooper *looper)
	:BLooper("PopClient")
	,fEndpoint(NULL)
	,fHandler(handler)
	,fLooper(looper)
{
	Run();
	fBlackList.MakeEmpty();
}

/***********************************************************
 * Destructor
 ***********************************************************/
PopClient::~PopClient()
{
	delete fEndpoint;
	// Free Blacklist
	int32 count = fBlackList.CountItems();
	while(count > 0)
		free( (char*)fBlackList.RemoveItem(--count) );
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
		
			int32 index;
			BString content;
			for(int32 i = 0;i < count;i++)
			{
				if(message->FindInt32("index",i,&index) != B_OK)
					continue;
				if( Retr(index,content) != B_OK)
				{
					PostError();
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
					if( Delete(index) != B_OK)
						PostError();
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
	//PRINT(("%s\n",line.String()));
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
		//PRINT(("Enter APOP\n"));
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
		cmd += user;
		cmd += " ";
		cmd += md5sum;
		cmd += CRLF;
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
	cmd += user;
	cmd += CRLF;
	err = SendCommand(cmd.String());
	if(err != B_OK)
	{
		PRINT(( "ERR:%s\n",fLog.String()));
		return err;
	}	
	// Send Password
	cmd = "PASS ";
	cmd += password;
	cmd += CRLF;
	
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
	int32 size = 0;
	BString size_list;
	
	content = "";
	PRINT(("BLACKLIST COUNT:%d\n",fBlackListCount));
	// Spam filter
	if(index != 0 && fBlackListCount > 0)
	{
		BString topOutput;
		if(Top(index,0,topOutput) != B_OK)
			PRINT(("%s\n",fLog.String() ));	
		else{
			if( IsSpam(topOutput.String()) )
			{
				if(((HPopClientView*)fHandler)->RetrieveType() > 0)
					Delete(index);
				return B_OK;
			}
		}
	}
	// Get mail content size 
	if(index != 0)
	{	
		if(List(index,size_list) != B_OK) {
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
	if( SendCommand(cmd.String()) != B_OK)
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
		if(fEndpoint->IsDataPending(kTimeout))
		{
			r = fEndpoint->Receive(buf,MAX_RECIEVE_BUF_SIZE);
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
	// Read output
	BString out;
	int32 r = 0;
	status_t err = B_OK;
	if(fLog.Compare("+OK",3) != 0 && fLog.Compare("-ERR",4) != 0)
	{
		while(1)
		{
			r = ReceiveLine(out);
			PRINT(("out:%s\n",out.String() ));
			if(r <= 0|out.Compare("+OK",3) == 0)
				break;
			else if(out.Compare("-ERR",4) == 0)
			{
				err = B_ERROR;
				break;
			}
		}
	}
	return err;
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
 * Top
 ***********************************************************/
status_t
PopClient::Top(int32 index,int32 lines,BString &out)
{
	BString cmd = "TOP ";
	cmd << index << " " << lines << CRLF;
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("TOP error:%s %d\n",__FILE__,__LINE__));
		return B_ERROR;
	}
	
	out = "";
	int32 r = 0;
	r = ReceiveLine(cmd);
	if(r <=0 || cmd[0] == '-')
		return B_ERROR;
	while(1)
	{
		r = ReceiveLine(cmd);
		if( r <= 0)
			break;
		int32 len = cmd.Length();
		const char* log = cmd.String();
		if(len >2 && ::strcmp(&log[len-3],".\r\n") == 0)
				break;
		out += cmd;	
	}
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
	if(strncmp(cmd,"PASS",4) != 0)
		PRINT(("C:%s",cmd));
	if( fEndpoint->Send(cmd, ::strlen(cmd)) == B_ERROR)
		return B_ERROR;
	fLog = "";
	// Receive
	status_t err = B_OK;
	
	while(1)
	{
		len = ReceiveLine(fLog);
		PRINT(("S:%s\n",fLog.String() ));
		if(len <= 0|fLog.ICompare("+OK",3) == 0)
			break;
		else if(fLog.ICompare("-ERR",4) == 0)
		{
			err = B_ERROR;
			break;
		}
	}
	return err;
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
	int c = 0;
	line = "";
	
	if(fEndpoint->IsDataPending(kTimeout))
	{	
		while(c != '\n'&& c != EOF && c != xEOF)
		{
			rcv = fEndpoint->Receive(&c,1);
			if(rcv <=0)
				break;
			len += rcv;
			line += (char)c;
		}
	}else{
		(new BAlert("",_("POP3 socket timeout."),_("OK")))->Go();
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
	//PRINT(("TIME:%s\n",stime));
	//PRINT(("H:%d M:%d S:%d\n",hour,min,sec ));
	// month
	for(mon = 0;mon < 12;mon++)
	{
		if(strncmp(mons[mon],smon,3) == 0)
			break;
	}
	//PRINT(("MONTH:%s\n",smon));
	//PRINT(("M:%d\n",mon));
	// week of day
	for(wday = 0;wday < 12;wday++)
	{
		if(strncmp(wdays[wday],swday,3) == 0)
			break;
	}
	//PRINT(("WEEK:%s\n",swday));
	//PRINT(("W:%d\n",wday));
	// offset 
	char op = offset[0];
	int off = atoi(offset+1);
	if(op == '+')
		gmt_off  = (off/100)*60*60;
	if(op == '-')
		gmt_off  = -(off/100)*60*60;
	//PRINT(("GMT_OFF:%s\n",offset));
	//PRINT(("G:%d\n",gmt_off));
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
 * IsSpam
 ***********************************************************/
bool
PopClient::IsSpam(const char* header)
{
	BString from("");
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
					from = *p;
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
		if(from.Compare((const char*)fBlackList.ItemAt(i)) == 0)
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
PopClient::InitBlackList()
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
PopClient::ForceQuit()
{
	if(!fEndpoint)
		return;
	
	fEndpoint->Close();
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
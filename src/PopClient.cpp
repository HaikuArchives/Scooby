#include "PopClient.h"
#include "md5.h"
#include "HFile.h"
#include "TrackerString.h"

#include <Debug.h>
#include <String.h>
#include <stdlib.h>

#define CRLF "\r\n"
#define MAX_RECIEVE_BUF_SIZE 1024000
	
const bigtime_t kTimeout = 1000000*180; //timeout 180 secs

/***********************************************************
 * Constructor
 ***********************************************************/
PopClient::PopClient()
	:_inherited()
{
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
	
	
	fLog = "";
	status_t err;
	err = _inherited::Connect(address,port);
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
		char md5sum[33];
		MD5Digest((unsigned char*)timestamp.String(),md5sum);
		
		//md5_buffer(timestamp.String(),timestamp.Length(),buf);
		
		char *cmd = new char[strlen(user)+strlen(md5sum)+9];
		::sprintf(cmd,"APOP %s %s\r\n",user,md5sum);
	
		err = SendCommand(cmd);
		delete[] cmd;
		if(err != B_OK)
		{
			PRINT(( "ERR:%s\n",fLog.String()));
			return err;
		}	
		return B_OK;
	}
normal:
	// Send Username
	char *cmd = new char[::strlen(user)+8];
	::sprintf(cmd,"USER %s\r\n",user);
	err = SendCommand(cmd);
	delete[] cmd;
	if(err != B_OK)
	{
		PRINT(( "ERR:%s\n",fLog.String()));
		return err;
	}	
	// Send Password
	cmd = new char[::strlen(password)+8];
	::sprintf(cmd,"PASS %s\r\n",password);
	err = SendCommand(cmd);
	delete[] cmd;
	
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
	if( SendCommand("STAT\r\n") != B_OK)
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
PopClient::Retr(int32 index
				,BString &content
				,void (*TotalSize)(int32,void*)
				,void (*SentSize)(int32 ,void*)
				,void *cookie
				)
{
	int32 size = 0;
	BString size_list;
	
	content = "";
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
	
	if(TotalSize)
		TotalSize(size,cookie);
	
	char *buf = new char[MAX_RECIEVE_BUF_SIZE+1];
	if(!buf)
	{
		fLog = "Memory was exhausted";
		return B_ERROR;
	}
	int32 content_len = 0;
	//PRINT(("buf_size:%d\n",buf_size));
	while(1)
	{
		if(IsDataPending(kTimeout))
		{
			r = Receive(buf,MAX_RECIEVE_BUF_SIZE);
			if(r <= 0)
			{
				PRINT(("Receive Err:%d %d %s\n",r,size,buf));
				return B_ERROR;
			}
			size -= r;
			content_len += r;
			buf[r] = '\0';
			content += buf;
			
			if(SentSize)
				SentSize(r,cookie);
			
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
	if( SendCommand("LAST\r\n") != B_OK)
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
	if( SendCommand("RSET\r\n") != B_OK)
		return B_ERROR;
	return B_OK;	
}

/***********************************************************
 * Quit
 ***********************************************************/
status_t
PopClient::PopQuit()
{
	if( SendCommand("QUIT\r\n") != B_OK)
		return B_ERROR;
	return B_OK;	
}

/***********************************************************
 * Command
 ***********************************************************/
status_t
PopClient::SendCommand(const char* cmd)
{
	int32 len;

	if(strncmp(cmd,"PASS",4) != 0)
		PRINT(("C:%s",cmd));
	if( Send(cmd, ::strlen(cmd)) == B_ERROR)
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
	int32 len = 0,rcv;
	int8 c = 0;
	line = "";
	
	if(IsDataPending(kTimeout))
	{	
		while(c != '\n'&& c != EOF )
		{
			rcv = Receive(&c,1);
			if(rcv <=0)
				break;
			len += rcv;
			line += (char)c;
		}
	}else{
		fLog = "POP3 socket timeout.";
	}
	return len;
}

/***********************************************************
 * MD5Digest
 ***********************************************************/
void
PopClient::MD5Digest (unsigned char *in,char *ascii_digest)
{
	int i;
	MD5_CTX context;
	unsigned char digest[16];
	
	MD5Init(&context);
	MD5Update(&context, in, ::strlen((char*)in));
	MD5Final(digest, &context);
  	
  	for (i = 0;  i < 16;  i++) 
    	sprintf(ascii_digest+2*i, "%02x", digest[i]);
 
	return;
}

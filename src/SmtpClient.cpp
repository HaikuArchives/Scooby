#include "SmtpClient.h"
#include "md5.h"
#include "base64.h"

#include <stdlib.h>
#include <Debug.h>
#include <E-mail.h>

#define CRLF "\r\n"
#define xEOF    236

#define SMTP_RESPONSE_SIZE 8192

enum AuthType{
	LOGIN=1,
	PLAIN=1<<2,
	CRAM_MD5=1<<3,
	DIGEST_MD5=1<<4
};

/***********************************************************
 * Constructor
 ***********************************************************/
SmtpClient::SmtpClient()
	:_inherited()
	,fAuthType(0)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
SmtpClient::~SmtpClient()
{
	Close();
}


/***********************************************************
 * Connect
 ***********************************************************/
status_t
SmtpClient::Connect(const char* address,int16 port,bool esmtp)
{
	if(_inherited::Connect(address,port) != B_OK)
	{
		PRINT(("Err:Unknown host\n"));
		return B_ERROR;	
	}
	
	BString line;
	ReceiveResponse(line);
	
	BString cmd;
	if(!esmtp)
		cmd = "HELO ";
	else
		cmd = "EHLO ";
	cmd += address;
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err:%s\n",fLog.String()));
		return B_ERROR;
	}
	
	// Check auth type
	if(esmtp)
	{
		const char* res = fLog.String();
		char* p;
		if((p=::strstr(res,"250-AUTH")))
		{
			if(::strstr(p,"LOGIN"))
				fAuthType |= LOGIN;
			if(::strstr(p,"PLAIN"))
				fAuthType |= PLAIN;	
			if(::strstr(p,"CRAM-MD5"))
				fAuthType |= CRAM_MD5;
			if(::strstr(p,"DIGEST-MD5"))
				fAuthType |= DIGEST_MD5;
		}
	}
	PRINT(("SMTP:Connect success\n"));
	return B_OK;
}

/***********************************************************
 * Login
 ***********************************************************/
status_t
SmtpClient::Login(const char* _login,const char* password)
{
	if(fAuthType==0)
		return B_ERROR;
	const char* login = _login;		
	char hex_digest[33];
	BString out;
	
	int32 loginlen = ::strlen(login);
	int32 passlen = ::strlen(password);
	
	if(fAuthType&CRAM_MD5)
	{
		//******* CRAM-MD5 Authentication ( not tested yet.)
		SendCommand("AUTH CRAM-MD5");
		const char* res = fLog.String();
		
		if(strncmp(res,"334",3)!=0)
			return B_ERROR;
		char *base = new char[::strlen(&res[4])+1];
		int32 baselen = ::strlen(base);
		baselen = ::decode64(base,base,baselen);
		base[baselen] = '\0';
		
		::MD5HexHmac(hex_digest,
				(const unsigned char*)base,
				(int)baselen,
				(const unsigned char*)password,
				(int)passlen);
		printf("%s\n%s\n",base,hex_digest);
		delete[] base;
		
		char *resp = new char[(strlen(hex_digest)+loginlen)*2+3];
		
		::sprintf(resp,"%s %s",login,hex_digest);
		baselen = ::encode64(resp,resp,strlen(resp));
		resp[baselen]='\0';
		SendCommand(resp);
		
		delete[] resp;
		
		res = fLog.String();
		if(atol(res)<500)
			return B_OK;
		
	}
	if(fAuthType&DIGEST_MD5){
	//******* DIGEST-MD5 Authentication ( not written yet..)
		fLog = "DIGEST-MD5 Authentication is not supported in Scooby";
		return B_ERROR;
	}
	if(fAuthType&LOGIN){
	//******* LOGIN Authentication ( tested. work fine)
		SendCommand("AUTH LOGIN");
		const char* res = fLog.String();
		
		if(strncmp(res,"334",3)!=0)
			return B_ERROR;
		// Send login name as base64
		char* login64 = new char[loginlen*3+3];
		::encode64(login64,(char*)login,loginlen);
		SendCommand(login64);
		delete [] login64;
		
		res = fLog.String();
		if(strncmp(res,"334",3)!=0)
			return B_ERROR;
		// Send password as base64
		login64 = new char[passlen*3+3];
		::encode64(login64,(char*)password,passlen);
		SendCommand(login64);
		delete[] login64;
		res = fLog.String();
		if(atol(res)<500)
			return B_OK;
	}
	//******* PLAIN Authentication ( not test yet.)
	if(fAuthType&PLAIN){	
		char* login64 = new char[((loginlen+1)*2+passlen)*3];
		::memset(login64,0,((loginlen+1)*2+passlen)*3);
		::memcpy(login64,login,loginlen);
		::memcpy(login64+loginlen+1,login,loginlen);
		::memcpy(login64+loginlen*2+2,password,passlen);
		
		::encode64(login64,login64,((loginlen+1)*2+passlen));
		BString cmd ="AUTH PLAIN ";
		cmd += login64;
		delete[] login64;
		
		SendCommand(cmd.String());	
		const char* res = fLog.String();
		if(atol(res)<500)
			return B_OK;
	}
		
	return B_ERROR;
}

/***********************************************************
 * ReceiveResponse
 ***********************************************************/
int32
SmtpClient::ReceiveResponse(BString &out)
{
	out = "";
	int32 len = 0,r;
	char buf[SMTP_RESPONSE_SIZE];
	bigtime_t timeout = 1000000*180; //timeout 180 secs
	
	if(IsDataPending(timeout))
	{	
		while(1)
		{
			r = Receive(buf,SMTP_RESPONSE_SIZE-1);
			if(r <= 0)
				break;
			len += r;
			out.Append(buf,r);
			if(strstr(buf,"\r\n"))
				break;
		}
	}else{
		fLog = "SMTP socket timeout.";
	}
	PRINT(("S:%s\n",out.String()));
	return len;
}

/***********************************************************
 * Command
 ***********************************************************/
status_t
SmtpClient::SendCommand(const char* _cmd,bool send_line_break)
{
	int32 len;
	BString cmd=_cmd;
	PRINT(("C:%s\n",_cmd));
	if(send_line_break)
		cmd += CRLF;
 	if( Send(cmd.String(), cmd.Length()) == B_ERROR)
		return B_ERROR;
	fLog = "";
	// Receive
	while(1)
	{
		len = ReceiveResponse(fLog);
		
		if(len <= 0)
			return B_ERROR;
		if(fLog.Length() > 4 && (fLog[3] == ' '||fLog[3] == '-'))
		{
			const char* top = fLog.String();
			int32 num = atol( top );
			PRINT(("ReplyNumber: %d\n",num));
			if(num >= 500)
				return B_ERROR;
			else
				break;
		}
	}
	return B_OK;
}

/***********************************************************
 * SendMail
 ***********************************************************/
status_t
SmtpClient::SendMail(const char* from
					,const char* to
					,const char* content
					,void (*TotalSize)(int32,void*)
					,void (*SentSize)(int32,void*)
					,void* cookie
					)
{
	// Set mail from
	BString cmd = "MAIL FROM: ";

	ParseAddress(from,cmd);
	if(SendCommand(cmd.String(),false) != B_OK)
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
			addr += (char)c;
		if(c == ','||i == len-1)
		{
			if(addr.Length() == 0)
				continue;
			cmd = "RCPT TO: ";
			const char* kText = addr.String();
			
			ParseAddress(kText,cmd);
			if(SendCommand(cmd.String(),false) != B_OK)
			{
				PRINT(("Err: rcpt\n"));
				return B_ERROR;
			}
			addr ="";
		}
	}
	// Data
	cmd = "DATA";
	if(SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err: data\n"));
		return B_ERROR;
	}
	// Content
	BString line("");
	len = ::strlen(content);
	if(TotalSize)
		TotalSize(len,cookie);
		
	int32 sent_len;
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
			if((sent_len = Send(line.String(),line_len))<=0)
				goto err;
			line="";
			if(SentSize)
				SentSize(sent_len,cookie);
		}
	}

	cmd = ".";
	if( SendCommand(cmd.String()) != B_OK)
	{
		PRINT(("Err:Send content\n"));
		return B_ERROR;
	}
	return B_OK;
err:
	PRINT(("Err:Fail to send contenn"));
	return B_ERROR;
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
			out += (char)*p;
			if(*p++ == '>')
				break;
		}
		out += CRLF;
	}else{
		out += "<"; 
		
		int32 len = strlen(in);
		for(int32 i = 0;i < len;i++)
		{
			if(in[i] == ' ')
				break;
			out += in[i];
		}
		out += ">\r\n";
	}
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
	return B_OK;
}
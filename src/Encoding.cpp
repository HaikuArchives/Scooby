#include "Encoding.h"
#include "HApp.h"
#include "HPrefs.h"

#include <ctype.h>
#include <Debug.h>

enum {
	NUL = 0,
	LF = 10,
	CR = 13,
	ESC = 27,
	SS2 = 142
};

#ifdef USE_ICONV
#include "iconv.h"
#endif

const char *kCharsets[] ={"ISO-8859-1",
								"ISO-8859-2",
								"ISO-8859-3",
								"ISO-8859-4",
								"ISO-8859-5",
								"ISO-8859-6",
								"ISO-8859-7",
								"ISO-8859-8",
								"ISO-8859-9",
								"ISO-8859-10",
								"ISO-2022-JP",
								"koi8-r",
								"euc-kr",
								"ISO-8859-13",
								"ISO-8859-14",
								"ISO-8859-15",
								"WINDOWS-1251",
								"WINDWOS-1252"};
	
const int32 kEncodings[] = {B_ISO1_CONVERSION,
								B_ISO2_CONVERSION,
								B_ISO3_CONVERSION,
								B_ISO4_CONVERSION,
								B_ISO5_CONVERSION,
								B_ISO6_CONVERSION,
								B_ISO7_CONVERSION,
								B_ISO8_CONVERSION,
								B_ISO9_CONVERSION,
								B_ISO10_CONVERSION,
								B_JIS_CONVERSION,
								B_KOI8R_CONVERSION,
								B_EUC_KR_CONVERSION,
								B_ISO13_CONVERSION,
								B_ISO14_CONVERSION,
								B_ISO15_CONVERSION,
								B_MS_WINDOWS_1251_CONVERSION,
								B_MS_WINDOWS_CONVERSION};

const int32 kNumCharset = 18;

const char kMimeBase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/***********************************************************
 * Constructor
 ***********************************************************/
Encoding::Encoding()
{
	((HApp*)be_app)->Prefs()->GetData("encoding",&fDefaultEncoding);
}

/***********************************************************
 * Destructor
 ***********************************************************/
Encoding::~Encoding()
{
}

/***********************************************************
 * UTF82Mime
 ***********************************************************/
void
Encoding::UTF82Mime(BString &str,int32 encoding)
{
	int32 len = str.Length();

	const char *kText = str.String();
	BString out("");
	bool is_mime = false;
	BString mime("");
	for(int32 i = 0;i < len;i++)
	{
		if( !isascii(kText[i]) )
		{
			is_mime = true;
			mime += (char)kText[i];
		}else{
			if(is_mime)
			{
				ToMime(mime,encoding);
				out += mime;
				mime = "";		
			}
			is_mime = false;
			out += kText[i];
		}	
	}
	if(is_mime)
	{
		ToMime(mime,encoding);
		out += mime;
	}	
	str = out;
}

/***********************************************************
 * p_IsMimeBase
 ***********************************************************/
bool
Encoding::p_IsMultiByte(char c)
{
	bool b = c>>7;
	
	return !b;
}

/***********************************************************
 * Mime
 ***********************************************************/
void
Encoding::ToMime(BString &inString, int32 encoding) 
{ 
	int i = 0; 
	BString outString("");
	
	const char kJis_End[3] = {0x1b,0x28,0x42};
	
	ConvertFromUTF8(inString,encoding);
	// For japanese jis support	
	if(encoding == B_JIS_CONVERSION)
		inString << kJis_End;
	int32 inlen = inString.Length();
	const char *in = inString.String();
	char *out = outString.LockBuffer(inString.Length() *3);
 
	for (; inlen >= 3; inlen -= 3)
    {
    	out[i++] = kMimeBase[in[0] >> 2];
		out[i++] = kMimeBase[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		out[i++] = kMimeBase[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		out[i++] = kMimeBase[in[2] & 0x3f];
		in += 3;
    }
    if (inlen > 0)
    {
		char f;
    
		out[i++] = kMimeBase[in[0] >> 2];
		f = (in[0] << 4) & 0x30;
		if (inlen > 1)
		    f |= in[1] >> 4;
		out[i++] = kMimeBase[f];
		out[i++] = (inlen < 2) ? '=' : kMimeBase[(in[1] << 2) & 0x3c];
		out[i++] = '=';
    }
    out[i] = '\0';
    
    outString.UnlockBuffer(strlen(out));
    outString << "?=";
    
    // Find encoding
    int32 encoding_index = -1;
    for(int32 i = 0;i < kNumCharset;i++)
    {
    	if(encoding == kEncodings[i] )
    	{
    		encoding_index = i;
    		break;
    	}
    }
    if(encoding_index < 0)
    	return;
    
    inString = "=?";
    inString << kCharsets[encoding_index] << "?B?";
    inString << outString;
    return; 
} 

/***********************************************************
 * ISO2022JP2UTF8
 ***********************************************************/
status_t
Encoding::ISO2UTF8(BString &str,int32 &encoding)
{
	// Find encoding
	int32 encoding_index = -1;
	for(int32 i = 0;i < kNumCharset;i++)
	{
		int32 index = str.IFindFirst(kCharsets[i]);
		if(index != B_ERROR)
		{
			encoding = kEncodings[i];
			encoding_index = i;
			break;
		}
	}
	if(encoding_index < 0)
		return B_ERROR;
	
	BString result(""),mime("");
	bool is_mime = false;
	bool quoted_printable = false;
	int32 len = str.Length();
	const char *buf = str.String();
	
	BString key("=?");
	key << kCharsets[encoding_index] << "?";
	
	for(int32 i = 0;i < len;i++)
	{
		if(!is_mime && ::strncasecmp(buf+i,key.String(),key.Length()) == 0)
		{
			buf+=key.Length();
			is_mime = true;
			quoted_printable = (buf[i++] == 'Q')?true:false;
		}else if(is_mime && ::strncmp(buf+i,"?=",2) == 0){//if(buf[i] == '?'||buf[i] == '='){
			is_mime = false;
			buf+=1;
			MimeDecode(mime,quoted_printable);
			result << mime;
			mime = "";
		}else{
			if(!is_mime)
			{
				if(buf[i] != '\n' && buf[i] != '\r')
					result << buf[i];
				if(buf[i] == '\0')
					break;
			}else{
				if(buf[i] != '\n' && buf[i] != '\r')
					mime << buf[i];
			}
		}
	}

	str = result;
	return B_OK;
}

/***********************************************************
 * Mime2UTF8
 ***********************************************************/
void
Encoding::Mime2UTF8(BString &str)
{
#ifdef USE_ICONV
	int32 encode = fDefaultEncoding;
	int32 start = str.FindFirst("=?");
	BString charset("");
	if(start != B_ERROR)
	{	
		start += 2;
		int32 end = str.FindFirst("?",start);
		str.CopyInto(charset,start,end-start);
	}
	ISO2UTF8(str,encode);
	if(encode > 0 && charset.Length() > 0)
		ConvertToUTF8(str,charset.String());
#else
	int32 encode = fDefaultEncoding;
	
	ISO2UTF8(str,encode);
	
	if(encode > 0)
		ConvertToUTF8(str,encode);
#endif
	return; 
}


/***********************************************************
 * MimeDecode
 ***********************************************************/
void
Encoding::MimeDecode(BString &str,bool quoted_printable)
{
   int len, i, iR;
   char a1, a2, a3, a4;
   len = str.Length();
   char *old = str.LockBuffer(0);
   if(len == 0)
   		return;
   // MIME-Q
   if(quoted_printable)
   {
   		char *buf = new char[len+1];
   		decode_quoted_printable(buf,old,len,false);	
   		str.UnlockBuffer();
   		str = buf;
   		delete[] buf;
   }else{	
   // MIME-B
   		i = 0;
    	iR = 0;
    	while (1) {
        	if (i >= len)
        	    break;
        	a1 = p_Charconv(old[i]);
        	a2 = p_Charconv(old[i+1]);
        	a3 = p_Charconv(old[i+2]);
        	a4 = p_Charconv(old[i+3]);
        	//printf("%c %c %c %c\n",old[i],old[i+1],old[i+2],old[i+3]);
        	//printf("%c %c %c %c\n",a1,a2,a2,a3);
        	old[iR] = (a1 << 2) | (a2 >>4);
        	
        	old[iR + 1] = (a2 << 4) | (a3 >>2);
        	old[iR + 2] = (a3 << 6) | a4;
        	
        	iR += 3;    	
        	i += 4;
        }
    	old[iR] = '\0';
    	str.UnlockBuffer();
    }
}

/***********************************************************
 * p_Charconv
 ***********************************************************/
char
Encoding::p_Charconv(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (c - 'A');
    if (c >= 'a' && c <= 'z')
        return (c - 'a' + 0x1a);
    if (c >= '0' && c <= '9')
        return (c - '0' + 0x34);
    if (c == '+')
        return 0x3e;
    if (c == '/')
        return 0x3f;
    if (c == '=')
        return '\0';
    PRINT(("Invalid character[%c]", c));
    return '\0';
}

/***********************************************************
 * Return encoding with charset
 ***********************************************************/
int32
Encoding::p_Encoding(const char* charset)
{
	int32 i;
	int32 encoding = -1;
	
	for(i = 0;i < kNumCharset;i++)
	{
		if(::strncasecmp(charset,kCharsets[i],strlen(kCharsets[i])) == 0)
		{
			encoding = kEncodings[i];
			break;
		}
	}
	return encoding;
}

/***********************************************************
 * ConvertFromUTF8
 ***********************************************************/
void
Encoding::ConvertFromUTF8(BString &text,const char* charset)
{
	int32 encoding = p_Encoding(charset);
	
	if(encoding < 0)
		return;
	ConvertFromUTF8(text,encoding);
}

/***********************************************************
 * ConvertToUTF8
 ***********************************************************/
void
Encoding::ConvertToUTF8(char** text,const char* charset)
{
#ifdef USE_ICONV
	size_t inlen = ::strlen(*text) + 1;
	
	iconv_t cd;
	cd = iconv_open("UTF-8",charset);
	if(cd == (iconv_t)(-1))
		return;
	size_t outlen = inlen*2;
	char *outbuf = new char[outlen];
	const char* inbuf = *text;
	::memset(outbuf,0,outlen);
	char *outbufp = outbuf;
	size_t r = iconv(cd,&inbuf,&inlen,&outbufp,&outlen);
	if(r < 0)
	{
		delete[] outbuf;
		return;
	}
	delete[] *text;
	*text = outbuf;
	iconv_close(cd);
#else
	int32 encoding = p_Encoding(charset);
	
	if(encoding < 0)
		return;
	ConvertToUTF8(text,encoding);
#endif
}


/***********************************************************
 * ConvertFromUTF8
 ***********************************************************/
void
Encoding::ConvertFromUTF8(char** text,const char* charset)
{
	int32 encoding = p_Encoding(charset);
	
	if(encoding < 0)
		return;
	ConvertFromUTF8(text,encoding);
}

/***********************************************************
 * ConvertToUTF8
 ***********************************************************/
void
Encoding::ConvertToUTF8(BString &text,const char* charset)
{
#ifdef USE_ICONV
	size_t inlen = text.Length() + 1;
	
	iconv_t cd;
	cd = iconv_open("UTF-8",charset);
	if(cd == (iconv_t)(-1))
		return;
	size_t outlen = inlen*2;
	char *outbuf = new char[outlen];
	const char* inbuf = text.String();
	::memset(outbuf,0,outlen);
	char *outbufp = outbuf;
	size_t r = iconv(cd,&inbuf,&inlen,&outbufp,&outlen);
	if(r < 0)
	{
		delete[] outbuf;
		return;
	}
	text = outbuf;
	delete[] outbuf;
	iconv_close(cd);
#else
	int32 encoding = p_Encoding(charset);

	if(encoding < 0)
		return;
	ConvertToUTF8(text,encoding);
#endif
}

/***********************************************************
 * ConvertToUTF8
 ***********************************************************/
void
Encoding::ConvertToUTF8(char** text,int32 encoding)
{
	int32	sourceLen = strlen(*text);
	int32	destLen = 4 * sourceLen;
	
	char*	buf = new char[destLen];
	status_t	err = B_OK;
	int32		state = 0;
	
	err = convert_to_utf8(encoding, *text, &sourceLen, buf, &destLen, &state);
	
	for(register int32 i = 0; i < destLen; i++)
	{
		if(*(buf + i) == '\0'){ *(buf + i) = ' '; } 
	}
	
	buf[destLen] = '\0';
	if(!err){
		delete[] *text;
		*text = buf;
	}else
		delete[] buf;	
}

/***********************************************************
 * ConvertFromUTF8
 ***********************************************************/
void
Encoding::ConvertFromUTF8(char** text,int32 encoding)
{
	int32	sourceLen = strlen(*text);
	int32	destLen = 4 * sourceLen;
	
	char*	buf = new char[destLen];
	status_t	err = B_OK;
	int32		state = 0;

	err = convert_from_utf8(encoding, *text, &sourceLen, buf, &destLen, &state);
	buf[destLen] = '\0';
	
	if(!err){
		delete[] *text;
		*text = buf;
	}else
		delete[] buf;
}

/***********************************************************
 * ConvertToUTF8
 ***********************************************************/
void
Encoding::ConvertToUTF8(BString& text,int32 encoding)
{
	int32 len = text.Length();
	char *buf = new char[len+1];
	::strcpy(buf,text.String());
	
	ConvertToUTF8(&buf,encoding);
	
	text = buf;
}

/***********************************************************
 * ConvertFromUTF8
 ***********************************************************/
void
Encoding::ConvertFromUTF8(BString& text,int32 encoding)
{
	int32 len = text.Length();
	char *buf = new char[len+1];
	::strcpy(buf,text.String());
	
	ConvertFromUTF8(&buf,encoding);
	
	text = buf;
}

/***********************************************************
 * ConvertReturnCode
 ***********************************************************/
void
Encoding::ConvertReturnCode(char** text, int32 retcode)
{
	char* str = *text;
	const int32	len = strlen(str);
	
	if(retcode == M_CR){
		ConvertReturnsToCR(str);
	}else if(retcode == M_CRLF){
		int32	LFNum = 0;
		for(int32 i = 0; *(str + i) != '\0'; i++){
			if(*(str + i) == LF)
				LFNum++;
		}
		
		//printf("LFNum = %d\n", LFNum);
		
		char*	buf = new char[len + LFNum + 1];
		
		for(int32 i = 0, j = 0;; i++, j++){
			if(*(str + j) == LF){
				*(buf + i) = CR;
				*(buf + (++i)) = LF;
			}else{
				*(buf + i) = *(str + j);
			}
			
			if(*(str + j) == '\0')
				break;
		}
		
		delete[] *text;
		*text = buf;
	}else if(retcode == M_LF) {
		ConvertReturnsToLF(str);
	}
}

/***********************************************************
 * ConvertReturnCode
 ***********************************************************/
void
Encoding::ConvertReturnCode(BString &text, int32 retcode)
{
	char* str = new char[text.Length()+1];
	::strcpy(str,text.String());
	const int32	len = text.Length();
	str[len] = '\0';
	
	if(retcode == M_CR){
		ConvertReturnsToCR(text);
	}else if(retcode == M_CRLF){
		ConvertReturnsToLF(text);
		text.ReplaceAll("\n","\r\n");
	}else if(retcode == M_LF) {
		ConvertReturnsToLF(text);
	}
	delete[] str;
}

/***********************************************************
 * ConvertReturnsToLF
 ***********************************************************/
void
Encoding::ConvertReturnsToLF(char* text)
{	
	for(int32 i = 0, j = 0; true; i++, j++){
		if(*(text + i) == CR){
			*(text + j) = LF;
			if(*(text + i + 1) == LF)
				i++;
		}else{
			*(text + j) = *(text + i);
			if(*(text + j) == '\0')
				break;
		}
	}
}

/***********************************************************
 * ConvertReturnsToCR
 ***********************************************************/
void
Encoding::ConvertReturnsToCR(char* text)
{	
	for(int32 i = 0, j = 0; true; i++, j++){
		if(*(text + i) == LF){
			*(text + j) = CR;
			if(*(text + i + 1) == CR)
				i++;
		}else{
			*(text + j) = *(text + i);
			if(*(text + j) == '\0')
				break;
		}
	}
}

/***********************************************************
 * ConvertReturnsToLF
 ***********************************************************/
void
Encoding::ConvertReturnsToLF(BString &str)
{	
	char *text = new char[str.Length()+1];
	::strcpy(text,str.String());
	for(int32 i = 0, j = 0; true; i++, j++){
		if(*(text + i) == CR){
			*(text + j) = LF;
			if(*(text + i + 1) == LF)
				i++;
		}else{
			*(text + j) = *(text + i);
			if(*(text + j) == '\0')
				break;
		}
	}
	str = text;
	delete[] text;
}


/***********************************************************
 * ConvertReturnsToCR
 ***********************************************************/
void
Encoding::ConvertReturnsToCR(BString &str)
{	
	char *text = new char[str.Length()+1];
	::strcpy(text,str.String());
	for(int32 i = 0, j = 0; true; i++, j++){
		if(*(text + i) == LF){
			*(text + j) = CR;
			if(*(text + i + 1) == CR)
				i++;
		}else{
			*(text + j) = *(text + i);
			if(*(text + j) == '\0')
				break;
		}
	}
	str = text;
	delete[] text;
}

/***********************************************************
 * decode_quoted_printable
 ***********************************************************/
int32
Encoding::decode_quoted_printable(char *dest,char *in,off_t length,
								bool treat_underscore_as_space)
{
	char ch,c1,c2;
   	char *src = (char*)malloc(length+1);
   	::strncpy(src,in,length);
   	off_t len =0;
   	int32 k = 0;
   
   	while (len < length) 
   	{ 
		ch = src[len++]; 
        
        if(!ch)
        	break;
        if ((ch == '=') && (len + 1 < length) )
        {
        	c1 = unhex(src[len]);
        	c2 = unhex(src[len+1]);
        	len+=2;
        	if ((c1 > 15) || (c2 > 15)) 
   				continue;
   			ch = (16 * c1) + c2;
   			if(ch >= 0)
   				dest[k++] = ch;
	    }else if ((ch == '_') && treat_underscore_as_space){ 
            dest[k++] = ' '; 
        }else {
        	dest[k++] = ch;
   		}
   }
   dest[k] = '\0';
   free(src);
   return k; 
}

/***********************************************************
 * unhex
 ***********************************************************/
char 
Encoding::unhex(char c)
{
  if ((c >= '0') && (c <= '9'))
    return (c - '0');
  else if ((c >= 'A') && (c <= 'F'))
    return (c - 'A' + 10);
  else if ((c >= 'a') && (c <= 'f'))
    return (c - 'a' + 10);
  else
    return c;
}


/***********************************************************
 * FindCharset
 ***********************************************************/
const char*
Encoding::FindCharset(int32 conversion)
{
	for(int32 i = 0;i < kNumCharset;i++)
	{
		if(conversion == kEncodings[i])
			return kCharsets[i];
	}
	return kCharsets[0];
}

/***********************************************************
 * CountCharset
 ***********************************************************/
int32
Encoding::CountCharset() const
{
	return kNumCharset;
}

/***********************************************************
 * Charset
 ***********************************************************/
const char*
Encoding::Charset(int32 index) const
{
	return kCharsets[index];
}


/***********************************************************
 * Conversion
 ***********************************************************/
int32
Encoding::Conversion(int32 index) const
{
	return kEncodings[index];
}
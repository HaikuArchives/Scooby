#include "Encoding.h"
#include "HApp.h"
#include "HPrefs.h"

#include <ctype.h>
#include <Debug.h>
#include <E-mail.h>
#include <Alert.h>

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
	
	const char kJis_End[4] = {0x1b,0x28,0x42,'\0'};
	
	ConvertFromUTF8(inString,encoding);
	// For japanese jis support	
	if(encoding == B_JIS_CONVERSION)
		inString += kJis_End;
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
    outString += "?=";
    
    // Find encoding
    int32 encoding_index = -1;
    int32 count = sizeof(kCharsets)/sizeof(kCharsets[0]);
    for(int32 i = 0;i < count;i++)
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
    inString += kCharsets[encoding_index];
    inString += "?B?";
    inString += outString;
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
	int32 count = sizeof(kCharsets)/sizeof(kCharsets[0]);
	for(int32 i = 0;i < count;i++)
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
	key += kCharsets[encoding_index];
	key += "?";
	
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
			result += mime;
			mime = "";
		}else{
			if(!is_mime)
			{
				if(buf[i] != '\n' && buf[i] != '\r')
					result += buf[i];
				if(buf[i] == '\0')
					break;
			}else{
				if(buf[i] != '\n' && buf[i] != '\r')
					mime += buf[i];
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
#define USE_BASE64DECODER
void
Encoding::MimeDecode(BString &str,bool quoted_printable)
{
    int32 len = str.Length();
   const char kJisEnd[4] = {0x1b,0x28,0x42,'\0'};
   const char kMimeJisEnd[] = "GyhK";
   bool check_jis = false;
   
   char *old = str.LockBuffer(0);
   if(len == 0)
   		return;
   	char *buf = new char[len+1];
   // MIME-Q
   if(quoted_printable)
   		len = decode_quoted_printable(buf,old,len,false);	
   else	{
   // MIME-B
#ifndef USE_BASE64DECODER
		// Check whether it's jis charactor.
		if(old && len > 5 && strcmp(&old[len-4],kMimeJisEnd) == 0)
			check_jis = true;
 		//
 		len = decode_base64(buf, old, len,true); 
#else
		int i, iR;
		char a1, a2, a3, a4;
		i = 0; 
        iR = 0;
        ::strcpy(buf,old);
        buf[len] = '\0'; 
        while (1) { 
                if (i >= len) 
                    break;
                if(buf[i] == '\r'|| buf[i] == '\n')
                {
                	i++;
                	continue;
                }
                a1 = p_Charconv(buf[i]); 
                a2 = p_Charconv(buf[i+1]); 
                a3 = p_Charconv(buf[i+2]); 
                a4 = p_Charconv(buf[i+3]); 
                buf[iR] = (a1 << 2) | (a2 >>4);        
                buf[iR + 1] = (a2 << 4) | (a3 >>2); 
                buf[iR + 2] = (a3 << 6) | a4; 
                
                iR += 3;        
                i += 4; 
        } 
        len = iR;
#endif
    }
    buf[len] = '\0'; 
   	::strcpy(old,buf);
    
    str.UnlockBuffer();
 	
 	// If it's jis charactor, check it contain jis end.
 	if(check_jis)
 	{
 		if(buf && len > 4 && strcmp(&buf[len-3],kJisEnd) != 0)
 			str += kJisEnd;
 	}
 	//
 	delete[] buf;
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
    PRINT(("Invalid character[%c:%d]\n", c,c));
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
	int32 count = sizeof(kCharsets)/sizeof(kCharsets[0]);
	
	for(i = 0;i < count;i++)
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
	
	char*	buf = new char[destLen+1];
	
	if(!buf)
	{
		(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}

	status_t	err = B_OK;
	int32		state = 0;
	err = convert_to_utf8(encoding, *text, &sourceLen, buf, &destLen, &state);

	for(register int32 i = 0; i < destLen; i++)
		if(*(buf + i) == '\0'){ *(buf + i) = ' '; } 
	
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
	
	char*	buf = new char[destLen+1];
	if(!buf)
	{
		(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}
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
	int32	sourceLen = text.Length();
	int32	destLen = 4 * sourceLen;
	
	status_t	err = B_OK;
	int32		state = 0;
	
	char *buf = new char[destLen+1];
	
	if(!buf)
	{
		(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}
	char*	inBuf = text.LockBuffer(destLen+1);
	
	err = convert_to_utf8(encoding, inBuf, &sourceLen, buf, &destLen, &state);

	if(err != B_OK)
	{
		delete[] buf;
		text.UnlockBuffer();
		return;	
	}
	for(register int32 i = 0; i < destLen; i++)
	{
		if(*(buf + i) == '\0'){ *(buf + i) = ' '; } 
	}

	buf[destLen] = '\0';
	::strncpy(inBuf,buf,destLen);
	text.UnlockBuffer(destLen);
	delete[] buf;
}

/***********************************************************
 * ConvertFromUTF8
 ***********************************************************/
void
Encoding::ConvertFromUTF8(BString& text,int32 encoding)
{
	int32	sourceLen = text.Length();
	int32	destLen = sourceLen;
	
	status_t	err = B_OK;
	int32		state = 0;
	
	char *buf = new char[destLen+1];
	if(!buf)
	{
		(new BAlert("",_("Memory was exhausted"),_("OK"),NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}	
	
	char*	inBuf = text.LockBuffer(destLen+1);
	
	err = convert_from_utf8(encoding, inBuf, &sourceLen, buf, &destLen, &state);

	if(err != B_OK)
	{
		delete[] buf;
		text.UnlockBuffer();
		return;	
	}
	
	for(register int32 i = 0; i < destLen; i++)
		if(*(buf + i) == '\0'){ *(buf + i) = ' '; } 
	
	buf[destLen] = '\0';
	::strncpy(inBuf,buf,destLen);
	text.UnlockBuffer(destLen);
	delete[] buf;
}

/***********************************************************
 * ConvertReturnsToLF
 ***********************************************************/
void
Encoding::ConvertReturnsToLF(char* text)
{
	int32 len = ::strlen(text);
	
	for(int32 i = 0, j = 0; true; i++, j++){
		if(*(text + i) == CR){
			*(text + j) = LF;
			if(i<len-1 && *(text + i + 1) == LF)
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
		}else if(*(text+i) == CR){
			*(text + j) = CR;
			if(*(text+i+1) == LF)
				i++;
		}else{
			*(text + j) = *(text + i);
			if(*(text + j) == '\0')
				break;
		}
	}
}


/***********************************************************
 * ConvertReturnsToCRLF
 ***********************************************************/
void
Encoding::ConvertReturnsToCRLF(char** intext)
{	
	char *text = new char[::strlen(*intext)*2];
	char *in = *intext;
	for(int32 i = 0, j = 0; true; i++, j++){
		if(*(in + i) == LF)
		{
			*(text + j) = CR;
			*(text + j + 1) = LF;
			j++;
		}else if(*(in+i) == CR) {
			*(text + j) = CR;
			*(text + j + 1) = LF;
			j++;
			if(*(in + i + 1) == LF)
				i++;
		}else{
			*(text + j) = *(in + i);
			if(*(text + j) == '\0')
				break;
		}
	}
	
	delete[] *intext;
	*intext = text;
}

/***********************************************************
 * ConvertReturnsToCRLF
 ***********************************************************/
void
Encoding::ConvertReturnsToCRLF(BString &str)
{	
	int32 len = str.Length();
	char *text = new char[len*2];
	const char *in = str.String();
	for(int32 i = 0, j = 0; true; i++, j++){
		if(*(in + i) == LF)
		{
			*(text + j) = CR;
			*(text + j + 1) = LF;
			j++;
		}else if(*(in+i) == CR) {
			*(text + j) = CR;
			*(text + j + 1) = LF;
			j++;
			if(*(in + i + 1) == LF)
				i++;
		}else{
			*(text + j) = *(in + i);
			if(*(text + j) == '\0')
				break;
		}
	}
	
	str = text;
	delete[] text;
}

/***********************************************************
 * ConvertReturnsToLF
 ***********************************************************/
void
Encoding::ConvertReturnsToLF(BString &str)
{	
	char *text = str.LockBuffer(0);
	ConvertReturnsToLF(text);
	str.UnlockBuffer();	
}


/***********************************************************
 * ConvertReturnsToCR
 ***********************************************************/
void
Encoding::ConvertReturnsToCR(BString &str)
{	
	char *text = str.LockBuffer(0);
	ConvertReturnsToCR(text);
	str.UnlockBuffer();	
}

/***********************************************************
 * decode_quoted_printable
 ***********************************************************/

static int Index_Hex[128] = 
{ 
   -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, 
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, 
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, 
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1, 
     -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1, 
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, 
     -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1, 
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1 
}; 
#define HEX(c) (Index_Hex[(unsigned char)(c) & 0x7F])

int32
Encoding::decode_quoted_printable(char *dest,char *in,off_t length,
								bool treat_underscore_as_space)
{
	char ch;
	const char *allowed_in_qp = "0123456789ABCDEFabcdef";
   	char *src = (char*)malloc(length+1);
   	::strncpy(src,in,length);
   	off_t len =0;
   	int32 k = 0;
   	
   	while (len < length) 
   	{ 
		ch = src[len++]; 
        
        if(!ch)
        	break;
        if ((ch == '=') && (len + 1 < length) 
        		&& strchr(allowed_in_qp,src[len])
        		&& strchr(allowed_in_qp,src[len+1]))
        {
        	ch = (HEX(src[len]) << 4) | HEX(src[len+1]);
        	dest[k++] = ch;
   			len+=2;
   		}else if(ch == '=' && src[len] == '\r' && src[len+1] == '\n' ){
   			// Eliminate soft line feeds (CRLF)
   			// Do nothing
   			len+=2;
   		}else if((ch == '=') && (src[len] == '\r' || src[len] == '\n') ){
   			// Eliminate soft line feeds (CR or LF)
   			// Do nothing
   			len++;
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
 * FindCharset
 ***********************************************************/
const char*
Encoding::FindCharset(int32 conversion)
{
	int32 count = sizeof(kCharsets)/sizeof(kCharsets[0]);
	for(int32 i = 0;i < count;i++)
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
	return sizeof(kCharsets)/sizeof(kCharsets[0]);
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
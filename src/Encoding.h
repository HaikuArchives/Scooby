#ifndef __ENCODING_H__
#define __ENCODING_H__

#include <UTF8.h>
#include <String.h>
#include <map>

enum{
	M_CR = 1,
	M_LF,
	M_CRLF
};

class Encoding {
public:
					Encoding();
	virtual			~Encoding();
	
	static	int32	decode_quoted_printable(char *dest,char *in,off_t length,
								bool treat_underscore_as_apace);
	
		int32		DefaultEncoding()const{return fDefaultEncoding;}
			
		status_t	ISO2UTF8(BString &str,int32 &encoding);
	
			void	Mime2UTF8(BString &str);
			void	UTF82Mime(BString &str,int32 encoding);

			void	ConvertToUTF8(BString &text,const char* charset);
			void	ConvertFromUTF8(BString &text,const char* charset);
			
			void	ConvertToUTF8(char** text,const char* charset);
			void	ConvertFromUTF8(char** text,const char* charset);

			void	ConvertToUTF8(BString &text,int32 encoding );
			void	ConvertFromUTF8(BString &text,int32 encoding );
			
			void	ConvertToUTF8(char** text,int32 encoding );
			void	ConvertFromUTF8(char** text,int32 encoding);

			void	ConvertReturnsToLF(char* text);
			void	ConvertReturnsToLF(BString &text);
			
			void	ConvertReturnsToCR(char* text);
			void	ConvertReturnsToCR(BString &text);
			
			void	ConvertReturnsToCRLF(char* text); // You have to prepare enough buffer
			void	ConvertReturnsToCRLF(char** text);
			void	ConvertReturnsToCRLF(BString &text);
			
	const char*		FindCharset(int32 conversion);
	const char*		Charset(int32 index) const;
			int32	Conversion(int32 index) const;
			int32	CountCharset() const;

			void	MimeDecode(BString &str,bool quoted_printable);

protected:
			char	p_Charconv(char c);
			bool	p_IsMultiByte(char c);
			int32 	p_Encoding(const char *charset);
			void	ToMime(BString &in, int32 encoding);
	static	char	unhex(char c);
private:		
			int32	fDefaultEncoding;
};
#endif
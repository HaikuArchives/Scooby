#ifndef __ENCODING_H__
#define __ENCODING_H__

#include <UTF8.h>
#include <String.h>
#include <map>

#define B_UTF8_CONVERSION -1

//!Convert text to and from the Unicode UTF-8 encoding.
class Encoding {
public:
			//! Constructor.
					Encoding();
			//!	Decode quoted-printable. It returns size of dest buffer.
	static	int32	decode_quoted_printable(char *dest //!< Output buffer.
											,char *in	//!< Input buffer.
											,off_t length //!< Input buffer length.
											,bool treat_underscore_as_apace //!< Whether treat underscore as space or not.
											);
			//! Return dafult encoding stored in preference.
		int32		DefaultEncoding()const{return fDefaultEncoding;}
			//!	Convert mime encoded text to UTF8
			void	Mime2UTF8(BString &str);
			//!	Convert UTF8 to mime base64.
			void	UTF82Mime(BString &str,int32 encoding);
			//@{
			//!	Convert UTF8 to another encoding.
			void	ConvertToUTF8(BString &text,const char* charset);
			void	ConvertToUTF8(char** text,const char* charset);
			void	ConvertToUTF8(BString &text,int32 encoding );
			void	ConvertToUTF8(char** text,int32 encoding );
			//@}
			//@{
			//! Convert text to UTF8 encoding.
			void	ConvertFromUTF8(BString &text,const char* charset);
			void	ConvertFromUTF8(char** text,const char* charset);
			void	ConvertFromUTF8(BString &text,int32 encoding );
			void	ConvertFromUTF8(char** text,int32 encoding);
			//@}
			//@{
			//! Convert all returns to line feeds.
			void	ConvertReturnsToLF(char* text);
			void	ConvertReturnsToLF(BString &text);
			//@}
			//@{
			//! Convert all returns to caridge retrns.
			void	ConvertReturnsToCR(char* text);
			void	ConvertReturnsToCR(BString &text);
			//@}
			//@{
			//!	Convert all returns to CR-LF.
			void	ConvertReturnsToCRLF(char** text);
			void	ConvertReturnsToCRLF(BString &text);
			//@}
	
			//! Get character set string by BeOS internal encoding name. If could not find charset, it returns ISO-8859-1.
	const char*		FindCharset(int32 conversion/*!<BeOS internal encoding name like B_ISO1_CONVERSION.*/);
			//! Get character set string by index.
	const char*		Charset(int32 index) const;
			//! Return BoOS internal encoding name by index.
			int32	Conversion(int32 index) const;
			//! Return supported conversion count.
			int32	CountCharset() const;
			//! Decode base64 or quoted-printable to original encoded text.
			void	MimeDecode(BString &str,bool quoted_printable);

protected:
			//!	Convert any encodings to UTF8
			status_t	ISO2UTF8(BString &str,int32 &encoding);
			//! Check whether input character is multibyte.
			bool	p_IsMultiByte(char c);
			//!	Guess BeOS internal encoding name by charset string.
			int32 	p_Encoding(const char *charset);
			//!  Convert text to base64
			void	ToMime(BString &in, int32 encoding);
private:		
			//! Default encoding stored in preference.
			int32	fDefaultEncoding;
};
#endif

#ifndef __UTF7_H__
#define __UTF7_H__

#ifdef __cplusplus
extern "C"{
#endif

//! Convert IMAP4 modified UTF7 to UTF8
void IMAP4UTF72UTF8(char *dst, char *src);
//! Convert UTF8 to IMAP4 modified UTF7
void UTF8IMAP4UTF7(char *dst, char *src);

#ifdef __cplusplus
};
#endif
#endif /* __UTF7_H__ */

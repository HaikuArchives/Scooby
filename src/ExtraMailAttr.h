#ifndef __EXTRAMAILATTR_H__
#define __EXTRAMAILATTR_H__

//!Extra mail attributes.
 
//! SMTP server attribute for outgoing mails.
#define B_MAIL_ATTR_SMTP_SERVER "MAIL:smtp_server"
//! BCC attribute for outgoing mails.
#define B_MAIL_ATTR_BCC "MAIL:bcc"
//! Attachment attribute. If it has attachment, value is 1.
#define B_MAIL_ATTR_ATTACHMENT "MAIL:attachment"
//! Account name attribute.
#define B_MAIL_ATTR_ACCOUNT "MAIL:account"
//! Use smtp auth or not.
#define B_MAIL_ATTR_SMTP_AUTH "MAIL:smtp_auth"
#endif

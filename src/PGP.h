#ifndef __PGPIF_H__
#define __PGPIF_H__

#include <BeBuild.h>
#include <Path.h>
#include <String.h>


/**
 *	PGP class
 */
class PGP {
public:
				//!Constructor.
							PGP();
							~PGP();

				bool		CanUse() const; //!<Returns true if yuu can use pgp command.
				
				//! PGP sign
				status_t	Sign(const char* passphrase,const char* in,BString *out);
				//! PGP Encrypt
				status_t	Encrypt(const char* userid,
									const char* in, 
									BString *out,
									bool sign=false,
									const char* passphrase=NULL);
				//! PGP Decrypt
				status_t	Decrypt(const char* passphrase,const char* in, BString *out);		
				//! Add PGP key
				status_t	AddKey(const char* key);
				//! Get PGP public key
				status_t	GetPublicKey(const char* keyname,BString *out);
protected:
				status_t	CreateInputFilePointer(BFile* file);
				status_t	CreateOutputFilePointer(BFile* file);
					void	DeleteFiles();
					
				status_t	ProcessCommand(const char* cmd,const char* in,BString *out);
private:
			BPath 			fTmpDirPath; //!<Temporary directory path.
};
#endif
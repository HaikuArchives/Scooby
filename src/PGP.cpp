#include "PGP.h"

#include <File.h>
#include <FindDirectory.h>
#include <stdlib.h>
#include <Debug.h>

//! Input and Output file names.
#define INPUT_FILENAME "ScoobyPGPTmp"
#define OUTPUT_FILENAME "ScoobyPGPTmp.asc"

/***********************************************************
 * Constructor
 ***********************************************************/
PGP::PGP()
{
	// Store tmp directory path
	::find_directory(B_COMMON_TEMP_DIRECTORY,&fTmpDirPath);
}

/***********************************************************
 * Desturctor
 ***********************************************************/
PGP::~PGP()
{
	DeleteFiles();
}

/***********************************************************
 * CanUse: Check PGP commands have been installed.
 ***********************************************************/
bool
PGP::CanUse() const
{
	BPath path;
	::find_directory(B_USER_DIRECTORY,&path);
	path.Append("config/bin/pgp");
	BFile file(path.Path(),B_READ_ONLY);
	return (file.InitCheck() == B_OK)?true:false;
}

/***********************************************************
 * Encrypt
 ***********************************************************/
status_t
PGP::Encrypt(const char* userid,const char* in, BString *out,bool sign,const char* passphrase)
{
	if(sign && strlen(passphrase) ==0)
		return B_ERROR;
	
	BPath path(fTmpDirPath);
	path.Append(INPUT_FILENAME);

	BString cmd("pgpe ");
	cmd += path.Path();
	cmd += " -ta -r ";
	
	int len = ::strlen(userid);
	for(int32 i = 0;i < len;i++)
	{
		if(userid[i] == ',')
			cmd += " -r ";
		else
			cmd += userid[i];
	}
	cmd += " -z ";
	cmd += passphrase;
	if(sign)
		cmd += " -s";
	
	return ProcessCommand(cmd.String(),in,out);
}

/***********************************************************
 * Decrypt
 ***********************************************************/
status_t
PGP::Decrypt(const char* passphrase,const char* in,BString *out)
{	
	BPath path(fTmpDirPath);
	path.Append(INPUT_FILENAME);

	BString cmd("pgpv ");
	cmd += path.Path();
	cmd += " -z ";
	cmd += passphrase;
	
	return ProcessCommand(cmd.String(),in,out);
}

/***********************************************************
 * Sign
 ***********************************************************/
status_t
PGP::Sign(const char* passphrase,const char* in,BString *out)
{
	BPath path(fTmpDirPath);
	path.Append(INPUT_FILENAME);
	
	BString cmd("pgps ");
	cmd += path.Path();
	cmd += " -t -z ";
	cmd += passphrase;
	
	return ProcessCommand(cmd.String(),in,out);
}

/***********************************************************
 * AddKey
 ***********************************************************/
status_t
PGP::AddKey(const char* key)
{
	BPath path(fTmpDirPath);
	path.Append(INPUT_FILENAME);
	
	BString cmd;
	cmd += "pgpk -a ";
	cmd += path.Path();
	
	return ProcessCommand(cmd.String(),key,NULL);
}

/***********************************************************
 * GetPublicKey
 ***********************************************************/
status_t
PGP::GetPublicKey(const char* keyname,BString *out)
{
	BPath path(fTmpDirPath);
	path.Append(OUTPUT_FILENAME);
	
	BString cmd;
	cmd += "pgpk -xa ";
	cmd += keyname;
	cmd += " -o";
	cmd += path.Path();
	
	return ProcessCommand(cmd.String(),NULL,out);
}

/***********************************************************
 * CreateInputFile
 ***********************************************************/
status_t
PGP::CreateInputFilePointer(BFile *file)
{
	BPath path(fTmpDirPath);
	path.Append(INPUT_FILENAME);
	return file->SetTo(path.Path(),B_WRITE_ONLY|B_CREATE_FILE);
}

/***********************************************************
 * CreateOutputFile
 ***********************************************************/
status_t
PGP::CreateOutputFilePointer(BFile* file)
{
	BPath path(fTmpDirPath);
	path.Append(OUTPUT_FILENAME);
	return file->SetTo(path.Path(),B_READ_ONLY);
}

/***********************************************************
 * DeleteFiles: Delete tmp files
 ***********************************************************/
void
PGP::DeleteFiles()
{
	BPath inPath(fTmpDirPath.Path());
	inPath.Append(INPUT_FILENAME);
	BPath outPath(fTmpDirPath.Path());
	outPath.Append(OUTPUT_FILENAME);
	
	BString cmd;
	cmd = "rm -rf ";
	cmd += inPath.Path(); 
	cmd += " ";
	cmd += outPath.Path();
	::system(cmd.String());
}

/***********************************************************
 * ProcessCommand
 ***********************************************************/
status_t
PGP::ProcessCommand(const char* cmd ,const char* in,BString *out)
{
	BFile file;
	status_t err = B_OK;	
	// Delete tmp files
	DeleteFiles();
	
	// Create an input tmp file
	if(strlen(in) > 0)
	{
		PRINT(("PGPIN:%s\n",in));
		if(CreateInputFilePointer(&file) != B_OK)
			return B_ERROR;
		file.Write(in, ::strlen(in));
		file.Unset();
	}	
	// Excute pgp command
	PRINT(("PGPCMD:%s\n",cmd));
	::system(cmd);
	
	if(out)
	{
		// Create output file pointer
		err = CreateOutputFilePointer(&file);
		// Read output file
		if(err == B_OK)
		{
			off_t size;
			file.GetSize(&size);
			if(size != 0)
			{
				out->SetTo("");
				char *buf = out->LockBuffer(size+1);
				size = file.Read(buf,size);
				buf[size] = '\0';
				out->UnlockBuffer();
			}
		}
		PRINT(("PGPOUT:%s\n",out->String()));
	}
	// Delete tmp files
	DeleteFiles();
	return err;	
}
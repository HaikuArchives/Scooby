#ifndef __HFILE_H__
#define __HFILE_H__

#include <File.h>
#include <String.h>

class HFile :public BFile{
public:
						HFile();
						HFile(const entry_ref *ref, uint32 open_mode);
						HFile(const BEntry *entry, uint32 open_mode);
						HFile(const char *path, uint32 open_mode);
						HFile(const BDirectory *dir, const char *path,
							  uint32 open_mode);
						HFile(const BFile &file);

	virtual				~HFile();

			int32		GetLine(BString *out);
			int32		GetLine(char *buf);
			
			ssize_t		ReadAll(BString *out);
			ssize_t		ReadAll(char **buf);
			
			ssize_t		WriteAll(BString str);
			ssize_t		WriteAll(const char* buf);
protected:

};
#endif
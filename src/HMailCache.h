#ifndef __HMAILCACHE_H__
#define __HMALICACHE_H__

#include <String.h>
#include <DataIO.h>

class HFolderItem;

class HMailCache {
public:
							HMailCache(const char* path);
	virtual					~HMailCache();

			status_t		Open(BList &out,HFolderItem *item);	
			status_t		Save(BList &list);
			
			status_t		Append(BList &list);
protected:
					void	ReadString(BMemoryIO &buf,char** out);
					void	WriteString(BMallocIO &buf,const char* str);
					void	AddMails(BList &list,BMallocIO &buf);
			status_t		SaveToFile(BMallocIO &buf,BFile &file);
private:
	char					*fPath;
};
#endif
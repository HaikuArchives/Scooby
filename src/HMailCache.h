#ifndef __HMAILCACHE_H__
#define __HMALICACHE_H__

#include <String.h>
#include <DataIO.h>

class BList;
class BFile;

class HFolderItem;

//!Mails cache class.
class HMailCache {
public:
				//!Constructor.
							HMailCache(const char* path);
				//!Destructor.
							~HMailCache();

			status_t		Open(HFolderItem *item);	
			status_t		Save(BList &list);
			
				int32		CountItems();
			status_t		Append(BList &list);
protected:
					void	ReadString(BMemoryIO &buf,char** out);
					void	WriteString(BMallocIO &buf,const char* str);
					
					void	ReadInt32(BMemoryIO &buf,int32 *out);
					void	WriteInt32(BMallocIO &buf,int32 &inInt);
					
					void	AddMails(BList &list,BMallocIO &buf);
			status_t		SaveToFile(BMallocIO &buf,BFile &file);
private:
	char					*fPath;
};
#endif

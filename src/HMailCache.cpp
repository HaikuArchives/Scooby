#include "HMailCache.h"
#include "HMailItem.h"
#include "HFolderItem.h"

#include <File.h>
#include <Entry.h>
#include <Debug.h>
#include <NodeMonitor.h>
#include <ListView.h>
#include <stdlib.h>


#define VERSION 1

typedef struct{
	int32 version;
	int32 count;
}HEADER;

#define HEADER_SIZE 8

typedef struct{
	dev_t dev;
	ino_t	dir;
}ENTRY_REF_DATA;

#define ENTRY_REF_DATA_SIZE 12

typedef struct{
	int8 enclosure;
	ino_t	node;
	int64	when;
}INTEGER_DATA;

#define INTEGER_DATA_SIZE 17

/***********************************************************
 * Constructor
 ***********************************************************/
HMailCache::HMailCache(const char* path)
{
	fPath = new char[strlen(path)+1];
	::strcpy(fPath,path);	
}

/***********************************************************
 * Destructor
 ***********************************************************/
HMailCache::~HMailCache()
{
	delete[] fPath;
}

/***********************************************************
 * Open
 ***********************************************************/
status_t
HMailCache::Open(BList &outList,HFolderItem *folder)
{
	BFile file(fPath,B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return B_ERROR;
	off_t size;
	file.GetSize(&size);
	char *buf = new char[size+1];
	if(!buf)
	{
		PRINT(("Memory exhausted\n"));
		return B_ERROR;
	}
	size = file.Read(buf,size);
	buf[size] = '\0';
	BMemoryIO memory(buf,size);
	
	HEADER header;
	memory.Read(&header,HEADER_SIZE);
	
	if(header.version != VERSION)
		return B_ERROR;
	int32 count = header.count;
	//PRINT(("Count:%d\n",count));
	INTEGER_DATA int_data;
	ENTRY_REF_DATA ref_data;
	HMailItem *item;
	int32 unread = 0;
	char *name,*status,*subject,*from,*to,*cc,*reply,*priority;
	
	for(register int32 i = 0;i < count;i++)
	{
		memory.Read(&int_data,INTEGER_DATA_SIZE);
		memory.Read(&ref_data,ENTRY_REF_DATA_SIZE);
		
		ReadString(memory,&name);
		entry_ref ref(ref_data.dev,ref_data.dir,name);
		delete[] name;
		ReadString(memory,&status);
		ReadString(memory,&subject);
		ReadString(memory,&from);
		ReadString(memory,&to);
		ReadString(memory,&cc);
		ReadString(memory,&reply);
		ReadString(memory,&priority);
		
		outList.AddItem(item = new HMailItem(ref,status,subject,from,to,cc,reply,
									int_data.when,priority,int_data.enclosure,
									int_data.node));
		delete[] status;
		delete[] subject;
		delete[] from;
		delete[] to;
		delete[] cc;
		delete[] reply;
		delete[] priority;
		
		::watch_node(&item->fNodeRef,B_WATCH_ATTR,(BHandler*)folder,(BLooper*)folder->Owner()->Window());
		if(!item->IsRead())
			unread++;
	}
	folder->SetName(unread);
	delete[] buf;
	return B_OK;
}

/***********************************************************
 * ReadString
 ***********************************************************/
void
HMailCache::ReadString(BMemoryIO &buf,char** out)
{
	int32 len;
	buf.Read(&len,4);
	char *str = new char[len+1];
	buf.Read(str,len);
	str[len] = '\0';
	*out = str;
	//PRINT(("%s\n",str));
}

/***********************************************************
 * Save
 ***********************************************************/
status_t
HMailCache::Save(BList &list)
{
	HEADER header;
	header.version = VERSION;
	header.count = list.CountItems();
	
	BMallocIO buf;
	buf.Write(&header,HEADER_SIZE);
	AddMails(list,buf);
	
	BFile file(fPath,B_WRITE_ONLY|B_CREATE_FILE);
	return SaveToFile(buf,file);
}

/***********************************************************
 * WriteString
 ***********************************************************/
void
HMailCache::WriteString(BMallocIO &buf,const char* str)
{
	int32 len = strlen(str);
	
	buf.Write(&len,4);
	buf.Write(str,len);
}

/***********************************************************
 * Append
 ***********************************************************/
status_t
HMailCache::Append(BList &list)
{
	BMallocIO buf;
	AddMails(list,buf);
	
	BFile file(fPath,B_WRITE_ONLY);
	if(file.InitCheck() != B_OK)
		return B_ERROR;
	off_t size;
	file.GetSize(&size);
	HEADER header;
	file.Read(&header,HEADER_SIZE);
	header.count += list.CountItems();
	file.Write(&header,HEADER_SIZE);
	file.Seek(0,SEEK_END);
	file.Write(buf.Buffer(),buf.BufferLength());
	size += buf.BufferLength();
	file.SetSize(size);
	return B_OK;
}

/***********************************************************
 * AddMails
 ***********************************************************/
void
HMailCache::AddMails(BList &list,BMallocIO &buf)
{
	int32 count = list.CountItems();
	INTEGER_DATA int_data;
	ENTRY_REF_DATA ref_data;
	
	HMailItem *item;
	for(register int32 i = 0;i < count;i++)
	{
		item = (HMailItem*)list.ItemAt(i);
		if(!item)
			continue;
		int_data.enclosure = item->fEnclosure;
		int_data.node= item->fNodeRef.node;
		int_data.when = item->fWhen;
		buf.Write(&int_data,INTEGER_DATA_SIZE);
		ref_data.dev = item->fRef.device;
		ref_data.dir = item->fRef.directory;
		buf.Write(&ref_data,ENTRY_REF_DATA_SIZE);
	
		WriteString(buf,item->fRef.name);
		WriteString(buf,item->fStatus.String());
		WriteString(buf,item->fSubject.String());
		WriteString(buf,item->fFrom.String());
		WriteString(buf,item->fTo.String());
		WriteString(buf,item->fCC.String());
		WriteString(buf,item->fReply.String());
		WriteString(buf,item->fPriority.String());
	}
}

/***********************************************************
 * SaveToFile
 ***********************************************************/
status_t
HMailCache::SaveToFile(BMallocIO &buf,BFile &file)
{
	if(file.InitCheck() != B_OK)
		return B_ERROR;
	int32 len = buf.BufferLength();
	file.Write(buf.Buffer(),len);
	file.SetSize(len);
	return B_OK;
}

/***********************************************************
 * CountItems
 ***********************************************************/
int32
HMailCache::CountItems()
{
	BFile file(fPath,B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return -1;
	HEADER header;
	file.Read(&header,HEADER_SIZE);
	return header.count;
}
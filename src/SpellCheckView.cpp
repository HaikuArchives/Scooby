#include "SpellCheckView.h"
#include <Debug.h>
#include <Looper.h>
#include <ctype.h>
#include <Directory.h>
#include <File.h>
#include <Entry.h>

const rgb_color kSpellErrorColor = {250,0,0,255};
const rgb_color kNormalColor = {0,0,0,255};

#define DIC_DIR "/boot/beos/etc/word_dictionary"

/***********************************************************
 * Constructor
 ***********************************************************/
SpellCheckView::SpellCheckView(BRect frame,
							const char* name,
							//BRect textRect,
							uint32 resize,
							uint32 flags)
	:_inherited(frame,name,/*textRect,*/resize,flags)
	,fCheckThreadID(-1)
	,fEnabled(false)
{
	SetStylable(true);
	InitDictionaries();
}

/*
SpellCheckView::SpellCheckView(BRect		frame, 
							const char		*name,
							// BRect			textRect,
						 	const BFont		*initialFont,
							const rgb_color	*initialColor, 
							 uint32			resizeMask, 
							 uint32			flags)
	:_inherited(frame,name,textRect,initialFont,initialColor,resizeMask,flags)
	,fCheckThreadID(-1)
	,fEnabled(false)
{
	SetStylable(true);
	InitDictionaries();
}
*/
/***********************************************************
 * Destructor
 ***********************************************************/
SpellCheckView::~SpellCheckView()
{
	for(int32 i = 0;i < 27;i++)
	{
		BList *list = (BList*)fWordList.RemoveItem(26-i);
		
		int32 count = list->CountItems();
		if(count <=0)
			continue;
		for(int32 j = 0;j < count;j++)
			free( list->RemoveItem(count-j-1) );
		delete list;
	}
}

/***********************************************************
 * InsertText
 ***********************************************************/
void
SpellCheckView::InsertText(const char				*inText, 
						   int32					inLength, 
						   int32					inOffset,
						   const text_run_array		*inRuns)
{
	// reset colors
	text_run_array array;
	array.count = 1;
	array.runs[0].color = kNormalColor;
	array.runs[0].offset = inOffset;
	//
	_inherited::InsertText(inText,inLength,inOffset,&array);
	
	bool check = false;
	// Normal input action
	if(inLength == 1)
	{
		if(!isalpha(inText[0]))
		{
			inOffset--;
			check = true;
		}
	}else if(inLength > 1){
		// Paste action and so on
		PRINT(("Paste:%s\n",inText));
		check = true;
	}
	int32 start=0,end=0;
	FindWord(inOffset,&start,&end);
	//if(start != inOffset)
	//	start = inOffset - start;
	end = inOffset+inLength;
	PRINT(("%d %d %d\n",start,end,inOffset));
	if(check && end-start > 1)
		StartChecking(start,end);	
}

/***********************************************************
 * DeleteText
 ***********************************************************/
void
SpellCheckView::DeleteText(int32 fromOffset, int32 toOffset)
{
	_inherited::DeleteText(fromOffset,toOffset);
	int32 start=0,end=0;
	FindWord((fromOffset >0)?fromOffset-1:fromOffset,&start,&end);
	PRINT(("DEL:%d %d %d %d\n",start,end,toOffset,fromOffset));
	if(end-start > 1)
		StartChecking(start,end);	
}

/***********************************************************
 * CheckWord
 ***********************************************************/
bool
SpellCheckView::CheckWord(const char* word)
{
	PRINT(("TEXT:%s\n",word ));
	if(isalpha(word[0]))
	{
		int32 index = tolower(word[0]) - 'a';
		BList *list = (BList*)fWordList.ItemAt(index);
		if(!list)
			return true;
		int32 count = list->CountItems();
	
		while(count>0)
		{
			const char *p = (const char*)list->ItemAt(--count);
			//PRINT(( "%s\n",p));
			if( ::strcasecmp(p,word) == 0)
				return true;
		}
	}else
		return true;
	return false;
}

/***********************************************************
 * StartChecking
 ***********************************************************/
void
SpellCheckView::StartChecking(int32 start,int32 end)
{
	if(!fEnabled)
		return;
	if(fCheckThreadID >= 0)
	{
		status_t err;
		wait_for_thread(fCheckThreadID,&err);
	}
	CheckThreadData *data = new CheckThreadData;
	data->start = start;
	data->end = end;
	data->view = this;
	
	fCheckThreadID = ::spawn_thread(CheckThread,"SpellCheckThread",B_NORMAL_PRIORITY,data);
	::resume_thread(fCheckThreadID);
}

/***********************************************************
 * CheckThread
 ***********************************************************/
int32
SpellCheckView::CheckThread(void* data)
{
	CheckThreadData *checkData = (CheckThreadData*)data;
	
	int32 start = checkData->start;
	int32 end = checkData->end;
	int32 len = end - start;
	SpellCheckView *view = checkData->view;
	
	if(view->Looper()->Lock() )
	{
		BString word;
		char *buf = new char[len+1];
		
		view->GetText(start,len,buf);
		
		int32 len = ::strlen(buf);
		int32 old_pos = start;
		for(int32 i = 0;i < len;i++)
		{
			if(!isalpha(buf[i]) || i == len-1)
			{
				if(i == len -1 && isalpha(buf[i]))
					word += buf[i];
				if(!view->CheckWord(word.String()))
					view->SetColor(old_pos,old_pos+word.Length(),(rgb_color*)&kSpellErrorColor);
				else
					view->SetColor(old_pos,old_pos+word.Length(),(rgb_color*)&kNormalColor);
				
				old_pos = i+1;
				word = "";
			}else
				word += buf[i];
		}
		delete[] buf;
		view->Looper()->Unlock();
	}
	view->fCheckThreadID = -1;
	delete data;
}

/***********************************************************
 * IsSameColor
 ***********************************************************/
bool
SpellCheckView::IsSameColor(rgb_color c1 ,rgb_color c2)
{
	if(c1.red == c2.red
			&& c1.green == c2.green
			&& c1.blue == c2.blue
			&& c1.alpha == c2.alpha){
		return true;
	}
	return false;
}

/***********************************************************
 * SetColor
 ***********************************************************/
void
SpellCheckView::SetColor(int32 startOffset,int32 endOffset,rgb_color *col)
{
	BFont font;
	GetFontAndColor(startOffset,&font);
	PRINT(("SetColor:%d %d\n",startOffset,endOffset));
	SetFontAndColor(startOffset, endOffset,&font,B_FONT_FAMILY_AND_STYLE,col);
}

/***********************************************************
 * InitDictionaries
 ***********************************************************/
void
SpellCheckView::InitDictionaries()
{
	for(int32 i = 0;i < 27;i++)
	{
		BList *list = new BList();
		list->MakeEmpty();
		fWordList.AddItem( list );
	}
	BDirectory dir(DIC_DIR);
	BEntry entry;
	
	while(dir.GetNextEntry(&entry,true) == B_OK)
	{
		BFile file(&entry,B_READ_ONLY);
		
		off_t size;
		file.GetSize(&size);
		char *all = new char[size+1];
		if(!all){
			return;
		}
		size = file.Read(all,size);
		char *p = strtok(all,"\n");
		while(p)
		{
			int16 index;
			index = tolower(p[0]) - 'a';
			BList *list = (BList*)fWordList.ItemAt(index);
			if(list && strlen(p)>0)
				list->AddItem(strdup(p));
			// for option
			char *q = strstr(p,"/");
			if(q != NULL)
			{
				char *tmp = (char*)list->LastItem();
				tmp[ strlen(tmp)- strlen(q) ] = '\0';
				q++;
				int32 oplen = strlen(q);
				for(int32 k = 0;k < oplen;k++)
				{
					switch(*q++)
					{
					// IVE
					case 'V':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+3];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"ive");
							if(list)
								list->AddItem(s);
						}else{
							char* s = new char[strlen(tmp)+5];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"ive");
							
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// ION
					case 'N':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"tion");
							if(list)
								list->AddItem(s);
						}else if(	tmp[strlen(tmp)-1] == 'y' )
						{
							char* s = new char[strlen(tmp)+5];
							strcpy(s,tmp);
							s[strlen(s)-1] = 'i';
							strcat(s,"tion");
							if(list)
								list->AddItem(s);
						}else{
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"en");
							
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// IONS
					case 'X':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"tions");
							if(list)
								list->AddItem(s);
						}else if(	tmp[strlen(tmp)-1] == 'y' )
						{
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							s[strlen(s)-1] = 'i';
							strcat(s,"tions");
							if(list)
								list->AddItem(s);
						}else{
							char* s = new char[strlen(tmp)+45];
							strcpy(s,tmp);
							strcat(s,"ens");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// IETH
					case 'H':
					{
						if(	tmp[strlen(tmp)-1] == 'y' )
						{
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							s[strlen(s)-1] = 'i';
							strcat(s,"ETH");
							if(list)
								list->AddItem(s);
						}else{
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"th");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// LY
					case 'Y':
					{
						char* s = new char[strlen(tmp)+4];
						strcpy(s,tmp);
						strcat(s,"ly");
						if(list)
							list->AddItem(s);
						break;
					}
					// ING
					case 'G':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+3];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"ing");
							if(list)
								list->AddItem(s);
						}else{
							char* s = new char[strlen(tmp)+5];
							strcpy(s,tmp);
							strcat(s,"ing");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// INGS
					case 'J':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"ings");
							if(list)
								list->AddItem(s);
						}else{
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							strcat(s,"ings");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// D
					case 'D':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+3];
							strcpy(s,tmp);
							strcat(s,"d");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&& tmp[strlen(tmp)-2] != 'a'
								&& tmp[strlen(tmp)-2] != 'e'
								&& tmp[strlen(tmp)-2] != 'i'
								&& tmp[strlen(tmp)-2] != 'o'
								&& tmp[strlen(tmp)-2] != 'u'
								){
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"ied");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] != 'e' 
								&&tmp[strlen(tmp)-1] != 'y'
								){
							char* s = new char[strlen(tmp)+3];
							strcpy(s,tmp);
							strcat(s,"ed");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&&( tmp[strlen(tmp)-2] == 'a'
								|| tmp[strlen(tmp)-2] == 'e'
								|| tmp[strlen(tmp)-2] == 'i'
								|| tmp[strlen(tmp)-2] == 'o'
								|| tmp[strlen(tmp)-2] == 'u'
								)){
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"ed");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// ST
					case 'T':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+3];
							strcpy(s,tmp);
							strcat(s,"st");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&& tmp[strlen(tmp)-2] != 'a'
								&& tmp[strlen(tmp)-2] != 'e'
								&& tmp[strlen(tmp)-2] != 'i'
								&& tmp[strlen(tmp)-2] != 'o'
								&& tmp[strlen(tmp)-2] != 'u'
								){
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"iest");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] != 'e' 
								&&tmp[strlen(tmp)-1] != 'y'
								){
							char* s = new char[strlen(tmp)+5];
							strcpy(s,tmp);
							strcat(s,"est");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&&( tmp[strlen(tmp)-2] == 'a'
								|| tmp[strlen(tmp)-2] == 'e'
								|| tmp[strlen(tmp)-2] == 'i'
								|| tmp[strlen(tmp)-2] == 'o'
								|| tmp[strlen(tmp)-2] == 'u'
								)){
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"est");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// R
					case 'R':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+3];
							strcpy(s,tmp);
							strcat(s,"r");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&& tmp[strlen(tmp)-2] != 'a'
								&& tmp[strlen(tmp)-2] != 'e'
								&& tmp[strlen(tmp)-2] != 'i'
								&& tmp[strlen(tmp)-2] != 'o'
								&& tmp[strlen(tmp)-2] != 'u'
								){
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"ier");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] != 'e' 
								&&tmp[strlen(tmp)-1] != 'y'
								){
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"er");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&&( tmp[strlen(tmp)-2] == 'a'
								|| tmp[strlen(tmp)-2] == 'e'
								|| tmp[strlen(tmp)-2] == 'i'
								|| tmp[strlen(tmp)-2] == 'o'
								|| tmp[strlen(tmp)-2] == 'u'
								)){
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"er");
							if(list)
								list->AddItem(s);
						}
						break;
					} 
					// RS
					case 'Z':
					{
						if(	tmp[strlen(tmp)-1] == 'e' )
						{
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"rs");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&& tmp[strlen(tmp)-2] != 'a'
								&& tmp[strlen(tmp)-2] != 'e'
								&& tmp[strlen(tmp)-2] != 'i'
								&& tmp[strlen(tmp)-2] != 'o'
								&& tmp[strlen(tmp)-2] != 'u'
								){
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"iers");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] != 'e' 
								&&tmp[strlen(tmp)-1] != 'y'
								){
							char* s = new char[strlen(tmp)+5];
							strcpy(s,tmp);
							strcat(s,"ers");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&&( tmp[strlen(tmp)-2] == 'a'
								|| tmp[strlen(tmp)-2] == 'e'
								|| tmp[strlen(tmp)-2] == 'i'
								|| tmp[strlen(tmp)-2] == 'o'
								|| tmp[strlen(tmp)-2] == 'u'
								)){
							char* s = new char[strlen(tmp)+5];
							strcpy(s,tmp);
							strcat(s,"ers");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// S
					case 'S':
					{
						if(	tmp[strlen(tmp)-1] == 's' 
							||tmp[strlen(tmp)-1] == 'x'
							||tmp[strlen(tmp)-1] == 'z'
							||tmp[strlen(tmp)-1] == 'h'
							)
						{
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"es");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&& tmp[strlen(tmp)-2] != 'a'
								&& tmp[strlen(tmp)-2] != 'e'
								&& tmp[strlen(tmp)-2] != 'i'
								&& tmp[strlen(tmp)-2] != 'o'
								&& tmp[strlen(tmp)-2] != 'u'
								){
							char* s = new char[strlen(tmp)+5];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"ies");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] != 's' 
								&&tmp[strlen(tmp)-1] != 'x'
								&&tmp[strlen(tmp)-1] != 'z'
								&&tmp[strlen(tmp)-1] != 'h'
								&&tmp[strlen(tmp)-1] != 'y'
								){
							char* s = new char[strlen(tmp)+3];
							strcpy(s,tmp);
							strcat(s,"s");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&&( tmp[strlen(tmp)-2] == 'a'
								|| tmp[strlen(tmp)-2] == 'e'
								|| tmp[strlen(tmp)-2] == 'i'
								|| tmp[strlen(tmp)-2] == 'o'
								|| tmp[strlen(tmp)-2] == 'u'
								)){
							char* s = new char[strlen(tmp)+4];
							strcpy(s,tmp);
							strcat(s,"s");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// p
					case 'P':
					{
						if(tmp[strlen(tmp)-1] == 'y' 
								&& tmp[strlen(tmp)-2] != 'a'
								&& tmp[strlen(tmp)-2] != 'e'
								&& tmp[strlen(tmp)-2] != 'i'
								&& tmp[strlen(tmp)-2] != 'o'
								&& tmp[strlen(tmp)-2] != 'u'
								){
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							s[strlen(s)-1] = '\0';
							strcat(s,"iness");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] == 'y' 
								&&( tmp[strlen(tmp)-2] == 'a'
								|| tmp[strlen(tmp)-2] == 'e'
								|| tmp[strlen(tmp)-2] == 'i'
								|| tmp[strlen(tmp)-2] == 'o'
								|| tmp[strlen(tmp)-2] == 'u'
								)){
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							strcat(s,"ness");
							if(list)
								list->AddItem(s);
						}else if(tmp[strlen(tmp)-1] != 'y'){
							char* s = new char[strlen(tmp)+6];
							strcpy(s,tmp);
							strcat(s,"ness");
							if(list)
								list->AddItem(s);
						}
						break;
					}
					// 's
					case 'M':
					{
						char* s = new char[strlen(tmp)+4];
						strcpy(s,tmp);
						strcat(s,"'s");
						if(list)
							list->AddItem(s);
						break;
					}
					}	
				}
			}
			p = strtok('\0',"\n");
		}
		delete[] all;
	}
}
#include "HPrefs.h"

#include <String.h>
#include <UTF8.h>
#include <Font.h>

HPrefs::HPrefs(const char* name, const char* dir_name)
	: HSetting(name,dir_name)
{
	MakeDefault();
}


HPrefs::~HPrefs()
{
}

void
HPrefs::MakeDefault()
{
	BRect rect;
	rect.Set(50,50,50+500,50+500);
	fDefaultMessage->AddPoint("verticalsplit",BPoint(200,0));
	fDefaultMessage->AddPoint("horizontalsplit",BPoint(0,200));
	fDefaultMessage->AddRect("window_rect",rect);
	
	const char* kLabel[] = {"Subject","From","To","When","P","A"};
	
	for(int32 i = 0;i < 6;i++)
	{
		fDefaultMessage->AddFloat(kLabel[i],100);
		BString str = kLabel[i];
		str<<"_IsShown";
		fDefaultMessage->AddBool(str.String(),true);
	}
	fDefaultMessage->AddInt32("sort_key",1);
	
	const char* kOrder= "order";
	for(int32 i = 0;i < 7;i++)
	{
		BString str = kOrder;
		str << i;
		fDefaultMessage->AddInt32(str.String(),i);
	}
	fDefaultMessage->AddRect("write_window_rect",BRect(40,40,600,540));
	fDefaultMessage->AddInt32("encoding",B_ISO1_CONVERSION);
	
	fDefaultMessage->AddInt32("smtp_account",0);
	
	BFont font(be_plain_font);
	float size = font.Size();
	font_family family;
	font_style style;
	font.GetFamilyAndStyle(&family,&style);
	fDefaultMessage->AddFloat("font_size",size);
	fDefaultMessage->AddString("font_family",family);
	fDefaultMessage->AddString("font_style",style);
	fDefaultMessage->AddInt32("check_minutes",0);
	fDefaultMessage->AddInt32("auto_check",false);
	fDefaultMessage->AddBool("expand_addr",false);
	fDefaultMessage->AddBool("expand_enclosure",false);
	fDefaultMessage->AddBool("load_list_on_start_up",false);
	fDefaultMessage->AddBool("use_folder_cache",true);
	fDefaultMessage->AddInt16("toolbar_mode",0);
	fDefaultMessage->AddBool("use_desktray",true);
	fDefaultMessage->AddRect("read_win_rect",BRect(40,40,400,400));
	
	HSetting::MakeDefault();
}

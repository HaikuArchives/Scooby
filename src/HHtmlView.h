#ifndef __HTMLVIEW_H__
#define __HTMLVIEW_H__

#include <NetPositive.h>
#include <View.h>
#include <Shelf.h>

enum EncodingMessages{
	NETPOSITIVE_ISO1 = 'ISO1',
	NETPOSITIVE_ISO2 = 'ISO2',
	NETPOSITIVE_ISO5 = 'ISO5',
	NETPOSITIVE_ISO7 = 'ISO7',
	NETPOSITIVE_SJIS = 'SJIS',
	NETPOSITIVE_JAPANESE_AUTO = 'AutJ',
	NETPOSITIVE_EUC = 'EUC!',
	NETPOSITIVE_UNICODE = 'UNIC',
	NETPOSITIVE_UTF8 = 'UTF8',
	NETPOSITIVE_MACROMAN = 'MacR',
	NETPOSITIVE_KOI8R = 'KOI8',
	NETPOSITIVE_MSDOS_886= 'D886',
	NETPOSITIVE_WINDOWS_1251 = '1251'
};

class HHtmlView :public BView {
public:
					HHtmlView(BRect rect,
							const char* name,
							bool use_toolbar = false,
							uint32 resize = B_FOLLOW_LEFT|B_FOLLOW_TOP,
							uint32	flags = B_WILL_DRAW);
	virtual 		~HHtmlView();
			
			void	ShowURL(const char* url);
			void	SetEncoding(EncodingMessages encoding);
		
			void	Back();
			void	Forward();
			void	Home();
			void	Stop();
			void	Reload();
			
			int32	GetDefaultEncoding();			
protected:
	
private:
	BView*			fNetPositiveView;
	BShelf*			fShelf;
};
#endif
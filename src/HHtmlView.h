#ifndef __HTMLVIEW_H__
#define __HTMLVIEW_H__

#include <NetPositive.h>
#include <View.h>
#include <Shelf.h>

class HHtmlView :public BView {
public:
					HHtmlView(BRect rect,
							const char* name,
							bool use_toolbar = false,
							uint32 resize = B_FOLLOW_LEFT|B_FOLLOW_TOP,
							uint32	flags = B_WILL_DRAW);
	virtual 		~HHtmlView();
			
			void	ShowURL(const char* url);
			void	SetEncoding(int32 encoding_index);
		
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
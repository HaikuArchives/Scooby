#ifndef __LOCALEAPP_H__
#define __LOCALEAPP_H__

#include <Application.h>

#define _( String ) ((LocaleApp*)be_app)->GetText( String )

class LocaleApp :public BApplication {
public:
						LocaleApp(const char *signature);
	virtual				~LocaleApp();
	const char*			GetText(const char* text);
protected:
			void		InitData(const char* lang);
	virtual void		ArgvReceived(int32 argc,char **argv);
			void		Dump();
private:
	BMessage*			fStrings;
	BMessage*			fDump;
};
#endif
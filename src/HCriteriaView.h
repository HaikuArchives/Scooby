#ifndef __HCRITERIAVIEW_H__
#define __HCRITERIAVIEW_H__

#include <View.h>
#include <MenuField.h>
#include <TextControl.h>

class HCriteriaView :public BView{
public:
						HCriteriaView(BRect rect,const char* name);
	virtual				~HCriteriaView();
			void		SetEnableControls(bool enable);
			
			void		SetValue(int32 attr,
							int32 operation,
							const char* attr_value,
							int32 operation2);
							
			int32		Attribute() const;
			int32		Operator() const;
			int32		Operator2() const;
	
	const char*			AttributeValue()const{return fValue->Text();}
protected:
			void		InitGUI();
	virtual void		Draw(BRect updateRect);
	virtual void		MouseDown(BPoint point);
	virtual void		MakeFocus(bool focus);
private:
	BMenuField			*fAttrMenu;
	BMenuField			*fOpMenu;
	BTextControl		*fValue;
	BMenuField			*fOp2Menu;
};
#endif
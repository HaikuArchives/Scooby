#ifndef __HCRITERIAVIEW_H__
#define __HCRITERIAVIEW_H__

#include <View.h>
#include <MenuField.h>
#include <TextControl.h>

//!Mail filter setting criteria view.
class HCriteriaView :public BView{
public:
		//! Constructor.
						HCriteriaView(BRect rect,const char* name);
		//! Set all controls as enabled.
			void		SetEnableControls(bool enable);
		//! Set criteria values.
			void		SetValue(int32 attr //!<Mail attribute index to be used filtering.
							,int32 operation //!<Criteria operator index such as "and, or, contains".
							,const char* attr_value //<! Criteria value.
							,int32 operation2 //<! Operator for next criteria.
							);
		//! Mail attribute index to be used filtering.
			int32		Attribute() const;
		//! Return criteria operator index such as "and, or, contains".
			int32		Operator() const;
		//! Returns operator for next criteria.
			int32		Operator2() const;
		//! Returns criteria value.
	const char*			AttributeValue()const{return fValue->Text();}
protected:
		//! Initialize all GUI.
			void		InitGUI();
		//@{
		//! Override function.
			void		Draw(BRect updateRect);
			void		MouseDown(BPoint point);
			void		MakeFocus(bool focus);
		//@}
private:
	BMenuField			*fAttrMenu;
	BMenuField			*fOpMenu;
	BTextControl		*fValue;
	BMenuField			*fOp2Menu;
};
#endif

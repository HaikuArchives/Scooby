#include "LEDAnimation.h"

#include <InterfaceDefs.h>

#define SNOOZE_TIME 150000

/***********************************************************
 * Constructor
 ***********************************************************/
LEDAnimation::LEDAnimation()
	:fThread(-1)
	,fRunning(false)
{
}

/***********************************************************
 * Destructor
 ***********************************************************/
LEDAnimation::~LEDAnimation()
{
	Stop();
}

/***********************************************************
 * Start
 ***********************************************************/
void
LEDAnimation::Start()
{
	if(fThread>=0)
		return;
	fThread = ::spawn_thread(AnimationThread,"LED thread",B_NORMAL_PRIORITY,this);
	::resume_thread(fThread);
}

/***********************************************************
 * Stop
 ***********************************************************/
void
LEDAnimation::Stop()
{
	fRunning = false;
	LED(B_NUM_LOCK,false);
	LED(B_CAPS_LOCK,false);
	LED(B_SCROLL_LOCK,false);	
}

/***********************************************************
 * AnimationThread
 ***********************************************************/
int32
LEDAnimation::AnimationThread(void* data)
{
	LEDAnimation *anim = (LEDAnimation*)data;
	anim->fRunning = true;
	
	while(anim->fRunning)
	{
		LED(B_NUM_LOCK,true);	
		LED(B_NUM_LOCK,false);
		
		LED(B_CAPS_LOCK,true);	
		LED(B_CAPS_LOCK,false);
		
		LED(B_SCROLL_LOCK,true);	
		LED(B_SCROLL_LOCK,false);				
	}
	anim->fThread = -1;
	return 0;
}

/***********************************************************
 * LED
 ***********************************************************/
void
LEDAnimation::LED(uint32 mod,bool on)
{
	uint32 current_modifiers = ::modifiers();
	if(on)
		current_modifiers |= mod;
	else
		current_modifiers &= ~mod;
	::set_keyboard_locks(current_modifiers);
	if(on)
		::snooze(SNOOZE_TIME);
}
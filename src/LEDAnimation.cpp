#include "LEDAnimation.h"

#include <InterfaceDefs.h>

#define SNOOZE_TIME 150000

/***********************************************************
 * Constructor
 ***********************************************************/
LEDAnimation::LEDAnimation()
	:fThread(-1)
	,fRunning(false)
	,fOrigModifiers(::modifiers())
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
	fOrigModifiers = ::modifiers();
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
	status_t err;
	::wait_for_thread(fThread,&err);

	::set_keyboard_locks(fOrigModifiers);
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
		LED(B_SCROLL_LOCK);	
		LED(0);	
	}

	anim->fThread = -1;
	return 0;
}

/***********************************************************
 * LED
 ***********************************************************/
void
LEDAnimation::LED(uint32 mod)
{
	::set_keyboard_locks(mod);
	::snooze(SNOOZE_TIME);
}

#ifndef __LEDANIMATION_H__
#define __LEDANIMATION_H__

#include <SupportDefs.h>
#include <OS.h>

class LEDAnimation {
public:
							LEDAnimation();
		virtual				~LEDAnimation();
		
				void		Start();
				void		Stop();
				
				bool		IsRunning() const {return fRunning;}
private:
	static	int32			AnimationThread(void *data);
	static	void			LED(uint32 mod,bool on);
		thread_id			fThread;
			bool			fRunning;
};
#endif
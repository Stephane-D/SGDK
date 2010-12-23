#ifndef _TIMER_H_
#define _TIMER_H_


#define SUBTICKPERSECOND    76800
#define TICKPERSECOND       300
#define TIMEPERSECOND       256

#define MAXTIMER            16

extern u32 vtimer;


// return elapsed subticks from console reset (1/76800 second based)
// WARNING : this function isn't accurate because of the VCounter rollback
u32  getSubTick();
// return elapsed ticks from console reset (1/300 second based)
u32  getTick();
// return elapsed time from console reset (1/256 second based)
u32  getTime(u16 fromTick);
// return elapsed time from console reset as fix32 number (in second)
fix32 getTimeAsFix32(u16 fromTick);

// start internal timer (0 <= numtimer < MAXTIMER)
void startTimer(u16 numTimer);
// get elapsed subticks from last startTimer(numTimer)
u32  getTimer(u16 numTimer, u16 restart);

// wait for a certain amount of subticks
// WARNING : this function isn't accurate because of the VCounter rollback
void waitSubTick(u32 subtick);
// wait for a certain amount of ticks
void waitTick(u32 tick);
// wait for a certain amount of millisecond
// WARNING : ~3.33 ms based timer so use 4 ms at least
void waitMs(u32 ms);


#endif // _TIMER_H_

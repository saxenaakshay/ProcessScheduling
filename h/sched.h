#ifndef _SCHED_H_
#define _SCHED_H_
#endif

#define EXPDISTSCHED 1 				/* exponenetial scheduler	*/
#define LINUXSCHED 2 				/* linux-like scheduler		*/
#define DEFAULTSCHED 3				/* xinu scheduler		*/
extern  int     schedclass;
extern	int	epochstate;			/* 1=running, 0=completed       */

extern 	void 	setschedclass (int sched_class);
extern  int 	getschedclass();
extern  void    set_epochstate(int epoch_state);
extern 	int	get_epochstate();

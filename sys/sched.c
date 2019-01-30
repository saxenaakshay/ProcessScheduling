/* sched.c  -  setschedclass, getschedclass */ 
#include <stdio.h>
#include <sched.h>

void setschedclass (int sched_class) {
	schedclass = sched_class;
}

int getschedclass() {
	return schedclass;
}

void set_epochstate(int state) {
	epochstate = state;
}

int get_epochstate() {
	return epochstate;
} 

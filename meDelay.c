#include "meDelay.h"
#include "system.h"



void meDelayInit(meDelay_t *p, uint32_t delay){
    p->timeout = meTime + delay;
}

volatile uint8_t meDelay(meDelay_t *p){
    return ( p->timeout && (meTime >= p->timeout));
}

void meDelayStop(meDelay_t *p){
    p->timeout = 0;
}

uint32_t GiveTime(){
	return meTime;
}
uint32_t GiveTimeSec(){
	return meTime / 1000;
}

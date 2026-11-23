#ifndef GMU_TIMER_H
#define GMU_TIMER_H

#include <stdint.h>

#define GMU_TIMER_HZ 100

void gmu_timer_init(void);
uint64_t gmu_timer_ticks(void);
uint64_t gmu_timer_ms(void);
uint64_t gmu_timer_seconds(void);
void gmu_sleep_ms(uint64_t milliseconds);

#endif

/*
    m5threads, a pthread library for the M5 simulator
    Copyright (C) 2009, Stanford University

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
	
	Test
*/



#ifndef __SPINLOCK_RISCV_H__
#define __SPINLOCK_RISCV_H__



#include <linux/kernel.h>
#include "compiler.h"
#include <stdint.h>
//#include "tests/mutex_print.hpp"

typedef union {
	uint32_t lock;
 struct {
	uint16_t serving_now;
	uint16_t ticket;
} h;
}arch_spinlock_t;


//#include <atomic.h>
//#include "barrier.h"
//#include "fence.h"
// routines adapted from /usr/src/linux/include/asm-alpha/spinlock.h

#define RISCV_FENCE(p, s) \
	__asm__ __volatile__ ("fence " #p "," #s : : : "memory")

/* These barriers need to enforce ordering on both devices or memory. */
#define mb()		RISCV_FENCE(iorw,iorw)
#define rmb()		RISCV_FENCE(ir,ir)
#define wmb()		RISCV_FENCE(ow,ow)

#if 1//#ifdef CONFIG_SMP
#define RISCV_ACQUIRE_BARRIER		"\tfence r , rw\n"
#define RISCV_RELEASE_BARRIER		"\tfence rw,  w\n"
#else
#define RISCV_ACQUIRE_BARRIER
#define RISCV_RELEASE_BARRIER
#endif

//#define atomic_read(ptr) (*(volatile typeof(*(ptr)) *)(ptr))
//#define spin_is_locked(x)	(atomic_read(x) != 0)
#define spin_is_locked(x)	(*(x) != 0)
//#define spin_is_locked(x)	((*x) != 0) //- READ_ONCE?



/*
static inline int spin_trylock(volatile int* lock)
{
	int tmp = 1;
	int busy;

	__asm__ __volatile__ (
		"	amoswap.w %0, %2, %1\n"
		RISCV_ACQUIRE_BARRIER
		: "=r" (busy), "+A" (*lock)
		: "r" (tmp)
		: "memory");

	return !busy;
}
*/


/*
static inline int spin_trylock(volatile int* lock)
{
	int tmp = 1, busy;

	__asm__ __volatile__ (
		"	amoswap.w %0, %2, %1\n"
		RISCV_ACQUIRE_BARRIER
		: "=r" (busy), "+A" (*lock)
		: "r" (tmp)
		: "memory");

	return !busy;
}
*/

int atomic_inc(volatile int* c)
{
	int tmp=0;
	int inc=1;
	//int res=1;
	printf("tmp: %d, inc: %d, c: %d \n", tmp,inc,*c);
	
	__asm__ __volatile__ (
	"amoadd.w %1, %2, %0 \n"
	:"+A"(*c):"r"(tmp),"r"(inc):"memory");
	
	printf("tmp: %d, inc: %d, c: %d \n", tmp,inc,*c);
}


static inline int trylock(volatile int* in)
{
	int tmp = 1, busy;
	arch_spinlock_t *lock = (arch_spinlock_t*)in;

	__asm__ __volatile__ (
		"	amoswap.w %0, %2, %1\n"
		RISCV_ACQUIRE_BARRIER
		: "=r" (busy), "+A" (*lock)
		: "r" (tmp)
		: "memory");

	return !busy;
}


static inline void spin_lock(volatile int* in)
{
	arch_spinlock_t *lock = (arch_spinlock_t*)in;
	
	
    //printf("spin_lock start\n");
	int my_ticket;
	int tmp;
	int inc = 0x10000;
	int success;
	
    
	do{
		//printf("BEFORE: serving now: %d next: %d\n", lock->h.serving_now, lock->h.ticket);
		__asm__ __volatile__ (
			"1:	lr.w %[ticket], %[ticket_ptr] \n"
			"	add %[my_ticket], %[ticket], %[inc]\n"
			"	sc.w	%[succ], %[my_ticket], %[ticket_ptr]\n"
			//"	bnez	%[succ], 1b\n"
			: [ticket_ptr] "+A" (lock->lock),
			  //[serving_now_ptr] "+m" (lock->h.serving_now),
			  [ticket] "=&r" (tmp),
			  [my_ticket] "=&r" (my_ticket),
			  [succ] "=&r" (success)
			: [inc] "r" (0x10000)
			: "memory"
		);
		//printf("Success: %d\n", success);
	}while(success != 0);
	
	mb();
    printf("my ticket:%d  serving now: %d next: %d\n", (my_ticket>>16), lock->h.serving_now, lock->h.ticket);
	//my_ticket = my_ticket-1;
	//for(volatile int i = 100000; i > 0; --i);
    //printf("my ticket:%d serving:%d \n",(my_ticket>>16), (lock->h.serving_now));
	volatile int c = 0;
	while((my_ticket>>16) != (lock->h.serving_now +1)){
		if(++c >= 10){
			printf("my ticket:%d  serving now: %d next: %d\n", (my_ticket>>16), lock->h.serving_now, lock->h.ticket);
			c = 0;
		}
		for(volatile int i = 10000; i > 0; --i);
	}
	for(volatile int i = 100000; i > 0; --i);
	printf("my ticket:%d  is being served\n", (my_ticket>>16));
	//for(volatile int i = 100000; i > 0; --i);
	
}
/*

static __inline__ void spin_lock (volatile int* lock) 
{
	int tmp = 1, busy;
	
	while(*lock != 0){
		printf("Lock taken\n");
	}
	//printf("spin lock\n");
	__asm__ __volatile__(
		"again%=:\n"
		"amoswap.w %0, %2, %1 \n" //# Attempt to acquire lock.
		RISCV_ACQUIRE_BARRIER
		"bnez %0, again%= \n"// Retry if held
		: "=r"(busy), "+A" (*lock)
		: "r" (tmp): "memory");
}

*/



static inline void spin_unlock(volatile int* in)
{
	arch_spinlock_t *lock = (arch_spinlock_t*)in;
	
	printf("unlock\n");
	//printf("serving now: %d\n", lock->h.serving_now);
	unsigned int serving_now = lock->h.serving_now + 1;
	wmb();
	printf("unlocked\n");
	
	lock->h.serving_now = (uint16_t)serving_now;
	printf("serving now: %d\n", lock->h.serving_now);
	
	/*
	//printf("try unlock TID=0x%llx\n", pthread_self());
	__asm__ __volatile__ (
		"amoswap.w x0, x0, %0 \n"
		RISCV_RELEASE_BARRIER
		: "=A" (*lock)
		:: "memory");
		*/
	//printf("Unlock done TID=0x%llx\n", pthread_self());;
		
		
		
}






#endif  // __SPINLOCK_H__

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

#define _LR_SC_
#ifdef _LR_SC_

static inline void spin_lock(volatile int* lock)
{
	int tmp = 0xFF;
	while(tmp!=0)
	{
		__asm__ __volatile__(
			"go%=:\n"
			"   lr.w	%1, %0\n"
			"	bnez	%1, go%=\n"
			"	li		%1, 1\n"
			"	sc.w	%1, %1, %0\n"
			//"end%=: "
			//"	bnez	%1, go%=\n"
			: "+A" (*lock), "=&r" (tmp)
			:: "memory");
			
	}
	//printf("lock done\n");
}


static inline void spin_unlock(volatile int* lock)
{
	*lock = 0;
}

#endif
#ifdef _AMOSWAP_
static inline void spin_lock(volatile int* lock)
{
	__asm__ __volatile__ (
		"li x0, 1 \n" //# Initialize swap value.
		"again%=:\n"
		"amoswap.w x0, x0, %[lock]\n" //# Attempt to acquire lock.
		RISCV_ACQUIRE_BARRIER
		"bnez x0, again%=\n" // # Retry if held.
		:[lock]"+A" (*lock):
		:"memory"
	);
	
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



static inline void spin_unlock(volatile int* lock)
{
	printf("unlock\n");
	__asm__ __volatile__ (
		RISCV_RELEASE_BARRIER
		"amoswap.w.rl x0, x0, %[lock]\n"// # Release lock by storing 0.
		: [lock]"=A" (*lock)
		:: "memory");
	printf("unlocked\n");
	/*
	arch_spinlock_t *lock = (arch_spinlock_t*)in;
	
	
	//printf("serving now: %d\n", lock->h.serving_now);
	unsigned int serving_now = lock->h.serving_now + 1;
	wmb();
	
	
	lock->h.serving_now = (uint16_t)serving_now;
	printf("serving now: %d\n", lock->h.serving_now);
	*/
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


#endif



#endif  // __SPINLOCK_H__

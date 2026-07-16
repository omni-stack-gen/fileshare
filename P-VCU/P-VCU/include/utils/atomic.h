#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

long atomic_inc_long(volatile long *val);
long atomic_dec_long(volatile long *val);
void atomic_store_long(volatile long *ptr, long val);
long atomic_set_long(volatile long *ptr, long val);
long atomic_exchange_long(volatile long *ptr, long val);
long atomic_load_long(const volatile long *ptr);

bool atomic_compare_swap_long(volatile long *val, long old_val,
					       long new_val);

bool atomic_compare_exchange_long(volatile long *val,
						   long *old_val, long new_val);

void atomic_store_bool(volatile bool *ptr, bool val);
bool atomic_set_bool(volatile bool *ptr, bool val);
bool atomic_exchange_bool(volatile bool *ptr, bool val);
bool atomic_load_bool(const volatile bool *ptr);

#ifdef __cplusplus
}
#endif

#endif //!__ATOMIC_H__
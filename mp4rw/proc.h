#pragma once

#include "memory.h"

#include <stdint.h>
#include <unistd.h>

constexpr uint32_t PROC_PID_OFFSET = 0xbc;

uintptr_t proc_get_lib(uintptr_t proc, int handle);

uintptr_t get_proc(int target_pid);

static inline int proc_get_pid(uintptr_t proc) {
	int pid = 0;
	kernel_copyout(proc + PROC_PID_OFFSET, &pid, sizeof(pid));
	return pid;
}

static inline uintptr_t proc_get_next(uintptr_t proc) {
	uintptr_t next = 0;
	kernel_copyout(proc, &next, sizeof(next));
	return next;
}

uintptr_t get_current_proc(void);

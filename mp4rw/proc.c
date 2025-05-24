#include "offsets.h"
#include "proc.h"

uintptr_t get_proc(int target_pid) {
	uintptr_t proc = 0;
	kernel_copyout(get_allproc_address(), &proc, sizeof(proc));
	while (proc != 0) {
		int pid = proc_get_pid(proc);
		if (pid == target_pid) {
			return proc;
		}
		proc = proc_get_next(proc);
	}
	return 0;
}

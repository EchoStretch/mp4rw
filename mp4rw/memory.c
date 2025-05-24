#include "proc.h"

uintptr_t get_current_proc(void) {
	static uintptr_t g_current_proc = 0;
	if (g_current_proc != 0) {
		return g_current_proc;
	}
	g_current_proc = get_proc(getpid());
	return g_current_proc;
}

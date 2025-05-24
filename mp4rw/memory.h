#pragma once

#include <stdint.h>
#include <stddef.h> // IWYU pragma: keep
#include <unistd.h>

constexpr uint32_t PROC_UCRED_OFFSET = 0x40;

void kernel_copyin(const void *src, uint64_t kdest, size_t length);
void kernel_copyout(uint64_t ksrc, void *dest, size_t length);

static inline uint32_t kread32(uintptr_t addr) {
	uint32_t res = 0;
	kernel_copyout(addr, &res, sizeof(res));
	return res;
}

static inline void kwrite32(uintptr_t addr, uint32_t value) {
	kernel_copyin(&value, addr, sizeof(value));
}

static inline uint64_t kread64(uintptr_t addr) {
	uint64_t res = 0;
	kernel_copyout(addr, &res, sizeof(res));
	return res;
}

static inline void kwrite64(uintptr_t addr, uint64_t value) {
	kernel_copyin(&value, addr, sizeof(value));
}

static inline uintptr_t kread_uintptr(uintptr_t addr) {
	uintptr_t res = 0;
	kernel_copyout(addr, &res, sizeof(res));
	return res;
}

static inline uintptr_t proc_get_ucred(uintptr_t proc) {
	uintptr_t ucred = 0;
	kernel_copyout(proc + PROC_UCRED_OFFSET, &ucred, sizeof(ucred));
	return ucred;
}

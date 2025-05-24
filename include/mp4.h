#pragma once

#include <stddef.h>
#include <stdint.h>


constexpr int ERROR_VALUE = -1;
constexpr int SUCCESS_VALUE = 0;

/**
 * @brief mp4 memory address types
 *
 * These values tell the sdbg handler how to access the memory
 */
typedef enum mp4_memory_type : uint32_t {
	// converts the provided physical address to an EL3 accessible address
	PA_TO_EL3_VA = 0x020000,
	// uses the provided address as is
	EL3_VA_TO_EL3_VA = 0x430000,
	// converts the provided EL0 accessible address to an to an EL3 accessible address
	EL0_VA_TO_EL3_VA = 0x400000
} mp4_memory_type_t;

// the size of the coredump buffer constrains the length to an int

/**
 * @brief reads memory from mp4 at the specified address accessed as the specified type
 *
 * @param addr the mp4 address to read from
 * @param dst the destination buffer
 * @param length the length of memory to read
 * @param type the memory type to read from the address as
 * @return the amount of memory that was actually read
 */
int mp4_read_typed(uintptr_t addr, void *dst, int length, mp4_memory_type_t type);

/**
 * @brief reads memory from mp4 at the specified EL3 accessible address
 *
 * @param addr the EL3 address to read from
 * @param dst the destination buffer
 * @param length the length of memory to read
 * @return the amount of memory that was actually read
 */
static inline int mp4_read(uintptr_t vaddr, void *dst, int length) {
	return mp4_read_typed(vaddr, dst, length, EL3_VA_TO_EL3_VA);
}

/**
 * @brief reads memory from mp4 at the specified physical address
 *
 * @param addr the pyhsical address to read from
 * @param dst the destination buffer
 * @param length the length of memory to read
 * @return the amount of memory that was actually read
 */
static inline int mp4_read_pa(uintptr_t paddr, void *dst, int length) {
	return mp4_read_typed(paddr, dst, length, PA_TO_EL3_VA);
}

/**
 * @brief reads memory from mp4 at the specified EL0 accessible address
 *
 * @param addr the EL0 address to read from
 * @param dst the destination buffer
 * @param length the length of memory to read
 * @return the amount of memory that was actually read
 */
static inline int mp4_read_el0(uintptr_t addr, void *dst, int length) {
	return mp4_read_typed(addr, dst, length, EL0_VA_TO_EL3_VA);
}

/**
 * @brief write memory to mp4 at the specified address accessed as the specified type
 *
 * @param addr the mp4 address to write to
 * @param dst the source buffer
 * @param length the length of memory to write
 * @param type the memory type to write to the address as
 * @return the amount of memory that was actually written
 */
int mp4_write_typed(uintptr_t addr, const void *dst, int length, mp4_memory_type_t type);

/**
 * @brief writes memory to mp4 at the specified EL3 accessible address
 *
 * To circumvent write protection, provide a physical address to this function.
 *
 * @param addr the EL3 address to write to
 * @param dst the destination buffer
 * @param length the length of memory to write
 * @return the amount of memory that was actually written
 */
static inline int mp4_write(uintptr_t vaddr, const void *dst, int length) {
	return mp4_write_typed(vaddr, dst, length, EL3_VA_TO_EL3_VA);
}

/**
 * @brief writes memory to mp4 at the specified physical address
 *
 * @param addr the physical address to write to
 * @param dst the destination buffer
 * @param length the length of memory to write
 * @return the amount of memory that was actually written
 */
static inline int mp4_write_pa(uintptr_t paddr, const void *dst, int length) {
	return mp4_write_typed(paddr, dst, length, PA_TO_EL3_VA);
}

/**
 * @brief writes memory to mp4 at the specified EL0 accessible address
 *
 * @param addr the EL0 address to write to
 * @param dst the destination buffer
 * @param length the length of memory to write
 * @return the amount of memory that was actually written
 */
static inline int mp4_write_el0(uintptr_t addr, const void *dst, int length) {
	return mp4_write_typed(addr, dst, length, EL0_VA_TO_EL3_VA);
}

/**
 * @brief Get the mdsr logs
 *
 * @note Static buffers are used to hold the logs, this is not thread safe.
 *
 * @param mm_mdsr pointer to store the mm_mdsr log
 * @param mm_length pointer to store the mm_mdsr log length
 * @param io_mdsr pointer to store the io_mdsr log
 * @param io_length pointer to store the io_mdsr log length
 * @return SUCCESS_VALUE on success or ERROR_VALUE on failure
 */
int get_mdsr_logs(const char **mm_mdsr, size_t *mm_length, const char **io_mdsr, size_t *io_length);

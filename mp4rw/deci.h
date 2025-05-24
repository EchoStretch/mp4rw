#pragma once

#include "mp4.h"

#include <stdint.h>

constexpr uint32_t DECI5S_MAGIC = __builtin_bswap32('PD5s');
constexpr uint32_t DECI5S_TARGET_KERNEL = 0x80FF0180;
constexpr uint32_t DECI5S_TARGET_MP4 = 0x80FF0201;
constexpr uint32_t DECI5S_PROTOCOL_ID_SDBGP = 0x20000201;
constexpr uint32_t DECI5S_DEFAULT_DCMP = 0x18;
constexpr uint32_t DECI5S_DEFAULT_CODE = 0x50;


typedef enum deci5s_command_type : uint32_t {
    GET_CONF=0x1010010,
    GET_STATUS=0x1010011,
    SET_STATUS=0x1010012,
    BREAK=0x1020001,
    CONTINUE=0x1020002,
    GET_HARDWARE_BREAKPOINT=0x1020031,
    SET_HARDWARE_BREAKPOINT=0x1020032,
    CLEAR_HARDWARE_BREAKPOINT=0x1020033,
    GET_SOFTWARE_BREAKPOINT=0x1020041,
    SET_SOFTWARE_BREAKPOINT=0x1020042,
    CLEAR_SOFTWARE_BREAKPOINT=0x1020043,
    STEP=0x1020051,
    READ_MEMORY=0x1040021,
    WRITE_MEMORY=0x1040031,
    GET_REG=0x1050020,
    SET_REG=0x1050021,
    GET_BACKTRACE=0x1050030,
    MP4_GET_MSGBUF=0x1190201,
    ADDRESS_TRANSLATION=0x1200011,
    GET_MMU=0x1200021,
    GET_PMU=0x1200031,
    GET_SYSHUB_TLB=0x1200111,
    TEST=0x1ff0001,
    UNDEF=0x1ffffff,
    READ_MEMORY_RESULT=0x2040021,
    WRITE_MEMORY_RESULT=0x2040032,
    GET_REG_REPLY=0x2050020,
    ADDRESS_TRANSLATION_REPLY=0x2200011,
    GET_MMU_REPLY=0x2200021
} deci5s_command_type_t;

typedef struct deci5s_header {
    uint32_t magic;
    uint32_t self_size;
    uint32_t packet_size;
    uint32_t src;
    uint32_t dst;
    uint32_t protocol_id;
    uint32_t attr;
    uint32_t user_data;
    uint64_t timestamp;
} deci5s_header_t;

typedef struct deci5s_command_arg {
	uint32_t self_size;
	// uint8_t data[self_size]; // command specific
} deci5s_command_arg_t;

typedef struct deci5s_command {
    uint32_t self_size;
    uint32_t total_size;
    deci5s_command_type_t type;
	uint32_t pad[4];
    uint32_t num_args;
	// deci5s_command_arg_t args[num_args];
} deci5s_command_t;

typedef struct deci5s_command_header {
    deci5s_header_t header;
    uint32_t dcmp;
    uint32_t code;
    uint64_t pad;
    uint8_t unknown0[4];
    uint8_t num_commands;
    uint8_t unknown1[3];
	// command_t commands[num_commands];
} deci5s_command_header_t;

typedef struct deci5s_address_translate_reply {
    uint32_t self_size;
    uint32_t total_size;
    deci5s_command_type_t type;
    uint32_t field3_0xc;
    uint64_t field4_0x10;
    uint64_t virtual_address;
    uint64_t error;
    uint64_t AT_S1E0R;
    uint64_t AT_S1E0W;
    uint64_t AT_S1E3R;
    uint64_t AT_S1E3W;
} deci5s_address_translate_reply_t;

typedef struct deci5s_reply_result {
	uint32_t self_size;
	// uint8_t data[self_size]; // reply specific
} deci5s_reply_result_t;

typedef struct deci5s_reply {
    uint32_t self_size;
    uint32_t total_size;
    deci5s_command_type_t type;
    uint32_t field3_0xc;
    uint32_t field4_0x10;
    uint32_t field5_0x14;
    uint32_t field6_0x18;
    uint32_t field7_0x1c;
    uint32_t nargs;
    uint32_t field9_0x24;
} deci5s_reply_t;

typedef struct deci5s_reply_header {
    uint32_t magic;
    uint32_t self_size;
    uint32_t packet_size;
    uint32_t src;
    uint32_t dst;
    uint32_t protocol_id;
    uint32_t attr;
    uint32_t user_data;
    uint32_t field8_0x20;
    uint32_t field9_0x24;
    uint32_t dcmp;
    uint32_t code;
    uint64_t field12_0x30;
    uint8_t field13_0x38;
    uint8_t field14_0x39;
    uint8_t field15_0x3a;
    uint8_t field16_0x3b;
    uint8_t num_replies;
    uint8_t field18_0x3d;
    uint8_t field19_0x3e;
    uint8_t field20_0x3f;
    deci5s_reply_t reply;
} deci5s_reply_header_t;

typedef struct deci5s_get_mmu_reply {
    uint32_t self_size;
    uint32_t total_size;
    deci5s_command_type_t type;
    uint32_t field3_0xc;
    uint32_t field4_0x10;
    uint32_t field5_0x14;
    uint32_t field6_0x18;
    uint32_t nresults;
    uint32_t field8_0x20;
    uint32_t field9_0x24;
    uint64_t mair_el3;
} deci5s_get_mmu_reply_t;

typedef enum mpt_el : uint8_t {
    MMU_EL0=0,
    MMU_EL1=1,
    MMU_EL2=2,
    MMU_EL3=3
} mpt_el_t;

typedef struct deci5s_get_mmu_result {
    uint32_t self_size;
    mpt_el_t mpt_el;
    uint64_t vbase;
    uint64_t length;
    uint64_t pbase;
    uint64_t attr;
    uint64_t page_size;
} deci5s_get_mmu_result_t;

typedef struct deci5s_read_memory_command {
    uint32_t self_size;
    uint32_t total_size;
    deci5s_command_type_t type;
	uint32_t pad[4];
    uint32_t num_args;
    // read_memory_arg_t args[command.num_args];
} deci5s_read_memory_command_t;

typedef struct deci5s_write_memory_command {
    uint32_t self_size;
    uint32_t total_size;
    deci5s_command_type_t type;
	// 4 more bytes then the read command, I don't know why
	uint32_t pad0[5];
    uint32_t num_args;
	uint32_t pad1;
	// variable length array of args
	// array entries must be aligned after their data
	// deci5s_write_memory_arg_t args[num_args];
} deci5s_write_memory_command_t;

typedef struct deci5s_memory_arg {
    uint32_t self_size;
	uint32_t access_size:8;
    mp4_memory_type_t type:24;
    uint64_t addr;
    uint64_t size;
} deci5s_memory_arg_t;

typedef struct deci5s_read_memory_result {
    uint32_t self_size;
    uint32_t type;
    uint64_t addr;
    uint64_t size;
    uint64_t nread;
    uint32_t err;
    uint32_t pad;
} deci5s_read_memory_result_t;

typedef struct deci5s_write_memory_reply {
    uint32_t self_size;
    uint32_t type;
    uint64_t addr;
    uint64_t size;
    uint64_t nread;
    uint32_t err;
    uint32_t pad;
} deci5s_write_memory_reply_t;

typedef enum mp4_dump_syscore_ioctl_cmd : uint64_t {
	MP4_DUMP_SYSCORE_START_MP4DUMP = 0x800c410b,
	MP4_DUMP_SYSCORE_FINISH = 0x8008410f,
	MP4_DUMP_SYSCORE_START_CPDUMP = 0x8014410d,

	// this is not fully understood
	// this needs to be issued during the initial coredump only
	MP4_DUMP_SYSCORE_UNKNOWN_ALTER_STATE = 0x40184115
} mp4_dump_syscore_ioctl_cmd_t;

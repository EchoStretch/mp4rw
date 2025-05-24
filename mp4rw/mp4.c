#include "auth.h"
#include "deci.h"
#include "mp4.h"
#include "memory.h"
#include "offsets.h"
#include "proc.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

constexpr uint64_t MDSR_MAGIC = 0xCBB3D18A1AA5DAEF;
constexpr size_t MDSR_LOG_BUF_SIZE = 0x10000;

constexpr uint32_t MP4_DEVICE_NAMEUNIT = __builtin_bswap32('mp40');
constexpr uint32_t ROOT_DEVICE_NAMEUNIT = __builtin_bswap32('root');

constexpr uint32_t ZCN_BAR2_OFFSET = 0x18;
constexpr uint32_t BUSHANDLE_OFFSET = 0x10;

constexpr uint32_t COREDUMP_COMMAND = 0x20303000;

constexpr uint32_t EXPECTED_COREDUMP_STATE = 0xf;
constexpr uint32_t EXPECTED_COREDUMP_FLAGS = 0x212;


constexpr uint32_t DEVICE_DEVLINK_OFFSET = 0x18;
constexpr uint32_t DEVICE_NAMEUNIT_OFFSET = 0x58;
constexpr uint32_t DEVICE_SOFTC_OFFSET = 0x88;

// for locating the coredump flags, state and for finding zcn_bar2
static uint8_t mp4_softc_data[0x1000];

static uintptr_t mp4sc = 0;
static uintptr_t zcn_bar2 = 0;

static uintptr_t coredump_flags_address = 0;
static uintptr_t coredump_state_address = 0;
static uintptr_t coredump_buffer_address = 0;
static uintptr_t coredump_buffer_iommu_address = 0;
static uintptr_t coredump_buffer_size_address = 0;

extern uint64_t sceKernelReadTsc(void);

static uintptr_t device_get_next(uintptr_t device) {
	return kread_uintptr(device + DEVICE_DEVLINK_OFFSET);
}

static uintptr_t device_get_softc(uintptr_t device) {
	return kread_uintptr(device + DEVICE_SOFTC_OFFSET);
}

static uintptr_t device_get_nameunit(uintptr_t device) {
	return kread_uintptr(device + DEVICE_NAMEUNIT_OFFSET);
}

static uintptr_t get_mp40(void) {
	static uintptr_t mp40_device = 0;
	if (mp40_device != 0) {
		return mp40_device;
	}
	uintptr_t device = kread_uintptr(get_bus_data_devices_address());
	if (device == 0) {
		puts("bus_data_devices NULL tqh_first");
		puts("bus_data_devices offset is incorrect");
		return 0;
	}
	uintptr_t nameunit = device_get_nameunit(device);
	if (nameunit == 0 || kread32(nameunit) != ROOT_DEVICE_NAMEUNIT) {
		puts("bus_data_devices offset is incorrect");
		return 0;
	}
	while (device != 0) {
		uintptr_t nameunit = device_get_nameunit(device);
		if (nameunit != 0 && kread32(nameunit) == MP4_DEVICE_NAMEUNIT) {
			mp40_device = device;
			return device;
		}
		device = device_get_next(device);
	}
	return 0;
}

static uintptr_t get_mp4sc(void) {
	uintptr_t mp40 = get_mp40();
	if (mp40 == 0) {
		return 0;
	}
	return device_get_softc(mp40);
}

static size_t get_mp4_softc_size(void) {
	uintptr_t ops = kread_uintptr(get_mp40());
	uintptr_t cls = kread_uintptr(ops+0x800);
	return kread64(cls+0x10);
}

static void fill_mp4_softc_data(void) {
	size_t size = get_mp4_softc_size();
	if (size > sizeof(mp4_softc_data)) {
		printf("unexpected mp4 softc size: 0x%zx\n", size);
		// whether or not it was filled is checked later
		return;
	}
	kernel_copyout(mp4sc, mp4_softc_data, size);
	// TODO: get zcn_bar2
}

static bool is_coredump_buffer(uintptr_t value) {
	constexpr uintptr_t UPPER_MASK = 0xffffff0000000000;
	constexpr uintptr_t LOWER_MASK = 0xfffff;
	if ((value & UPPER_MASK) != UPPER_MASK) {
		return false;
	}
	// coredump buffer is between 0x100000 and 0x200000 bytes
	// so it is aligned accordingly
	return (value & LOWER_MASK) == 0;
}

static void dump_softc_data(void) {
	if (mkdir("/mnt/usb0/mp4", 0666) == -1) {
		if (errno != EEXIST) {
			perror("mkdir /mnt/usb0/mp4 failed");
			exit(-1);
		}
	}
	int fd = open("/mnt/usb0/mp4/softc.bin", 0x301, 0777);
	if (fd == -1) {
		perror("failed to open /mnt/usb0/mp4/softc.bin");
		return;
	}
	write(fd, mp4_softc_data, sizeof(mp4_softc_data));
	close(fd);
}

static void init_mp4_values(void) {
	mp4sc = get_mp4sc();

	if (*(uintptr_t*)mp4_softc_data != get_mp40()) {
		// error was already logged
		// just avoiding exiting during authid swap
		exit(-1);
	}

	const uint32_t *restrict values = (uint32_t *)mp4_softc_data;
	constexpr size_t VALUES_LENGTH = sizeof(mp4_softc_data)/sizeof(uint32_t);
	size_t pos = 0;
	for (; pos < VALUES_LENGTH; pos++) {
		if ((values[pos] & ~0x10) == EXPECTED_COREDUMP_STATE) {
			coredump_state_address = mp4sc + (pos++ * sizeof(uint32_t));
			break;
		}
	}

	if (coredump_state_address == 0) {
		puts("failed to locate coredump state");
		dump_softc_data();
		exit(-1);
	}

	for (; pos < VALUES_LENGTH; pos++) {
		if ((values[pos] & 0xffff) == EXPECTED_COREDUMP_FLAGS) {
			coredump_flags_address = mp4sc + (pos++ * sizeof(uint32_t));
			break;
		}
	}

	if (coredump_flags_address == 0) {
		puts("failed to locate coredump flags");
		dump_softc_data();
		exit(-1);
	}

	pos /= 2;
	const uintptr_t *restrict addresses = (uintptr_t *)mp4_softc_data;
	constexpr size_t ADDRESSES_LENGTH = sizeof(mp4_softc_data)/sizeof(uintptr_t);

	for (; pos < ADDRESSES_LENGTH; pos++) {
		if (is_coredump_buffer(addresses[pos])) {
			coredump_buffer_address = mp4sc + (pos * sizeof(uintptr_t));
			coredump_buffer_iommu_address = coredump_buffer_address - sizeof(uintptr_t);
			coredump_buffer_size_address = coredump_buffer_iommu_address - sizeof(uintptr_t);
			break;
		}
	}

	if (coredump_buffer_address == 0) {
		puts("failed to locate coredump buffer");
		exit(-1);
	}
}

static int run_coredump(void) {
	int err = ERROR_VALUE;
	int fd = open("/dev/mp4/dump", 0, 0);

	if (fd == ERROR_VALUE) {
		perror("open /dev/mp4/dump failed");
		return err;
	}

	int kq = kqueue();
	if (kq == ERROR_VALUE) {
		perror("kqueue failed");
		close(fd);
		return err;
	}

	uint32_t ctx[] = {12, 0, 0, 0, 0, 0}; // 12 is ioctl arg size
	struct kevent event = {
		.ident = fd,
		.filter = EVFILT_READ,
		.flags = EV_ADD,
		.fflags = 0,
		.data = 0,
		.udata = NULL
	};

	if (kevent(kq, &event, 1, NULL, 0, NULL) == ERROR_VALUE) {
		perror("kevent add failed");
		goto done;
	}

	if (ioctl(fd, MP4_DUMP_SYSCORE_START_MP4DUMP, ctx) == ERROR_VALUE) {
		perror("ioctl start mp4dump failed");
		goto done;
	}

	if (kevent(kq, NULL, 0, &event, 1, NULL) == ERROR_VALUE) {
		perror("kevent wait read event failed");
		goto done;
	}


	// this applies a mask to the coredump state that allows it to complete when closed
	// this needs 24 bytes
	if (ioctl(fd, MP4_DUMP_SYSCORE_UNKNOWN_ALTER_STATE, ctx) == ERROR_VALUE) {
		perror("ioctl alter state failed");
		goto done;
	}

	// fill the data not that the flags and state have an observable value
	fill_mp4_softc_data();
	ctx[0] = 8; // 8 is ioctl arg size
	ctx[1] = 0;
	if (ioctl(fd, MP4_DUMP_SYSCORE_FINISH, ctx) == ERROR_VALUE) {
		perror("ioctl mp4dump finish failed");
		goto done;
	}

	err = SUCCESS_VALUE;
done:
	close(kq);
	close(fd);
	return err;
}



static void mp4sc_init(void) {
	mp4sc = get_mp4sc();
	if (mp4sc == 0) {
		puts("failed to get mp4 softc");
		exit(-1);
	}
	uintptr_t zcn_bar2_resource = kread_uintptr(mp4sc + ZCN_BAR2_OFFSET);
	zcn_bar2 = kread_uintptr(zcn_bar2_resource + BUSHANDLE_OFFSET);
}


static void __attribute__((constructor)) coredump_init(void) {
	mp4sc_init();
	const uintptr_t current_proc = get_current_proc();
	if (current_proc == 0) {
		puts("failed to get current process");
		exit(-1);
	}

	const uint64_t orig_authid = proc_swap_authid(current_proc, SYSCORE_ID);
	int err = run_coredump();
	proc_swap_authid(current_proc, orig_authid);
	if (err) {
		puts("coredump failed");
		exit(-1);
	}
	init_mp4_values();
}

static void msi_write_c2p_arg1(int arg1) {
	kwrite32(zcn_bar2 + 0xf7000, arg1);
}

static void msi_write_c2p_arg2(int arg2) {
	kwrite32(zcn_bar2 + 0xf8000, arg2);
}

static void msi_write_c2p_arg3(int arg3) {
	kwrite32(zcn_bar2 + 0xf9000, arg3);
}

static void msi_write_c2p_command(uint32_t command) {
	const uintptr_t sc = mp4sc;
	int reqnum = kread32(sc + 0x160) + 1;
	kwrite32(sc + 0x160, reqnum); // reqnum
	kwrite32(sc + 0x164, command); // current command
	kwrite32(zcn_bar2 + 0xf6000, command);
}

static void set_coredump_state(uint32_t state) {
	kwrite32(coredump_state_address, state);
}

static void set_coredump_flags(uint32_t flags) {
	kwrite32(coredump_flags_address, flags);
}

static uintptr_t get_coredump_buffer(void) {
	return kread_uintptr(coredump_buffer_address);
}

static uint32_t get_coredump_buffer_size(void) {
	// the size gets truncated when sent to mp4
	return (uint32_t) kread64(coredump_buffer_size_address);
}

static uintptr_t get_coredump_buffer_iommu(void) {
	return kread_uintptr(coredump_buffer_iommu_address);
}

static int write_to_buffer(const deci5s_header_t *restrict pkt, size_t length, const void *buf, size_t buflen) {
	long addr = get_coredump_buffer();
	if (addr == 0) {
		puts("mp4 coredump buffer not allocated");
		return -1;
	}
	// packet length includes buflength already
	// if there is no buflen, then it is 0
	length -= buflen;
	kernel_copyin(pkt, addr, length);
	if (buf != 0 && buflen > 0) {
		kernel_copyin(buf, addr + length, buflen);
	}
	return 0;
}

static int send_packet_with_event(deci5s_header_t *restrict pkt, const void *buf, size_t buflen) {
	int err = ERROR_VALUE;
	int fd = open("/dev/mp4/dump", 0, 0);
	int kq = kqueue();

	uint32_t ctx[] = {8, 0};
	struct kevent event = {
		.ident = fd,
		.filter = EVFILT_READ,
		.flags = EV_ADD,
		.fflags = 0,
		.data = 0,
		.udata = NULL
	};

	set_coredump_state(~0x10);
	set_coredump_flags(0x210);

	// setup kevent
	if (kevent(kq, &event, 1, NULL, 0, NULL) == ERROR_VALUE) {
		perror("kevent add failed");
		goto done;
	}

	pkt->timestamp = sceKernelReadTsc();

	if (write_to_buffer(pkt, pkt->packet_size, buf, buflen) == ERROR_VALUE) {
		goto done;
	}

	uint32_t buffer_size = (uint32_t) get_coredump_buffer_size();

	uint64_t buffer = get_coredump_buffer_iommu();

	uint32_t buffer_lo = (uint32_t) buffer;
	uint32_t buffer_hi = (uint32_t)(buffer >> 0x20);

	msi_write_c2p_arg1(buffer_hi);
	msi_write_c2p_arg2(buffer_lo);
	msi_write_c2p_arg3(buffer_size);

	msi_write_c2p_command(COREDUMP_COMMAND /*0x20303000*/);

	// wait for the event to fire
	kevent(kq, 0, 0, &event, 1, NULL);

	// if accessing the debug scratch registers is a problem remove this
	// we can clear the coredump state manually if necessary
	ioctl(fd, MP4_DUMP_SYSCORE_FINISH , ctx);
	err = SUCCESS_VALUE;
done:
	close(kq);
	close(fd);
	return err;
}

static int send_packet(deci5s_header_t *restrict pkt, const void *buf, size_t buflen) {
	const uintptr_t current_proc = get_current_proc();
	const uint64_t orig_authid = proc_swap_authid(current_proc, SYSCORE_ID);
	int err = send_packet_with_event(pkt, buf, buflen);
	proc_swap_authid(current_proc, orig_authid);
	return err;
}

static size_t get_access_size(uintptr_t addr, int size) {
	// both need to be aligned
	// this simplifies things
	size |= (uint32_t)addr;

	// move sizes are weird
	if (size & 1) {
		// move 1 byte at a time
		return 1;
	}

	if (size & 2) {
		// move 2 bytes at a time
		return 3;
	}

	if (size & 4) {
		// move 4 bytes at a time
		return 4;
	}

	if (size & 8) {
		// move 8 bytes at a time
		return 5;
	}

	// move 8 bytes at a time, twice (16 bytes)
	// it is not actually a 16 byte load or store
	return 6;
}

typedef struct single_read_command {
	deci5s_command_header_t command_header;
	deci5s_read_memory_command_t command;
	// variable length array of args
	deci5s_memory_arg_t arg;
} single_read_command_t;

static int send_read(uintptr_t addr, size_t length, mp4_memory_type_t type) {
	if (length == 0) {
		return 0;
	}
	if (type == 0) {
		type = EL3_VA_TO_EL3_VA;
	}
	single_read_command_t pkt = {
		.command_header = {
			.header = {
				.magic = DECI5S_MAGIC,
				.self_size = sizeof(deci5s_header_t),
				.packet_size = sizeof(single_read_command_t),
				.src = DECI5S_TARGET_KERNEL,
				.dst = DECI5S_TARGET_MP4,
				.protocol_id = DECI5S_PROTOCOL_ID_SDBGP,
				.attr = 0,
				.user_data = 0,
				// timestamp set before sending
			},
			.dcmp = DECI5S_DEFAULT_DCMP,
			.code = DECI5S_DEFAULT_CODE,
			.pad = 0,
			.unknown0 = {0, 0, 0, 0},
			.num_commands = 1, // you can send more than one at a time
			.unknown1 = {0, 0, 0},
		},
		.command  = {
			.self_size = sizeof(deci5s_read_memory_command_t),
			.total_size = sizeof(deci5s_read_memory_command_t) + sizeof(deci5s_memory_arg_t[1]),
			.type = READ_MEMORY,
			.pad = {0, 0, 0, 0},
			.num_args = 1
		},
		.arg = {
			.self_size = sizeof(deci5s_memory_arg_t),
			.access_size = get_access_size(addr, length),
			.type = type,
			.addr = addr,
			.size = length
		},
	};

	return send_packet(&pkt.command_header.header, NULL, 0);
}

int mp4_read_typed(uintptr_t addr, void *dst, int length, mp4_memory_type_t type) {
	uintptr_t va = get_coredump_buffer();
	int err = send_read(addr, length, type);
	if (err != 0) {
		return err;
	}
	int nread = (int) kread64(va + 0xf8);
	if (nread > 0) {
		kernel_copyout(va + 0x108, dst, length);
	}
	return nread;
}

static uintptr_t mp4_get_oberon(void) {
	// appears to be constant throughout firmware versions
	return kread_uintptr(mp4sc + 0x180);
}

static uintptr_t mdsr_get_log_start(uintptr_t mdsr) {
	return mdsr + kread_uintptr(mdsr+0x208);
}

static uintptr_t mdsr_get_log_length(uintptr_t mdsr) {
	return kread_uintptr(mdsr+0x218);
}

static const char *get_mm_mdsr_log(uintptr_t mdsr, size_t *restrict length) {
	static char buf[MDSR_LOG_BUF_SIZE];
	const uintptr_t start = mdsr_get_log_start(mdsr);
	*length = mdsr_get_log_length(mdsr);
	if (*length > MDSR_LOG_BUF_SIZE) {
		return NULL;
	}
	kernel_copyout(start, buf, *length);
	return buf;
}

static const char *get_io_mdsr_log(uintptr_t mdsr, size_t *restrict length) {
	static char buf[MDSR_LOG_BUF_SIZE];
	const uintptr_t start = mdsr_get_log_start(mdsr);
	*length = mdsr_get_log_length(mdsr);
	if (*length > MDSR_LOG_BUF_SIZE) {
		return NULL;
	}
	kernel_copyout(start, buf, *length);
	return buf;
}

int get_mdsr_logs(const char **mm_mdsr, size_t *mm_length, const char **io_mdsr, size_t *io_length) {
	const uintptr_t oberon = mp4_get_oberon();

	// check io_mdsr first because it's in a lower area on low firmwares
	uintptr_t mdsr = oberon + 0x780000;
	if (kread64(mdsr) != MDSR_MAGIC) {
		mdsr = oberon + 0x700000;
		if (kread64(mdsr) != MDSR_MAGIC) {
			puts("bad io mdsr");
			return ERROR_VALUE;
		}

		*io_mdsr = get_io_mdsr_log(mdsr, io_length);
		mdsr = oberon + 0x600000;
		if (kread64(mdsr) != MDSR_MAGIC) {
			puts("bad mm mdsr");
			return ERROR_VALUE;
		}

		*mm_mdsr = get_mm_mdsr_log(mdsr, mm_length);
		return SUCCESS_VALUE;
	}

	*io_mdsr = get_io_mdsr_log(mdsr, io_length);
	mdsr = oberon + 0x700000;
	if (kread64(mdsr) != MDSR_MAGIC) {
		puts("bad mm mdsr");
		return SUCCESS_VALUE;
	}

	*mm_mdsr = get_mm_mdsr_log(mdsr, mm_length);
	return SUCCESS_VALUE;
}

typedef struct single_write_command {
	deci5s_command_header_t command_header;
	deci5s_write_memory_command_t command;
	deci5s_memory_arg_t arg;
} single_write_command_t;

static int send_write(uintptr_t addr, const void *src, size_t length, mp4_memory_type_t type) {
	if (length == 0) {
		return 0;
	}
	if (type == 0) {
		type = EL3_VA_TO_EL3_VA;
	}
	single_write_command_t pkt = {
		.command_header = {
			.header = {
				.magic = DECI5S_MAGIC,
				.self_size = sizeof(deci5s_header_t),
				.packet_size = sizeof(single_write_command_t) + length,
				.src = DECI5S_TARGET_KERNEL,
				.dst = DECI5S_TARGET_MP4,
				.protocol_id = DECI5S_PROTOCOL_ID_SDBGP,
				.attr = 0,
				.user_data = 0,
				// timestamp set before sending
			},
			.dcmp = DECI5S_DEFAULT_DCMP,
			.code = DECI5S_DEFAULT_CODE,
			.pad = 0,
			.unknown0 = {0, 0, 0, 0},
			.num_commands = 1, // you can send more than one at a time
			.unknown1 = {0, 0, 0},
		},
		.command  = {
			.self_size = sizeof(deci5s_write_memory_command_t),
			.total_size = sizeof(deci5s_write_memory_command_t) + sizeof(deci5s_memory_arg_t[1]),
			.type = WRITE_MEMORY,
			.pad0 = {0, 0, 0, 0, 0},
			.num_args = 1,
			.pad1 = 0
		},
		.arg = {
			.self_size = sizeof(deci5s_memory_arg_t),
			.access_size = get_access_size(addr, length),
			.type = type,
			.addr = addr,
			.size = length
		},
	};

	return send_packet(&pkt.command_header.header, src, length);
}

int mp4_write_typed(uintptr_t addr, const void *src, int length, mp4_memory_type_t type) {
	int err = send_write(addr, src, length, type);
	if (err != 0) {
		return err;
	}
	// FIXME: this was sheer lazyness, I'm not computing packet layouts for this example
	return length;
}

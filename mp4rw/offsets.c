#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysctl.h>


static const uint32_t VERSION_MASK = 0xffff0000;

typedef struct payload_args {
  int (*sys_dynlib_dlsym)(int, const char*, void*);
  int  *rwpipe;
  int  *rwpair;
  long  kpipe_addr;
  long  kdata_base_addr;
  int  *payloadout;
} payload_args_t;

payload_args_t* payload_get_args(void);

typedef enum firmware_version : uint32_t {
	V100 = 0x1000000,
	V101 = 0x1010000,
	V102 = 0x1020000,
	V105 = 0x1050000,
	V110 = 0x1100000,
	V111 = 0x1110000,
	V112 = 0x1120000,
	V113 = 0x1130000,
	V114 = 0x1140000,
	V200 = 0x2000000,
	V220 = 0x2200000,
	V225 = 0x2250000,
	V226 = 0x2260000,
	V230 = 0x2300000,
	V250 = 0x2500000,
	V300 = 0x3000000,
	V310 = 0x3100000,
	V320 = 0x3200000,
	V321 = 0x3210000,
	V400 = 0x4000000,
	V402 = 0x4020000,
	V403 = 0x4030000,
	V450 = 0x4500000,
	V451 = 0x4510000,
	V500 = 0x5000000,
	V502 = 0x5020000,
	V510 = 0x5100000,
	V550 = 0x5500000,
	V600 = 0x6000000,
	V602 = 0x6020000,
	V650 = 0x6500000,
	V700 = 0x7000000,
	V701 = 0x7010000,
	V720 = 0x7200000,
	V740 = 0x7400000,
	V760 = 0x7600000,
	V761 = 0x7610000,
	V800 = 0x8000000,
	V820 = 0x8200000,
	V840 = 0x8400000,
	V860 = 0x8600000,
	V900 = 0x9000000,
	V920 = 0x9200000,
	V940 = 0x9400000,
	V960 = 0x9600000,
	V1000 = 0x10000000,
	V1001 = 0x10010000,
	V1020 = 0x10200000,
	V1040 = 0x10400000,
	V1060 = 0x10600000,
} firmware_version_t;

static uintptr_t get_kernel_base(void) {
	static uintptr_t kernel_base = 0;
	if (kernel_base != 0) {
		return kernel_base;
	}
	kernel_base = payload_get_args()->kdata_base_addr;
	return kernel_base;
}

static firmware_version_t get_system_software_version(void) {
	static firmware_version_t version;
	if (version != 0) {
		return version;
	}
	size_t size = 4;
	sysctlbyname("kern.sdk_version", &version, &size, NULL, 0);
	version &= VERSION_MASK;
	return version;
}

// NOLINTBEGIN(readability-magic-numbers)

uintptr_t get_allproc_address(void) {
	static uintptr_t allproc_address;
	if (allproc_address != 0) {
		return allproc_address;
	}
	switch(get_system_software_version()) {
		case V100:
		case V101:
		case V102:
		case V105:
		case V110:
		case V111:
		case V112:
		case V113:
		case V114:
			allproc_address = 0x26D1C18;
			break;
		case V200:
		case V220:
		case V225:
		case V226:
		case V230:
		case V250:
			allproc_address = 0x2701C28;
			break;
		case V300:
		case V310:
		case V320:
		case V321:
			allproc_address = 0x276DC58;
			break;
		case V400:
		case V402:
		case V403:
		case V450:
		case V451:
			allproc_address = 0x27EDCB8;
			break;
		case V500:
		case V502:
		case V510:
		case V550:
			allproc_address = 0x291DD00;
			break;
		case V600:
		case V602:
		case V650:
			allproc_address = 0x2869D20;
			break;
		case V700:
		case V701:
		case V720:
		case V740:
		case V760:
		case V761:
			allproc_address = 0x2859D50;
			break;
		case V800:
		case V820:
		case V840:
		case V860:
			allproc_address = 0x2875D50;
			break;
		case V900:
		case V920:
		case V940:
		case V960:
			allproc_address = 0x2755D50;
			break;
		case V1000:
		case V1001:
		case V1020:
		case V1040:
		case V1060:
			allproc_address = 0x2765D70;
			break;
		default:
			puts("firmware version not supported");
			exit(-1);
	}
	allproc_address += get_kernel_base();
	return allproc_address;
}

uintptr_t get_bus_data_devices_address(void) {
	static uintptr_t bus_data_devices_address;
	if (bus_data_devices_address != 0) {
		return bus_data_devices_address;
	}
	switch(get_system_software_version()) {
		case V100:
		case V101:
		case V102:
		case V105:
		case V110:
		case V111:
		case V112:
		case V113:
		case V114:
			bus_data_devices_address = 0x1D6D478;
			break;
		case V200:
		case V220:
		case V225:
		case V226:
		case V230:
		case V250:
			bus_data_devices_address = 0x1D91478;
			break;
		case V300:
		case V310:
		case V320:
		case V321:
			bus_data_devices_address = 0x1DF1678;
			break;
		case V400:
		case V402:
		case V403:
		case V450:
		case V451:
			bus_data_devices_address = 0x1E69678;
			break;
		case V500:
		case V502:
		case V510:
		case V550:
			bus_data_devices_address = 0x1F996C8;
			break;
		case V600:
		case V602:
		case V650:
			bus_data_devices_address = 0x1FB96C8;
			break;
		case V700:
		case V701:
		case V720:
		case V740:
		case V760:
		case V761:
			bus_data_devices_address = 0x1FA5718;
			break;
		case V800:
		case V820:
		case V840:
		case V860:
			bus_data_devices_address = 0x1FA5718;
			break;
		case V900:
		case V920:
		case V940:
		case V960:
			bus_data_devices_address = 0x1F65718;
			break;
		case V1000:
		case V1001:
		case V1020:
		case V1040:
		case V1060:
			bus_data_devices_address = 0x1F65718;
			break;
		default:
			puts("firmware version not supported");
			exit(-1);
	}
	bus_data_devices_address += get_kernel_base();
	return bus_data_devices_address;
}

// NOLINTEND(readability-magic-numbers)

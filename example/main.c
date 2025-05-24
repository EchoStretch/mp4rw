#include "mp4.h"
#include "notification.h"

#include <errno.h>
#include <fcntl.h>
#include <libelf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern void kernel_copyout(uint64_t ksrc, void *dest, size_t length);

// coredump buffer is 0x200000
// don't try to read more then this
constexpr size_t A53_BUF_LENGTH = 0x1FC000;
static uint8_t a53_buf[A53_BUF_LENGTH];

static void dump_mm_mdsr_log(const char *log, size_t length) {
	if (log == NULL) {
		puts("failed to get mm_mdsr log");
		return;
	}
	if (mkdir("/mnt/usb0/mp4", 0666) == -1) {
		if (errno != EEXIST) {
			perror("mkdir /mnt/usb0/mp4 failed");
			exit(-1);
		}
	}
	int fd = open("/mnt/usb0/mp4/mm_mdsr.txt", 0x301, 0777);
	if (fd == -1) {
		perror("failed to open /mnt/usb0/mp4/mm_mdsr.txt");
		return;
	}

	write(STDOUT_FILENO, log, length);
	write(fd, log, length);
	close(fd);
}

static void dump_io_mdsr_log(const char *log, size_t length) {
	if (log == NULL) {
		puts("failed to get io_mdsr log");
		return;
	}
	if (mkdir("/mnt/usb0/mp4", 0666) == -1) {
		if (errno != EEXIST) {
			perror("mkdir /mnt/usb0/mp4 failed");
			exit(-1);
		}
	}
	int fd = open("/mnt/usb0/mp4/io_mdsr.txt", 0x301, 0777);
	if (fd == -1) {
		perror("failed to open /mnt/usb0/mp4/io_mdsr.txt");
		return;
	}
	write(STDOUT_FILENO, log, length);
	write(fd, log, length);
	close(fd);
}
static void write_a53_elf(size_t length) {
	int fd = open("/mnt/usb0/mp4/a53.elf", 0x301, 0777);
	if (fd == -1) {
		perror("failed to open /mnt/usb0/mp4/a53.elf");
		return;
	}
	write(fd, a53_buf, length);
	close(fd);
}

static bool try_read_elf(uintptr_t addr) {
	int n = mp4_read(addr, a53_buf, A53_BUF_LENGTH);
	if (n <= 0) {
		return false;
	}
	printf("read %d bytes\n", n);
	return a53_buf[0] == 0x7F && a53_buf[1] == 'E' && a53_buf[2] == 'L' && a53_buf[3] == 'F';
}

static bool read_elf(void) {
	if (try_read_elf(0x88100000)) {
		return true;
	}
	if (try_read_elf(0x88440000)) {
		return true;
	}
	if (try_read_elf(0x8a000000)) {
		return true;
	}
	return false;
}

int main(void) {
	const char *mm_mdsr = NULL;
	size_t mm_length = 0;
	const char *io_mdsr = NULL;
	size_t io_length = 0;


	printf_notification("dumping...");

	if (get_mdsr_logs(&mm_mdsr, &mm_length, &io_mdsr, &io_length) == ERROR_VALUE) {
		/// only non 0 in special case for zeco
		puts("failed to dump mdsr logs");
		printf_notification("mdsr logs skipped.");
	} else {
		dump_mm_mdsr_log(mm_mdsr, mm_length);
		dump_io_mdsr_log(io_mdsr, io_length);
		puts("mdsr logs dumped");
		printf_notification("mdsr logs dumped.");
	}

	if (!read_elf()) {
		puts("failed to find elf");
		printf_notification("Dump finished. No ELF.");
		return -1;
	}

	const Elf64_Ehdr *restrict elf = (Elf64_Ehdr *)a53_buf;

	size_t length = 0;
	if (elf->e_shnum > 0) {
		length = elf->e_shoff + (elf->e_shnum * elf->e_shentsize);
	}

	write_a53_elf(length);
	printf_notification("Dump finished. ELF dumped successfully.");
	return 0;
}

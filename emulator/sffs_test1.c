#include <stdio.h>
#include <stdint.h>

#include "flash_emulator.h"
#include "sffs.h"

int main(int argc, char *argv[]) {
	
	struct flash_dev fl;
	struct sffs fs;
	
	flash_init(&fl, 32768);
	flash_chip_erase(&fl);

	sffs_format(&fl);
	
	sffs_init(&fs);
	sffs_mount(&fs, &fl);
	sffs_debug_print(&fs);
	sffs_free(&fs);
	
	flash_free(&fl);
}


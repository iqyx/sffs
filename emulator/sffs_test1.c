#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#include "flash_emulator.h"
#include "sffs.h"


struct sffs_test_file {
	uint32_t file_id;
	uint8_t *data;
	uint32_t len;
};

#define NUM_FILES 10

struct sffs_test_file files[NUM_FILES];


static void generate_test_file(struct sffs *fs, struct sffs_test_file *tf, uint32_t file_id, uint32_t len) {
	tf->len = len;
	tf->file_id = file_id;

	tf->data = malloc(len);
	if (tf->data == NULL) {
		return;
	}

	for (int i = 0; i < len; i++) {
		tf->data[i] = rand() % 256;
	}
}

static void release_test_file(struct sffs *fs, struct sffs_test_file *tf) {
	(void)fs;

	tf->len = 0;
	if (tf->data == NULL) {
		return;
	}
	free(tf->data);
	tf->data = NULL;
}

static void write_test_file(struct sffs *fs, struct sffs_test_file *tf) {
	struct sffs_file f;

	sffs_open_id(fs, &f, tf->file_id);

	uint32_t len = tf->len;
	uint8_t *data = tf->data;
	while (len > 0) {
		uint32_t block_len = rand() % 100 + 10;
		if (block_len > len) {
			block_len = len;
		}
		sffs_write(&f, data, block_len);

		len -= block_len;
		data += block_len;
	}
	sffs_close(&f);
}

static void verify_test_file(struct sffs *fs, struct sffs_test_file *tf) {
	struct sffs_file f;

	printf("#### verifying test file id=%d\n", tf->file_id);

	sffs_open_id(fs, &f, tf->file_id);

	uint32_t block_len;
	uint32_t read_len = 0;
	uint32_t same = 1;
	do {
		uint8_t data[200];
		block_len = sffs_read(&f, data, rand() % 100 + 10);

		if (memcmp(&(tf->data[read_len]), data, block_len)) {
			same = 0;
		}

		read_len += block_len;
	} while (block_len > 0);
	sffs_close(&f);

	if (tf->len != read_len) {
		same = 0;
	}

	if (same) {
		printf("file %d verified, uncorrupted, len = %d\n", tf->file_id, read_len);
	} else {
		printf("file %d corrupted\n", tf->file_id);
		exit(1);
	}
}

static void delete_test_file(struct sffs *fs, struct sffs_test_file *tf) {
	sffs_file_remove(fs, tf->file_id);
}



int main(int argc, char *argv[]) {

	srand(time(NULL));
	
	struct flash_dev fl;
	struct sffs fs;
	
	flash_init(&fl, 1024 * 1024);
	flash_chip_erase(&fl);
	sffs_format(&fl);
	sffs_init(&fs);
	sffs_mount(&fs, &fl);
	sffs_debug_print(&fs);
	


	for (int i = 0; i < 80; i++) {

		/* select one file randomly */
		int f = rand() % NUM_FILES;

		if (files[f].data != NULL) {
			int action = rand() % 10;

			switch (action) {
				case 0:
					/* write file, possibly overwriting previous one,
					 * which is still acceptable behaviour */
					printf("#### writing test file\n");
					write_test_file(&fs, &(files[f]));
					break;

				case 1:
					printf("#### deleting test file\n");
					delete_test_file(&fs, &(files[f]));
					release_test_file(&fs, &(files[f]));
					break;

				default:
					verify_test_file(&fs, &(files[f]));
			}
		} else {
			printf("#### generating test file\n");
			generate_test_file(&fs, &(files[f]), rand() % 65535 + 1, rand() % 5000 + 2000);
			write_test_file(&fs, &(files[f]));
		}
		//~ sffs_debug_print(&fs);

	}

	sffs_debug_print(&fs);
	sffs_free(&fs);
	flash_free(&fl);
}


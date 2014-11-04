/**
 * Copyright (c) 2014, Marek Koza (qyx@krtko.org)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "flash_emulator.h"

#ifndef _SFFS_H_
#define _SFFS_H_



#define SFFS_MASTER_MAGIC 0x93827485
#define SFFS_METADATA_MAGIC 0x87985214
#define SFFS_LABEL_SIZE 8

struct sffs {
	uint32_t page_size;
	uint32_t sector_size;
	uint32_t sector_count;
	uint32_t data_pages_per_sector;
	uint32_t first_data_page;
	
	struct flash_dev *flash;
	
	char label[SFFS_LABEL_SIZE];
};

struct sffs_file {
	uint32_t pos;
	
};


struct __attribute__((__packed__)) sffs_master_page {
	uint32_t magic;
	
	uint8_t page_size;
	uint8_t sector_size;
	uint16_t sector_count;
	
	uint8_t label[8];
};

/* Erase state is set right after sector has been erased. Note that 0xFF is not
 * a valid state (value after erase), it must be set to SFFS_SECTOR_STATE_ERASED.
 * It indicates that no pages are used by data except metadata page, which is
 * initialized to default. */
#define SFFS_SECTOR_STATE_ERASED 0xDE

/* Used state means that at least one data page in this sector is marked as used
 * and at least one data page is marked as erased. Sectors with used state are
 * searched for file data pages or erased data pages. */
#define SFFS_SECTOR_STATE_USED 0xD6

/* All data pages in the sector are marked as used. There are no free erased
 * pages nor pages marked as old. This sector is not being touched except when
 * searching for file data pages */
#define SFFS_SECTOR_STATE_FULL 0x56

/* Sector contains at least one old data page and the rest of data pages is
 * marked as used. No free data pages are available. This sector is prepared to
 * be erased after all used pages are moved to another places. */
#define SFFS_SECTOR_STATE_DIRTY 0x46


struct __attribute__((__packed__)) sffs_metadata_header {
	uint32_t magic;
	
	uint8_t state;
	uint8_t metadata_page_count;
	uint8_t metadata_item_cound;
	uint8_t reserved;
};


/* Page is erased (full of 0xff) */
#define SFFS_PAGE_STATE_ERASED 0xB7

/* Page is marked as used (ie. it is part of a file). Pages marked as used are
 * searched when file contents is requested */
#define SFFS_PAGE_STATE_USED 0xB5

/* Moving state means that this page was previously marked as used but now it is
 * being overwritten/moved to another location. New page (destination) must be
 * marked as reserved during this time. */
#define SFFS_PAGE_STATE_MOVING 0x35

/* Reserved state means that this page is no longer available for new files and
 * write operation is in progress. */
#define SFFS_PAGE_STATE_RESERVED 0x34

/* All expired used pages are marked as old. They are prepared to be erased. */
#define SFFS_PAGE_STATE_OLD 0x24

/**
 * Metadata items are placed in metadata pages just after page header. Number of
 * metadata items is written in the header (it is determined during formatiing).
 * Metadata items can be spread along multiple metadata pages.
 */
struct __attribute__((__packed__)) sffs_metadata_item {
	/* Unique file identifier. It can be assigned using
	 * some sort of directory structure. This also means
	 * that maximum number of files on single filesystem
	 * can be 65536 */
	uint16_t file_id;
	
	/* Block index within one file. */
	uint16_t block;
	
	/* State of block. Can be one of the following:
	 * - erased - block is erased and ready to be used
	 * - used - block is used as part of a file
	 * - moving - block is prepared to be expired,
	 *                 new block allocation is in progress
	 * - expired - block is no longer used */
	uint8_t state;

	/* How much block space is used */
	uint16_t size;
	
	uint8_t reserved;
};


int32_t sffs_init(struct sffs *fs);
#define SFFS_INIT_OK 0
#define SFFS_INIT_FAILED -1

int32_t sffs_mount(struct sffs *fs, struct flash_dev *flash);
#define SFFS_MOUNT_OK 0
#define SFFS_MOUNT_FAILED -1

int32_t sffs_free(struct sffs *fs);
#define SFFS_FREE_OK 0
#define SFFS_FREE_FAILED -1

int32_t sffs_cache_clear(struct sffs *fs);
#define SFFS_CACHE_CLEAR_OK 0
#define SFFS_CACHE_CLEAR_FAILED -1

int32_t sffs_format(struct flash_dev *flash);
#define SFFS_FORMAT_OK 0
#define SFFS_FORMAT_FAILED -1

int32_t sffs_debug_print(struct sffs *fs);
#define SFFS_DEBUG_PRINT_OK 0

int32_t sffs_metadata_header_check(struct sffs *fs, struct sffs_metadata_header *header);
#define SFFS_METADATA_HEADER_CHECK_OK 0
#define SFFS_METADATA_HEADER_CHECK_FAILED -1

int32_t sffs_cached_read(struct sffs *fs, uint32_t addr, uint8_t *data, uint32_t len);
#define SFFS_CACHED_READ_OK 0
#define SFFS_CACHED_READ_FAILED -1

int32_t sffs_find_page(struct sffs *fs, uint32_t file_id, uint32_t block, uint32_t *addr, uint8_t page_state);
#define SFFS_FIND_PAGE_OK 0
#define SFFS_FIND_PAGE_NOT_FOUND -1
#define SFFS_FIND_PAGE_FAILED -2



int32_t sffs_open_id(struct sffs_file *f, uint32_t file_id);
int32_t sffs_close(struct sffs_file *f);
int32_t sffs_write(struct sffs_file *f, unsigned char *buf, uint32_t len);
int32_t sffs_write_pos(struct sffs_file *f, unsigned char *buf, uint32_t pos, uint32_t len);
int32_t sffs_read(struct sffs_file *f, unsigned char *buf, uint32_t len);
int32_t sffs_read_pos(struct sffs_file *f, unsigned char *buf, uint32_t pos, uint32_t len);
int32_t sffs_seek(struct sffs_file *f);


#endif

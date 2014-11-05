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

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "sffs.h"
#include "flash_emulator.h"


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/**
 * Initialize SFFS filesystem structure and allocate all required resources.
 * This functions must be called prior to any other operation on the filesystem.
 * 
 * @param fs A filesystem structure to initialize.
 * 
 * @return SFFS_INIT_OK on success or
 *         SFFS_INIT_FAILED otherwise.
 */
int32_t sffs_init(struct sffs *fs) {
	assert(fs != NULL);
	
	/* nothing to initialize yet */
	return SFFS_INIT_OK;
}


/**
 * Mounts SFFS filesystem from a flash device. Mount operation fetches required
 * information from the flash, initializes page cache (if enabled), checks master
 * block if it is valid and marks the filesystem as mounted.
 * 
 * @param fs A SFFS filesystem structure where the flash will be mounted to.
 * @param flash A flash device to be mounted.
 * 
 * @return SFFS_MOUNT_OK on success or
 *         SFFS_MOUNT_FAILED otherwise.
 */
int32_t sffs_mount(struct sffs *fs, struct flash_dev *flash) {
	assert(fs != NULL);
	assert(flash != NULL);

	struct flash_info info;
	if (flash_get_info(flash, &info) != FLASH_GET_INFO_OK) {
		return SFFS_MOUNT_FAILED;
	}

	fs->flash = flash;
	fs->page_size = info.page_size;
	fs->sector_size = info.sector_size;
	fs->sector_count = info.capacity / info.sector_size;
	fs->data_pages_per_sector =
		(info.sector_size - sizeof(struct sffs_metadata_header)) /
		(sizeof(struct sffs_metadata_item) + info.page_size);
	fs->first_data_page = info.sector_size / info.page_size - fs->data_pages_per_sector;

	/* TODO: check flash geometry and computed values */

	if (sffs_cache_clear(fs) != SFFS_CACHE_CLEAR_OK) {
		return SFFS_MOUNT_FAILED;
	}

	/* Find first page of file "0", it should contain filesystem metadata */
	struct sffs_master_page master;
	struct sffs_file f;
	if (sffs_open_id(fs, &f, 0) != SFFS_OPEN_ID_OK) {
		return SFFS_MOUNT_FAILED;
	}
	sffs_read(&f, (unsigned char *)&master, sizeof(master));
	sffs_close(&f);

	/* TODO: check SFFS master page for validity */
	/* TODO: fetch filesystem label */

	return SFFS_MOUNT_OK;
}


/**
 * Free SFFS filesystem and all allocated resources.
 * 
 * @param fs A filesystem to free.
 * 
 * @return SFFS_FREE_OK on success or
 *         SFFS_FREE_FAILED otherwise.
 */
int32_t sffs_free(struct sffs *fs) {
	assert(fs != NULL);
	
	/* nothing to do yet */
	return SFFS_FREE_OK;
}


/**
 * Clears all pages from filesystem cache.
 * 
 * @param fs A filesystem with cache to clear.
 * 
 * @return SFFS_CACHE_CLEAR_OK on success or
 *         SFFS_CACHE_CLEAR_FAILED odtherwise.
 */
int32_t sffs_cache_clear(struct sffs *fs) {
	assert(fs != NULL);

	return SFFS_CACHE_CLEAR_OK;
}


/**
 * Create new SFFS filesystem on flash memory. Flash memory cannot be mounted
 * during this operation. Information about memory geometry is fetched directly
 * from the flash.
 * 
 * @param flash A flash device to create SFFS filesystem on.
 * 
 * @return SFFS_FORMAT_OK on success or
 *         SFFS_FORMAT_FAILED otherwise.
 */
int32_t sffs_format(struct flash_dev *flash) {
	assert(flash != NULL);
	
	struct flash_info info;
	flash_get_info(flash, &info);

	/* we need to simulate filesystem structure somehow (sector format functions
	 * operate on mounted filesystem). For this purpose we mount the filesystem
	 * using local sffs structure. Mount fails as no master page can be read,
	 * we ignore this silently, return code is intentionally unchecked. */
	struct sffs fs;
	sffs_mount(&fs, flash);

	/* now iterate over all sectors and format them */
	for (uint32_t sector = 0; sector < fs.sector_count; sector++) {
		sffs_sector_format(&fs, sector);
	}
	/* TODO: write master page */
	
	return SFFS_FORMAT_OK;
}


/**
 * Print filesystem structure to stdout. It can help to visualize  how pages and
 * sectors are managed.
 * 
 * @param fs A filesstem to print.
 * 
 * @return SFFS_DEBUG_PRINT_OK.
 */
int32_t sffs_debug_print(struct sffs *fs) {
	assert(fs != NULL);

	for (uint32_t sector = 0; sector < fs->sector_count; sector++) {

		struct sffs_metadata_header header;
		/* TODO: check return value */
		sffs_cached_read(fs, sector * fs->sector_size, (uint8_t *)&header, sizeof(header));

		char sector_state = '?';
		if (header.state == SFFS_SECTOR_STATE_ERASED) sector_state = ' ';
		if (header.state == SFFS_SECTOR_STATE_USED) sector_state = 'U';
		if (header.state == SFFS_SECTOR_STATE_FULL) sector_state = 'F';
		if (header.state == SFFS_SECTOR_STATE_DIRTY) sector_state = 'D';
		if (header.state == SFFS_SECTOR_STATE_OLD) sector_state = 'O';
		printf("%04d [%c]: ", sector, sector_state);

		for (uint32_t i = 0; i < fs->data_pages_per_sector; i++) {
			uint32_t item_pos = sector * fs->sector_size + sizeof(struct sffs_metadata_header) + i * sizeof(struct sffs_metadata_item);
			struct sffs_metadata_item item;
			/* TODO: check return value */
			sffs_cached_read(fs, item_pos, (uint8_t *)&item, sizeof(struct sffs_metadata_item));

			char page_state = '?';
			if (item.state == SFFS_PAGE_STATE_ERASED) page_state = ' ';
			if (item.state == SFFS_PAGE_STATE_USED) page_state = 'U';
			if (item.state == SFFS_PAGE_STATE_MOVING) page_state = 'M';
			if (item.state == SFFS_PAGE_STATE_RESERVED) page_state = 'R';
			if (item.state == SFFS_PAGE_STATE_OLD) page_state = 'O';
			printf("[%c] ", page_state);
		}

		printf("\n");
	}
	printf("\n");


	return SFFS_DEBUG_PRINT_OK;
}

/**
 * Perform various checks on metadata page header to find any inconsistencies.
 * 
 * @param fs A SFFS filesystem to check.
 * @param header Metadata header structure to check (extracted from metadata page)
 * 
 * @return SFFS_METADATA_HEADER_CHECK_OK if the header is valid or
 *         SFFS_METADATA_HEADER_CHECK_FAILED otherwise.
 */
int32_t sffs_metadata_header_check(struct sffs *fs, struct sffs_metadata_header *header) {
	assert(header != NULL);
	assert(fs != NULL);

	/* check magic number */
	if (header->magic != SFFS_METADATA_MAGIC) {
		return SFFS_METADATA_HEADER_CHECK_FAILED;
	}

	return SFFS_METADATA_HEADER_CHECK_OK;
}


/**
 * Try to fetch requested block of data from read cache. If requested data is not
 * found in the cache, read whole page from the flash.
 * 
 * @param fs TODO
 * @param addr TODO
 * @param data TODO
 * @param len TODO
 * 
 * @return TODO
 */
int32_t sffs_cached_read(struct sffs *fs, uint32_t addr, uint8_t *data, uint32_t len) {
	assert(fs != NULL);
	assert(data != NULL);
	
	if (flash_page_read(fs->flash, addr, data, len) != FLASH_PAGE_READ_OK) {
		return SFFS_CACHED_READ_FAILED;
	}
	
	return SFFS_CACHED_READ_OK;
}


/**
 * Write one page to cache.
 * 
 * @param fs A SFFS filesystem with cache.
 * @param addr Starting address of page to be written.
 * @param data Buffer with data.
 * @param len Length of data to be written.
 * 
 * @return SFFS_CACHED_WRITE_OK on success or
 *         SFFS_CACHED_WRITE_FAILED otherwise.
 */
int32_t sffs_cached_write(struct sffs *fs, uint32_t addr, uint8_t *data, uint32_t len) {
	assert(fs != NULL);
	assert(data != NULL);

	if (flash_page_write(fs->flash, addr, data, len) != FLASH_PAGE_WRITE_OK) {
		return SFFS_CACHED_WRITE_FAILED;
	}
	
	return SFFS_CACHED_WRITE_OK;
}


/**
 * Find data page for specified file_id and block.
 * 
 * @param fs A SFFS filesystem.
 * @param file_id File to search for.
 * @param block Block index within the file to search from.
 * @param addr Pointer to address which will be set to found data block.
 * 
 * @return SFFS_FIND_PAGE_OK on success,
 *         SFFS_FIND_PAGE_NOT_FOUND of no such file/block was found or
 *         SFFS_FIND_PAGE_FAILED otherwise.
 */
int32_t sffs_find_page(struct sffs *fs, uint32_t file_id, uint32_t block, struct sffs_page *page) {
	assert(fs != NULL);
	assert(page != NULL);
	
	/* first we need to iterate over all sectors in the flash */
	for (uint32_t sector = 0; sector < fs->sector_count; sector++) {
		struct sffs_metadata_header header;
		/* TODO: check return value */
		sffs_cached_read(fs, sector * fs->sector_size, (uint8_t *)&header, sizeof(header));
		
		//~ if (header.state == SFFS_SECTOR_STATE_ERASED) {
			//~ continue;
		//~ }
		
		for (uint32_t i = 0; i < fs->data_pages_per_sector; i++) {
			struct sffs_metadata_item item;
			sffs_get_page_metadata(fs, &(struct sffs_page){ .sector = sector, .page = i }, &item);

			/* check if page contains valid data for requested file */
			if (item.file_id == file_id && item.block == block && (item.state == SFFS_PAGE_STATE_USED || item.state == SFFS_PAGE_STATE_MOVING)) {
				page->sector = sector;
				page->page = i;
				return SFFS_FIND_PAGE_OK;
			}
		}
	}
	
	return SFFS_FIND_PAGE_NOT_FOUND;
}


/**
 * Try to find erased page suitable to be written with file contents.
 * 
 * @param fs A SFFS filesystem.
 * @param page Pointer to page structure which will be filled if page is found.
 * 
 * @return SFFS_FIND_ERASED_PAGE_OK on success,
 *         SFFS_FIND_ERASED_PAGE_NOT_FOUND if no erased page was found or
 *         SFFS_FIND_ERASED_PAGE_FAILED otherwise.
 */
int32_t sffs_find_erased_page(struct sffs *fs, struct sffs_page *page) {
	assert(fs != NULL);
	assert(page != NULL);
	
	/* first we need to iterate over all sectors in the flash */
	for (uint32_t sector = 0; sector < fs->sector_count; sector++) {
		struct sffs_metadata_header header;
		/* TODO: check return value */
		sffs_cached_read(fs, sector * fs->sector_size, (uint8_t *)&header, sizeof(header));
		
		if (header.state == SFFS_SECTOR_STATE_DIRTY || header.state == SFFS_SECTOR_STATE_FULL) {
			continue;
		} 
		
		for (uint32_t i = 0; i < fs->data_pages_per_sector; i++) {
			uint32_t item_pos = sector * fs->sector_size + sizeof(struct sffs_metadata_header) + i * sizeof(struct sffs_metadata_item);
			struct sffs_metadata_item item;
			/* TODO: check return value */
			sffs_cached_read(fs, item_pos, (uint8_t *)&item, sizeof(struct sffs_metadata_item));
			
			if (item.state == SFFS_PAGE_STATE_ERASED) {
				page->sector = sector;
				page->page = i;
				return SFFS_FIND_ERASED_PAGE_OK;
			}
		}
	}
	
	return SFFS_FIND_ERASED_PAGE_NOT_FOUND;
}


/**
 * Compute and return address of specified data page.
 * 
 * @param fs A SFFS filesystem.
 * @param page A page to return address of.
 * @param addr Pointer to page address.
 * 
 * @return SFFS_PAGE_ADDR_OK on success or
 *         SFFS_PAGE_ADDR_FAILED otherwise.
 */
int32_t sffs_page_addr(struct sffs *fs, struct sffs_page *page, uint32_t *addr) {
	assert(page != NULL);
	assert(addr != NULL);
	
	*addr = page->sector * fs->sector_size + (fs->first_data_page + page->page) * fs->page_size;
	
	return SFFS_PAGE_ADDR_OK;
}


int32_t sffs_sector_format(struct sffs *fs, uint32_t sector) {
	assert(fs != NULL);
	assert(sector < fs->sector_count);

	printf("sector format %d\n", sector);
	flash_sector_erase(fs->flash, sector * fs->sector_size);

	/* prepare and write sector header */
	struct sffs_metadata_header header;
	header.magic = SFFS_METADATA_MAGIC;
	header.state = SFFS_SECTOR_STATE_ERASED;
	/* TODO: fill other fields */
	flash_page_write(fs->flash, fs->sector_size * sector, (uint8_t *)&header, sizeof(header));

	/* prepare and write sector metadata items */
	for (uint32_t i = 0; i < fs->data_pages_per_sector; i++) {
		struct sffs_metadata_item item;
		item.file_id = 0xffff;
		item.block = 0xffff;
		item.state = SFFS_PAGE_STATE_ERASED;
		item.size = 0xffff;
		
		/* sffs_set_page_metadata cannot be used here as the remaining sector
		 * metadata are not complete yet and function will fail during sector
		 * metadata update */
		flash_page_write(fs->flash, fs->sector_size * sector + sizeof(header) + i * sizeof(item), (uint8_t *)&item, sizeof(item));
	}

	return SFFS_SECTOR_FORMAT_OK;
}


/**
 * Initiate garbage collection for a given sector. If the sector is marked as old,
 * it is being erased automatically. If the sector is dirty, garbage collector
 * finds out if it is worth it to move used page to other locations and erase
 * current dirty sector.
 * 
 * @param A SFFS filesystem.
 * @param sector A sector to perform garbage collection on.
 * 
 * @return SFFS_SECTOR_COLLECT_GARBAGE_OK on success or
 *         SFFS_SECTOR_COLLECT_GARBALE_FAILED otherwise.
 */
int32_t sffs_sector_collect_garbage(struct sffs *fs, uint32_t sector) {
	assert(fs != NULL);
	assert(sector < fs->sector_count);

	struct sffs_metadata_header header;
	if (sffs_cached_read(fs, sector * fs->sector_size, (uint8_t *)&header, sizeof(header)) != SFFS_CACHED_READ_OK) {
		return SFFS_UPDATE_SECTOR_METADATA_FAILED;
	}

	if (sffs_metadata_header_check(fs, &header) != SFFS_METADATA_HEADER_CHECK_OK) {
		return SFFS_UPDATE_SECTOR_METADATA_FAILED;
	}

	if (header.state == SFFS_SECTOR_STATE_OLD) {
		sffs_sector_format(fs, sector);
	}
	
	return SFFS_SECTOR_COLLECT_GARBAGE_OK;
}


/**
 * Function called after every page metadata write to update sector metadata.
 * It iterates over all pages contained in specified sector and determines sector
 * state. State is written to sector header afterwards.
 * 
 * @param fs A SFFS Filesystem.
 * @param sector Sector to update.
 * 
 * @return SFFS_UPDATE_SECTOR_METADATA_OK on success or
 *         SFFS_UPDATE_SECTOR_METADATA_FAILED otherwise.
 */
int32_t sffs_update_sector_metadata(struct sffs *fs, uint32_t sector) {
	assert(fs != NULL);
	assert(sector < fs->sector_count);

	struct sffs_metadata_header header;
	if (sffs_cached_read(fs, sector * fs->sector_size, (uint8_t *)&header, sizeof(header)) != SFFS_CACHED_READ_OK) {
		return SFFS_UPDATE_SECTOR_METADATA_FAILED;
	}

	if (sffs_metadata_header_check(fs, &header) != SFFS_METADATA_HEADER_CHECK_OK) {
		return SFFS_UPDATE_SECTOR_METADATA_FAILED;
	}
	
	uint8_t old_state = header.state;

	uint32_t p_erased = 0;
	uint32_t p_reserved = 0;
	uint32_t p_used = 0;
	uint32_t p_moving = 0;
	uint32_t p_old = 0;
	

	for (uint32_t i = 0; i < fs->data_pages_per_sector; i++) {
		struct sffs_metadata_item item;
		sffs_get_page_metadata(fs, &(struct sffs_page){ .sector = sector, .page = i }, &item);
		
		if (item.state == SFFS_PAGE_STATE_ERASED) p_erased++;
		if (item.state == SFFS_PAGE_STATE_RESERVED) p_reserved++;
		if (item.state == SFFS_PAGE_STATE_USED) p_used++;
		if (item.state == SFFS_PAGE_STATE_MOVING) p_moving++;
		if (item.state == SFFS_PAGE_STATE_OLD) p_old++;
	}

	int update_ok = 0;

	if (p_erased == fs->data_pages_per_sector && p_reserved == 0 && p_used == 0 && p_moving == 0 && p_old == 0) {
		header.state = SFFS_SECTOR_STATE_ERASED;
		update_ok = 1;
	}

	if (p_erased > 0 && (p_reserved > 0 || p_used > 0 || p_moving > 0 || p_old > 0)) {
		header.state = SFFS_SECTOR_STATE_USED;
		update_ok = 1;
	}

	if (p_erased == 0 && p_old == 0) {
		header.state = SFFS_SECTOR_STATE_FULL;
		update_ok = 1;
	}
	
	if (p_erased == 0 && (p_reserved + p_used + p_moving + p_old) == fs->data_pages_per_sector) {
		header.state = SFFS_SECTOR_STATE_DIRTY;
		update_ok = 1;
	}
	
	if (p_old == fs->data_pages_per_sector) {
		header.state = SFFS_SECTOR_STATE_OLD;
		update_ok = 1;
	}
	
	if (update_ok == 1) {
		/* New sector state cannot be greater than the old one
		 * (otherwise the sector metadata would need to be erased first).
		 * This would indicate error in metadata manipulation. */
		assert(header.state <= old_state);
		
		if (old_state != header.state) {
		
			if (sffs_cached_write(fs, sector * fs->sector_size, (uint8_t *)&header, sizeof(header)) != SFFS_CACHED_WRITE_OK) {
				return SFFS_UPDATE_SECTOR_METADATA_FAILED;
			}
			
			/* sffs_sector_collect_garbage(fs, sector); */
		}
		
		return SFFS_UPDATE_SECTOR_METADATA_OK;
	}

	assert(0);
	return SFFS_UPDATE_SECTOR_METADATA_FAILED;
}


int32_t sffs_get_page_metadata(struct sffs *fs, struct sffs_page *page, struct sffs_metadata_item *item) {
	assert(fs != NULL);
	assert(page != NULL);
	assert(item != NULL);
	
	uint32_t item_pos = page->sector * fs->sector_size + sizeof(struct sffs_metadata_header) + page->page * sizeof(struct sffs_metadata_item);
	sffs_cached_read(fs, item_pos, (uint8_t *)item, sizeof(struct sffs_metadata_item));
	
	return SFFS_GET_PAGE_METADATA_OK;
}


int32_t sffs_set_page_metadata(struct sffs *fs, struct sffs_page *page, struct sffs_metadata_item *item) {
	assert(fs != NULL);
	assert(page != NULL);
	assert(item != NULL);

	uint32_t item_pos = page->sector * fs->sector_size + sizeof(struct sffs_metadata_header) + page->page * sizeof(struct sffs_metadata_item);
	sffs_cached_write(fs, item_pos, (uint8_t *)item, sizeof(struct sffs_metadata_item));
	
	sffs_update_sector_metadata(fs, page->sector);
	
	return SFFS_SET_PAGE_MATEDATA_OK;
}


int32_t sffs_set_page_state(struct sffs *fs, struct sffs_page *page, uint8_t page_state) {
	assert(fs != NULL);
	assert(page != NULL);

	struct sffs_metadata_item item;
	sffs_get_page_metadata(fs, page, &item);
	item.state = page_state;
	sffs_set_page_metadata(fs, page, &item);

	return SFFS_SET_PAGE_STATE_OK;
}


/**
 * Check if specified file is opened.
 * 
 * @param f SFFS file to be checked.
 * 
 * @return SFFS_CHECK_FILE_OPENED_OK of the file is opened or
 *         SFFS_CHECK_FILE_OPENED_FAILED otherwise.
 */
int32_t sffs_check_file_opened(struct sffs_file *f) {
	assert(f != NULL);
	
	/* file is probably not opened. TODO: better check */
	if (f->file_id == 0 || f->fs == NULL) {
		return SFFS_CHECK_FILE_OPENED_FAILED;
	}
	
	return SFFS_CHECK_FILE_OPENED_OK;
}


/**
 * Open a file by its ID.
 * 
 * TODO: mode - read, overwrite, append.
 * 
 * @param fs A SFFS filesystem with the file.
 * @param f SFFS file structure.
 * @param id ID of file to be opened.
 * 
 * @return SFFS_OPEN_ID_OK on success or
 *         SFFS_OPEN_ID_FAILED otherwise.
 */
int32_t sffs_open_id(struct sffs *fs, struct sffs_file *f, uint32_t file_id) {
	assert(fs != NULL);
	assert(f != NULL);
	assert(file_id != 0xffff);
	
	f->pos = 0;
	f->fs = fs;
	f->file_id = file_id;
	
	return SFFS_OPEN_ID_OK;
}


/**
 * Close a previously opened file.
 * 
 * @param f SFFS File to close.
 * 
 * @return SFFS_CLOSE_OK on success or
 *         SFFS_CLOSE_FAILED otherwise.
 */
int32_t sffs_close(struct sffs_file *f) {
	assert(f != NULL);

	if (sffs_check_file_opened(f) != SFFS_CHECK_FILE_OPENED_OK) {
		return SFFS_CLOSE_FAILED;
	}

	f->file_id = 0;
	f->fs = NULL;
	
	return SFFS_CLOSE_OK;
}


/**
 * Write data buffer to an opened file at current position. Position in the file
 * is updated afterwards.
 * 
 * @param f SFFS file to write data to.
 * @param buf Buffer containing data to be written.
 * @param len Length of data to be written.
 * 
 * @return number of bytes written or -1 if error occured.
 */
int32_t sffs_write(struct sffs_file *f, unsigned char *buf, uint32_t len) {
	assert(f != NULL);
	assert(buf != NULL);
	
	if (sffs_check_file_opened(f) != SFFS_CHECK_FILE_OPENED_OK) {
		return -1;
	}

	/* Write buffer can span multiple flash blocks. We need to determine
	 * where the buffer starts and ends. */
	uint32_t b_start = f->pos / f->fs->page_size;
	uint32_t b_end = (f->pos + len - 1) / f->fs->page_size;

	/* now we can iterate over all flash pages which need to be modified */
	for (uint32_t i = b_start; i <= b_end; i++) {

		uint8_t page_data[f->fs->page_size];
		struct sffs_page page;
		uint32_t loaded_old = 0;
		
		if (sffs_find_page(f->fs, f->file_id, i, &page) == SFFS_FIND_PAGE_OK) {
			/* page is valid. Get its address and read from flash */
			uint32_t addr;
			sffs_page_addr(f->fs, &page, &addr);
			if (sffs_cached_read(f->fs, addr, page_data, sizeof(page_data)) != SFFS_CACHED_READ_OK) {
				return -1;
			}
			
			loaded_old = 1;
		} else {
			/* the file doesn't have allocated requested page. Create
			 * new one filled with zeroes */
			memset(page_data, 0x00, sizeof(page_data));
		}
		
		/* determine where data wants to be written */
		uint32_t data_start = f->pos;
		uint32_t data_end = f->pos + len - 1;
		
		/* crop to current page boundaries */
		data_start = MAX(data_start, i * f->fs->page_size);
		data_end = MIN(data_end, (i + 1) * f->fs->page_size - 1);
		
		/* get offset in the source buffer */
		uint32_t source_offset = data_start - f->pos;
		
		/* get offset in the destination buffer */
		uint32_t dest_offset = data_start % f->fs->page_size;
		
		/* length of data to be writte is the difference between cropped data */
		uint32_t dest_len = data_end - data_start + 1;

		//~ printf("writing buf[%d-%d] to page %d, offset %d, length %d\n", source_offset, source_offset + dest_len, i, dest_offset, dest_len);
		
		assert(source_offset < len);
		assert(dest_offset < f->fs->page_size);
		assert(dest_len <= f->fs->page_size);
		assert(dest_len <= len);
		
		/* TODO: write actual data */
		memcpy(&(page_data[dest_offset]), &(buf[source_offset]), dest_len);
		
		/* find new erased page and mark is as reserved (prepared for writing).
		 * Mark old page as moving. */
		struct sffs_page new_page;
		if (sffs_find_erased_page(f->fs, &new_page) != SFFS_FIND_ERASED_PAGE_OK) {
			return -1;
		}
		
		if (loaded_old) {
			sffs_set_page_state(f->fs, &page, SFFS_PAGE_STATE_MOVING);
		}
		sffs_set_page_state(f->fs, &new_page, SFFS_PAGE_STATE_RESERVED);
		
		/* Finally write modified page, set its state to used and set state of
		 * old page to old- */
		uint32_t addr;
		sffs_page_addr(f->fs, &new_page, &addr);
		if (sffs_cached_write(f->fs, addr, page_data, sizeof(page_data)) != SFFS_CACHED_WRITE_OK) {
			return -1;
		}
		
		if (loaded_old) {
			sffs_set_page_state(f->fs, &page, SFFS_PAGE_STATE_OLD);
		}
		struct sffs_metadata_item item;
		item.block = i;
		item.size = data_end % f->fs->page_size + 1;
		item.state = SFFS_PAGE_STATE_USED;
		item.file_id = f->file_id;
		sffs_set_page_metadata(f->fs, &new_page, &item);
	}
	
	f->pos += len;
	
	return len;
}


/**
 * Read up to len bytes of data from specified opened file starting at actual
 * position.
 * 
 * @param f A SFFS File.
 * @param buf A buffer for read data.
 * @param len Length of data to be read from the file.
 * 
 * @return -1 on error,
 *         0 if no more data can be read or
 *         number of bytes read.
 */
int32_t sffs_read(struct sffs_file *f, unsigned char *buf, uint32_t len) {
	assert(f != NULL);
	assert(buf != NULL);
	assert(len > 0);

	if (sffs_check_file_opened(f) != SFFS_CHECK_FILE_OPENED_OK) {
		return -1;
	}

	uint32_t b_start = f->pos / f->fs->page_size;
	uint32_t b_end = (f->pos + len - 1) / f->fs->page_size;

	uint32_t bytes_read = 0;

	/* iterate over all flash pages which need to be read */
	for (uint32_t i = b_start; i <= b_end; i++) {


		uint8_t page_data[f->fs->page_size];
		struct sffs_page page;
		uint32_t dest_len;
		
		if (sffs_find_page(f->fs, f->file_id, i, &page) == SFFS_FIND_PAGE_OK) {
			uint32_t addr;
			sffs_page_addr(f->fs, &page, &addr);
			if (sffs_cached_read(f->fs, addr, page_data, sizeof(page_data)) != SFFS_CACHED_READ_OK) {
				return -1;
			}

			struct sffs_metadata_item item;
			sffs_get_page_metadata(f->fs, &page, &item);

			uint32_t file_crop = i * f->fs->page_size + item.size - 1;
			uint32_t data_start = f->pos;
			uint32_t data_end = f->pos + len - 1;
			
			/* crop to current page boundaries */
			data_start = MAX(data_start, i * f->fs->page_size);
			data_end = MIN(data_end, (i + 1) * f->fs->page_size - 1);
			data_end = MIN(data_end, file_crop);
			
			/* get offset in the source buffer */
			uint32_t source_offset = data_start - f->pos;
			
			/* get offset in the destination buffer */
			uint32_t dest_offset = data_start % f->fs->page_size;
			
			/* length of data to be writte is the difference between cropped data */
			dest_len = data_end - data_start + 1;
	
			assert(source_offset < len);
			assert(dest_offset < f->fs->page_size);
			assert(dest_len <= f->fs->page_size);
			assert(dest_len <= len);

			//~ printf("reading from page %d, offset %d, length %d to  to buf[%d-%d]\n", i, dest_offset, dest_len, source_offset, source_offset + dest_len);
			memcpy(&(buf[source_offset]), &(page_data[dest_offset]), dest_len);
		} else {
			/* no more bytes to read */
			break;
		}
		
		bytes_read += dest_len;
	}
	
	f->pos += bytes_read;

	return bytes_read;
}


/**
 * Update write/read position within an opened file.
 * 
 * @param f SFFS file.
 * 
 * @return SFFS_SEEK_OK on success or
 *         SFFS_SEEK_FAILED otherwise.
 */
int32_t sffs_seek(struct sffs_file *f) {
	assert(f != NULL);

	return SFFS_SEEK_OK;
}


/**
 * Removes all blocks for file @a file_id. File should be closed during removal.
 * 
 * @param fs A SFFS filesystem.
 * @param file_id ID of the file to remove.
 * 
 * @return SFFS_FILE_REMOVE_OK on success or
 *         SFFS_FILE_REMOVE_FAILED otherise.
 */
int32_t sffs_file_remove(struct sffs *fs, uint32_t file_id) {
	assert(fs != NULL);
	
	/* cannot remove filesystem metadata file */
	if (file_id == 0) {
		return SFFS_FILE_REMOVE_FAILED;
	}
	
	/* iterate over file blocks until first find fails, mark all found blocks
	 * as old (remove from file).
	 * TODO: this action should probably be done in a safer way, interrupting
	 * deletion sequence will cause garbage to be left in the filesystem
	 * (file tail) */
	uint32_t block = 0;
	struct sffs_page page;
	while (sffs_find_page(fs, file_id, block, &page) == SFFS_FIND_PAGE_OK) {
		sffs_set_page_state(fs, &page, SFFS_PAGE_STATE_OLD);
		block++;
	}
}



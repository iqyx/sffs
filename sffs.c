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

#include "sffs.h"
#include "flash_emulator.h"


int32_t sffs_init(struct sffs *fs) {
	
}


int32_t sffs_mount(struct sffs *fs, struct flash_dev *flash) {
	
}


int32_t sffs_free(struct sffs *fs) {
	
}


/**
 * Create new SFFS filesystem on flash memory. Flash memory cannot be mounted
 * during this operation. Information about memory geometry is fetched directly
 * from flash.
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
	
	/* TODO: compute data page count */
	uint32_t data_page_count = 0;
	
	for (uint32_t sector = 0; sector < (info.capacity / info.sector_size); sector++) {

		/* prepare and write sector header */
		struct sffs_metadata_header header;
		header.magic = SFFS_METADATA_MAGIC;
		header.state = SFFS_SECTOR_STATE_ERASED;
		/* TODO: fill other fields */
		
		flash_page_write(flash, info.sector_size * sector, (uint8_t *)&header, sizeof(header));

		/* prepare and write sector metadata items */
		for (uint32_t i = 0; i < data_page_count; i++) {
			struct sffs_metadata_item item;
			item.file_id = 0xffff;
			item.block = 0xffff;
			item.state = 0xff;
			item.size = 0xffff;
			
			flash_page_write(flash, info.sector_size * sector + sizeof(header) + i * sizeof(item), (uint8_t *)&item, sizeof(item));
		}
	}
	
	return SFFS_FORMAT_OK;
}


int32_t sffs_debug_print(struct sffs *fs) {
	
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

	/* there is more metadata pages than the sector can contain */
	if (header->metadata_page_count >= (fs->sector_size / fs->page_size)) {
		return SFFS_METADATA_HEADER_CHECK_FAILED;
	}
	
	return SFFS_METADATA_HEADER_CHECK_OK;
}


/**
 * Try to fetch requested block of data from read cache. If requested data
 * is not found in the cache, read whole page from the flash.
 * 
 * @param fs TODO
 * @param addr TODO
 * @param data TODO
 * @param len TODO
 * 
 * @return TODO
 */
int32_t sffs_cached_read(struct sffs *fs, uint32_t addr, uint8_t *data, uint32_t len) {
	
	
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
int32_t sffs_find_page(struct sffs *fs, uint32_t file_id, uint32_t block, uint32_t *addr, uint8_t page_state) {
	assert(fs != NULL);
	assert(addr != NULL);
	
	/* first we need to iterate over all sectors in the flash */
	for (uint32_t sector = 0; sector < fs->sector_count; sector++) {
		struct sffs_metadata_header header;
		/* TODO: check return value */
		sffs_cached_read(fs, sector * fs->sector_size, (uint8_t *)&header, sizeof(header));
		
		/* if the sector is "dirty" or "erased", skip it */
		if (header.state == SFFS_SECTOR_STATE_DIRTY || header.state == SFFS_SECTOR_STATE_ERASED) {
			continue;
		} 
		
		for (uint32_t i = 0; i < fs->data_pages_per_sector; i++) {
			uint32_t item_pos = sector * fs->sector_size + sizeof(struct sffs_metadata_header) + i * sizeof(struct sffs_metadata_item);
			struct sffs_metadata_item item;
			/* TODO: check return value */
			sffs_cached_read(fs, item_pos, (uint8_t *)&item, sizeof(struct sffs_metadata_item));
		}
	}
	





	
	return SFFS_FIND_PAGE_NOT_FOUND;
}




int32_t sffs_open_id() {
	
}


int32_t sffs_close() {
	
}


int32_t sffs_write() {
	
}


int32_t sffs_read() {
	
}


int32_t sffs_seek() {
	
}




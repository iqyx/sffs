/**
 * Flash emulator emulates standard generic NOR flash with following functionality:
 *   - a block of data can be read from any location up to the page length
 *   - a block of data can be written to any location with length lower
 *     or equal to page length. Only bits set to 0 will be written (ie. bit can
 *     be written from 1 to 0 but not the other way around)
 *   - block erase - the whole eraseblock will be erased (all bits set to 1)
 *   - chip erase - the whole chip will be erased (all bits set to 1)
 *   - get geometry can be used to fetch information about flash size and
 *     block sizes (page/erase block)
 */

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#include "flash_emulator.h"


/**
 * Inits virtual flash in memory and allocates required space for temporary
 * flash data storage.
 * 
 * @param flash A flash device to initialize.
 * 
 * @return FLASH_INIT_OK on success or
 *         FLASH_INIT_FAILED otherwise.
 */
int32_t flash_init(struct flash_dev *flash, uint32_t size) {
	assert(flash != NULL);
	
	flash->page_size = FLASH_EMU_PAGE_SIZE;
	flash->sector_size = FLASH_EMU_SECTOR_SIZE;
	flash->block_size = FLASH_EMU_BLOCK_SIZE;
	flash->size = size;
	
	flash->data = malloc(flash->size);
	if (flash->data == NULL) {
		return FLASH_INIT_FAILED;
	}

	return FLASH_INIT_OK;
}


/**
 * Frees all resources used by virtual flash device (this means all flash
 * data will be lost, data is not saved to any nonvolatile memory)
 * 
 * @param flash A flash device to free.
 * 
 * @return FLASH_FREE_OK on success or
 *         FLASH_FREE_FAILED otherwise.
 */
int32_t flash_free(struct flash_dev *flash) {
	assert(flash != NULL);

	assert(flash->data != NULL);
	free(flash->data);
	flash->size = 0;

	return FLASH_FREE_OK;
}


/**
 * Get flash chip specification. Currently hard-coded for testing
 * purposes only.
 * 
 * @param flash A flash device to fetch info from.
 * @param info Pointer to structure which will be filled with flash info.
 * 
 * @return FLASH_GET_INFO_OK on success or
 *         FLASH_GET_INFO_FAILED otherwise.
 */
int32_t flash_get_info(struct flash_dev *flash, struct flash_info *info) {
	assert(flash != NULL);
	assert(info != NULL);

	info->capacity = flash->size;
	info->page_size = flash->page_size;
	info->sector_size = flash->sector_size;
	info->block_size = flash->block_size;
	
	return FLASH_GET_INFO_OK;
}


/**
 * Erase the whole emulated chip.
 * 
 * @param flash A flash device to erase.
 * 
 * @return FLASH_CHIP_ERASE_OK on success or
 *         FLASH_CHIP_ERASE_FAILED otherwise.
 */
int32_t flash_chip_erase(struct flash_dev *flash) {
	assert(flash != NULL);
	
	memset(flash->data, 0xff, flash->size);
}


/**
 * Flash block erase.
 * 
 * @param flash A flash device to erase.
 * @param addr Starting address of the block to be erased. Other addresses
 *             in the block are considered invalid.
 * 
 * @return FLASH_BLOCK_ERASE_OK on success or
 *         FLASH_BLOCK_ERASE_FAILED otherwise.
 */
int32_t flash_block_erase(struct flash_dev *flash, const uint32_t addr) {
	assert(flash != NULL);

	assert(addr < flash->size);
	assert((addr % flash->block_size) == 0);
	
	memset(&(flash->data[addr]), 0xff, flash->block_size);
	
	return FLASH_BLOCK_ERASE_OK;
}


/**
 * Flash sector erase.
 * 
 * @param flash A flash device to erase.
 * @param addr Starting address of the sector to be erased. Other addresses
 *             in the sector are considered invalid.
 * 
 * @return FLASH_SECTOR_ERASE_OK on success or
 *         FLASH_SECTOR_ERASE_FAILED otherwise.
 */
int32_t flash_sector_erase(struct flash_dev *flash, const uint32_t addr) {
	assert(flash != NULL);

	assert(addr < flash->size);
	assert((addr % flash->sector_size) == 0);

	memset(&(flash->data[addr]), 0xff, flash->sector_size);
	
	return FLASH_SECTOR_ERASE_OK;
}


/**
 * Program data (turn 1's to 0's) in chunks of size up to the page size.
 * Data to be written cannot cross page boundaries.
 * 
 * @param flash A flash to write bytes to.
 * @param addr Starting address.
 * @param *data Pointer to data buffer to be written.
 * @param len Length of data to be written.
 * 
 * @return FLASH_PAGE_WRITE_OK on success or
 *         FLASH_PAGE_WRITE_FAILED otherwise.
 */
int32_t flash_page_write(struct flash_dev *flash, const uint32_t addr, const uint8_t *data, const uint32_t len) {
	assert(flash != NULL);
	
	assert(len > 0);
	assert(len <= flash->page_size);
	assert((addr + len) <= flash->size);
	assert((addr + len) <= ((addr / flash->page_size) * flash->page_size + flash->page_size));
	assert(data != NULL);

	for (uint32_t i = 0; i < len; i++) {
		flash->data[addr + i] &= data[i];
		if (flash->data[addr + i] != data[i]) {
			printf("flash_emulator: bad write, erasing required, addr = %d, byte = %02x, new value = %02x\n", addr + i, flash->data[addr + i], data[i]);
		}
		assert(flash->data[addr + i] == data[i]);
	}

	return FLASH_PAGE_WRITE_OK;
}


int32_t flash_page_read(struct flash_dev *flash, const uint32_t addr, uint8_t *data, const uint32_t len) {
	assert(flash != NULL);
	
	assert(len > 0);
	assert(len <= flash->page_size);
	assert((addr + len) <= flash->size);
	assert((addr + len) <= ((addr / flash->page_size) * flash->page_size + flash->page_size));
	assert(data != NULL);

	memcpy(data, &(flash->data[addr]), len);
	
	return FLASH_PAGE_READ_OK;
	
}

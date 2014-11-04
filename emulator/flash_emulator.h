#ifndef _FLASH_EMULATOR_H_
#define _FLASH_EMULATOR_H_



#define FLASH_EMU_PAGE_SIZE 256
#define FLASH_EMU_SECTOR_SIZE 4096
#define FLASH_EMU_BLOCK_SIZE 65536


struct flash_dev {
	uint32_t page_size;
	uint32_t sector_size;
	uint32_t block_size;
	uint32_t size;

	uint8_t *data;
};


struct flash_info {
	uint32_t capacity;
	uint32_t page_size;
	uint32_t sector_size;
	uint32_t block_size;
	
};


int32_t flash_init(struct flash_dev *flash, uint32_t size);
#define FLASH_INIT_OK 0
#define FLASH_INIT_FAILED -1

int32_t flash_free(struct flash_dev *flash);
#define FLASH_FREE_OK 0
#define FLASH_FREE_FAILED -1

int32_t flash_get_info(struct flash_dev *flash, struct flash_info *info);
#define FLASH_GET_INFO_OK 0
#define FLASH_GET_INFO_FAILED -1

int32_t flash_chip_erase(struct flash_dev *flash);
#define FLASH_CHIP_ERASE_OK 0
#define FLASH_CHIP_ERASE_FAILED -1

int32_t flash_block_erase(struct flash_dev *flash, const uint32_t addr);
#define FLASH_BLOCK_ERASE_OK 0
#define FLASH_BLOCK_ERASE_FAILED -1

int32_t flash_sector_erase(struct flash_dev *flash, const uint32_t addr);
#define FLASH_SECTOR_ERASE_OK 0
#define FLASH_SECTOR_ERASE_FAILED -1

int32_t flash_page_write(struct flash_dev *flash, const uint32_t addr, const uint8_t *data, const uint32_t len);
#define FLASH_PAGE_WRITE_OK 0
#define FLASH_PAGE_WRITE_FAILED -1

int32_t flash_page_read(struct flash_dev *flash, const uint32_t addr, uint8_t *data, const uint32_t len);
#define FLASH_PAGE_READ_OK 0
#define FLASH_PAGE_READ_FAILED -1


#endif

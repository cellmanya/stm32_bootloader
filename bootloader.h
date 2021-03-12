/*
 * bootloader.h
 *
 *  Created on: Feb 27, 2021
 *      Author: CELL
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_


#include <stdint.h>

#define STM32F1

#ifdef STM32F1
#include "stm32f1xx.h"

#else
#error "Target MCU is not defined!"
#endif

//#include "stm32_hal_legacy.h"


#define SRAM_SIZE							(20*1024) // 20 KB of SRAM
#define SRAM_END							(SRAM_BASE + SRAM_SIZE)
#define FLASH_SIZE							(64*1024)
#define START_APP_ADDR						(0x08008000UL)

#define SRAM_SIZE							(20*1024) // 20 KB of SRAM
#define SRAM_END							(SRAM_BASE + SRAM_SIZE)
#define FLASH_SIZE							(64*1024)
#define START_APP_ADDR						(0x08008000UL)


#define GETID_CMD							(0x05)
#define ERASE_CMD							(0x08)
#define WRITE_CMD							(0x11)

#define ACK_SIGNAL							(0x30)
#define NACK_SIGNAL							(0x35)

#define FLASH_PAGE_NBPERBANK 				(256)
#define FLASH_NUM_OF_PAGES					((FLASH_BASE + FLASH_SIZE - START_APP_ADDR) / FLASH_PAGE_SIZE)
#define FLASH_MAX_NUM_OF_PAGES				(256)

#define USE_WRITE_PROTECTION_BOOTLOADER		(1)
#define USE_WRITE_PROTECTION_APP_BANK1		(0)
#define USE_WRITE_PROTECTION_APP_BANK2		(0)


typedef struct{

	uint32_t option_type;
	uint32_t wrp_page;
	uint8_t rdp_level;
	uint8_t user_config;

}Protection_Info_t;

void Bootloader_GetID(uint8_t * rec_string);
HAL_StatusTypeDef Bootloader_Erase(uint8_t * payload);
void Bootloader_Write(uint8_t * payload);
Protection_Info_t Bootloader_GetProtectionInfo();
void Bootloader_SetProtection(Protection_Info_t protection);



#endif /* BOOTLOADER_H_ */

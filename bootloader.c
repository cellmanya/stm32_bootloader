/*
 * bootloader.c
 *
 *  Created on: Feb 27, 2021
 *      Author: CELL
 */
#include <string.h>
#include "bootloader.h"

extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
/*
uint32_t * vector_table[]  = {

		(uint32_t *) SRAM_END,
		(uint32_t *)Reset_Handler,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		(uint32_t *)SysTick_Handler
};
*/

void Bootloader_GetID(uint8_t * payload){

	uint16_t deviceID;
	uint32_t crc_var = 0;
	uint32_t cmd = payload[0];

	memcpy(&crc_var, payload + 1, sizeof(uint32_t));

		if(crc_var == HAL_CRC_Calculate(&hcrc, &cmd, 1)){

			deviceID = (uint16_t)(DBGMCU->IDCODE & 0xFFF);

			payload[0] = ACK_SIGNAL;
			HAL_UART_Transmit(&huart1, payload, 1, HAL_MAX_DELAY);
			HAL_UART_Transmit(&huart1, (uint8_t *)&deviceID, 2, HAL_MAX_DELAY);
		}

		else {

			payload[0] = NACK_SIGNAL;
			HAL_UART_Transmit(&huart1, payload, 1, HAL_MAX_DELAY);

		}

}

HAL_StatusTypeDef Bootloader_Erase(uint8_t * payload){

	uint32_t page_error = 0;
	uint32_t crc_var = 0;
	uint32_t cmd_arr[] = { payload[0], payload[1] };
	HAL_StatusTypeDef status = HAL_OK;

	memcpy(&crc_var, payload + 2, sizeof(uint32_t));

		if(crc_var == HAL_CRC_Calculate(&hcrc, cmd_arr, 2) &&
				(payload[1] > 16 && (payload[1] < FLASH_NUM_OF_PAGES || payload[1] == 0xFF))){

				FLASH_EraseInitTypeDef erase_var;

				erase_var.Banks = FLASH_BANK_1;
				erase_var.PageAddress = START_APP_ADDR;
				erase_var.NbPages = payload[1] == 0xFF ? FLASH_NUM_OF_PAGES : payload[1];
				erase_var.TypeErase = FLASH_TYPEERASE_PAGES;

				HAL_FLASH_Unlock();
				status = HAL_FLASHEx_Erase(&erase_var, &page_error);
				HAL_FLASH_Lock();


				#if defined(FLASH_BANK2_END) && status

					erase_var.Banks = FLASH_BANK_2;
					erase_var.PageAddress = FLASH_BANK1_END;
					erase_var.NbPages = payload[1] == 0xFF ? FLASH_NUM_OF_PAGES : payload[1] - FLASH_NUM_OF_PAGES;
					erase_var.TypeErase = FLASH_TYPEERASE_PAGES;

					HAL_FLASH_Unlock();
					status = HAL_FLASHEx_Erase(&erase_var, &page_error);
					HAL_FLASH_Lock();

				#endif

				payload[0] = ACK_SIGNAL;
				HAL_UART_Transmit(&huart1, (uint8_t *) payload, 1, HAL_MAX_DELAY);
		}

		else {

				payload[0] = NACK_SIGNAL;
				HAL_UART_Transmit(&huart1, payload, 1, HAL_MAX_DELAY);
		}

		return status;

}

void Bootloader_Write(uint8_t * payload){


	uint32_t crc_var = 0;
	uint32_t addr;

	memcpy(&addr, payload + 1, sizeof(uint32_t));
	memcpy(&crc_var, payload + 5, sizeof(uint32_t));

	if(crc_var == HAL_CRC_Calculate(&hcrc, payload, 5) && addr >= START_APP_ADDR){

		payload[0] = ACK_SIGNAL;
		HAL_UART_Transmit(&huart1, (uint8_t *) payload, 1, HAL_MAX_DELAY);

		if(HAL_UART_Receive(&huart1, payload, 32 + 4, 200) == HAL_TIMEOUT) // 32 byte data and 4 byte CRC-32 value
			return;

		memcpy(&crc_var, payload + 32, sizeof(uint32_t));

			if(crc_var == HAL_CRC_Calculate(&hcrc, payload, 32)){

				HAL_FLASH_Unlock();

				for( int i = 0; i < 32; i+=4 ){

					HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, payload[i]);
					addr += 4;

				}

				HAL_FLASH_Lock();

				payload[0] = ACK_SIGNAL;
				HAL_UART_Transmit(&huart1, (uint8_t *) payload, 1, HAL_MAX_DELAY);
			}

			else {

			goto NACK;

			}

	}

	else {

		goto NACK;

	}


	NACK:

	payload[0] = NACK_SIGNAL;
	HAL_UART_Transmit(&huart1, (uint8_t *) payload, 1, HAL_MAX_DELAY);


}

Protection_Info_t Bootloader_GetProtectionInfo(void){

	FLASH_OBProgramInitTypeDef obconfig;
	Protection_Info_t 		   protection;

	HAL_FLASH_Unlock();

	HAL_FLASHEx_OBGetConfig(&obconfig);

	protection.option_type = obconfig.OptionType;
	protection.wrp_page = obconfig.WRPPage;
	protection.rdp_level = obconfig.RDPLevel;
	protection.user_config = obconfig.USERConfig;

	HAL_FLASH_Lock();

	return protection;
}

void Bootloader_SetProtection(void){

	FLASH_OBProgramInitTypeDef obconfig;
	HAL_StatusTypeDef status = HAL_OK;

	Protection_Info_t protection = Bootloader_GetProtectionInfo();

	#if (USE_WRITE_PROTECTION_BOOTLOADER)

	// Checking the write protection status of the custom bootloader pages

		for(int i = 0; i < 8; i++){ // 32 pages for custom bootloader

			if(protection.wrp_page & (1 << i) == (1 << i)){

				HAL_FLASH_Unlock();
				HAL_FLASH_OB_Unlock();

				obconfig.Banks = FLASH_BANK_1;
				obconfig.WRPState = OB_WRPSTATE_ENABLE;
				obconfig.WRPPage = (1 << i); // Pages n to n+4
				obconfig.OptionType = OPTIONBYTE_WRP;

				status = HAL_FLASHEx_OBProgram(&obconfig);

				HAL_FLASH_OB_Launch();

			}
		}

	#endif

	#if	(USE_WRITE_PROTECTION_APP_BANK1)

	// Checking the write protection status of the application space BANK 1

		for(int i = 8; i < 32; i++){ // 96 pages for application

			if(protection.wrp_page & (1 << i) == (1 << i)){

				HAL_FLASH_Unlock();
				HAL_FLASH_OB_Unlock();

				obconfig.Banks = FLASH_BANK_1;
				obconfig.WRPState = OB_WRPSTATE_ENABLE;
				obconfig.WRPPage = (1 << i); // Pages n to n+4
				obconfig.OptionType = OPTIONBYTE_WRP;

				status = HAL_FLASHEx_OBProgram(&obconfig);

				HAL_FLASH_OB_Launch();

			}
		}

	#endif

	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();

}

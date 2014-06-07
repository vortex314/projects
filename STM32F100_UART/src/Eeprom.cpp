/*
 * Eeprom.cpp
 *
 *  Created on: 2-nov.-2013
 *      Author: lieven2
 */

#include <Eeprom.h>
#include <stm32f10x_flash.h>
#include <string.h>
Eeprom::Eeprom() {
	// TODO Auto-generated constructor stub

}

Eeprom::~Eeprom() {
	// TODO Auto-generated destructor stub
}
//________________________________________________________________________________
// EEPROM REGION AT END of FLASH
//
#define PAGE_SIZE  (u16)0x400  /* 1KByte for medium sized */
#define EEPROM_START_ADDRESS  (u32)(0x08000000+((128-2)*1024))
#define PAGE0_BASE_ADDRESS  (u32)(EEPROM_START_ADDRESS + (u16)0x0000)
#define PAGE0_END_ADDRESS   (u32)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1))
#define PAGE1_BASE_ADDRESS  (u32)(EEPROM_START_ADDRESS + PAGE_SIZE)
#define PAGE1_END_ADDRESS   (u32)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1))

/* Used Flash pages for EEPROM emulation */
#define PAGE0    (u16)0x0000
#define PAGE1    (u16)0x0001

/* No valid page define */
#define NO_VALID_PAGE    (u16)0x00AB

/* Page status definitions */
#define ERASED             (u16)0xFFFF      /* PAGE is empty */
#define RECEIVE_DATA       (u16)0xEEEE      /* PAGE is marked to receive data */
#define VALID_PAGE         (u16)0x0000      /* PAGE containing valid data */

/* Valid pages in read and write defines */
#define READ_FROM_VALID_PAGE    (u8)0x00
#define WRITE_IN_VALID_PAGE     (u8)0x01

/* Page full define */
#define PAGE_FULL    (u8)0x80

/* Variables' number */
#define NumbOfVar  (u8)0x03

Erc Eeprom::erase() {
	FLASH_Status FlashStatus = FLASH_COMPLETE;
	FLASH_Unlock(); //unlock flash writing
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);
	FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);
	FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
//	FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
	FLASH_Lock();
	return E_OK;
}



uint8_t* Eeprom::findVar(char *name){

}



/*******************************************************************************
* Function Name  : EE_FindValidPage
* Description    : Find valid Page for write or read operation
* Input          : - Operation: operation to achieve on the valid page:
*                      - READ_FROM_VALID_PAGE: read operation from valid page
*                      - WRITE_IN_VALID_PAGE: write operation from valid page
* Output         : None
* Return         : Valid page number (PAGE0 or PAGE1) or NO_VALID_PAGE in case
*                  of no valid page was found
*******************************************************************************/
u16 EE_FindValidPage(u8 Operation)
{
  u16 PageStatus0 = 6, PageStatus1 = 6;

  /* Get Page0 actual status */
#ifdef MOD_MTHOMAS_STMLIB
  /* workaround a warning - search better solution */
  vu32 t = PAGE0_BASE_ADDRESS;
  PageStatus0 = (*(vu16*)t);
#else
  PageStatus0 = (*(vu16*)PAGE0_BASE_ADDRESS);
#endif

  /* Get Page1 actual status */
  PageStatus1 = (*(vu16*)PAGE1_BASE_ADDRESS);

  /* Write or read operation */
  switch (Operation)
  {
    case WRITE_IN_VALID_PAGE:   /* ---- Write operation ---- */
      if (PageStatus1 == VALID_PAGE)
      {
        /* Page0 receiving data */
        if (PageStatus0 == RECEIVE_DATA)
        {
          return PAGE0;   /* Page0 valid */
        }
        else
        {
          return PAGE1;   /* Page1 valid */
        }
      }
      else if (PageStatus0 == VALID_PAGE)
      {
        /* Page1 receiving data */
        if (PageStatus1 == RECEIVE_DATA)
        {
          return PAGE1;      /* Page1 valid */
        }
        else
        {
          return PAGE0;      /* Page0 valid */
        }
      }
      else
      {
        return NO_VALID_PAGE;   /* No valid Page */
      }

    case READ_FROM_VALID_PAGE:  /* ---- Read operation ---- */
      if (PageStatus0 == VALID_PAGE)
      {
        return PAGE0;           /* Page0 valid */
      }
      else if (PageStatus1 == VALID_PAGE)
      {
        return PAGE1;           /* Page1 valid */
      }
      else
      {
        return NO_VALID_PAGE ;  /* No valid Page */
      }

    default:
      return PAGE0;       /* Page0 valid */
  }
}

/*******************************************************************************
* Function Name  : EE_VerifyPageFullWriteVariable
* Description    : Verify if active page is full and Writes variable in EEPROM.
* Input          : - VirtAddress: 16 bit virtual address of the variable
*                  - Data: 16 bit data to be written as variable value
* Output         : None
* Return         : - Success or error status:
*                      - FLASH_COMPLETE: on success
*                      - PAGE_FULL: if valid page is full
*                      - NO_VALID_PAGE: if no valid page was found
*                      - Flash error code: on write Flash error
*******************************************************************************/
u16 EE_VerifyPageFullWriteVariable(u16 VirtAddress, u16 Data)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  u16 ValidPage = PAGE0;
  u32 Address = 0x08010000, PageEndAddress = 0x080107FF;

  /* Get valid Page for write operation */
  ValidPage = EE_FindValidPage(WRITE_IN_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = (u32)(EEPROM_START_ADDRESS + (u32)(ValidPage * PAGE_SIZE));

  /* Get the valid Page end Address */
  PageEndAddress = (u32)((EEPROM_START_ADDRESS - 2) + (u32)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from beginning */
  while (Address < PageEndAddress)
  {
    /* Verify each time if Address and Address+2 contents are equal to Data and VirtAddress respectively */
    if (((*(vu16*)Address) == Data) && ((*(vu16*)(Address + 2)) == VirtAddress))
    {
      return FLASH_COMPLETE;
    }

    /* Verify if Address and Address+2 contents are 0xFFFFFFFF */
    if ((*(vu32*)Address) == 0xFFFFFFFF)
    {
      /* Set variable data */
      FlashStatus = FLASH_ProgramHalfWord(Address, Data);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Set variable virtual address */
      FlashStatus = FLASH_ProgramHalfWord(Address + 2, VirtAddress);
      /* Return program operation status */
      return FlashStatus;
    }
    else
    {
      /* Next address location */
      Address = Address + 4;
    }
  }

  /* Return PAGE_FULL in case the valid page is full */
  return PAGE_FULL;
}

/*******************************************************************************
* Function Name  : EE_ReadVariable
* Description    : Returns the last stored variable data, if found, which
*                  correspond to the passed virtual address
* Input          : - VirtAddress: Variable virtual address
*                  - Read_data: Global variable contains the read variable value
* Output         : None
* Return         : - Success or error status:
*                      - 0: if variable was found
*                      - 1: if the variable was not found
*                      - NO_VALID_PAGE: if no valid page was found.
*******************************************************************************/
u16 EE_ReadVariable(u16 VirtAddress, u16* ReadData)
{
  u16 ValidPage = PAGE0;
  u16 AddressValue = 0x5555, ReadStatus = 1;
  u32 Address = 0x08010000, PageStartAddress = 0x08010000;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  PageStartAddress = (u32)(EEPROM_START_ADDRESS + (u32)(ValidPage * PAGE_SIZE));

  /* Get the valid Page end Address */
  Address = (u32)((EEPROM_START_ADDRESS - 2) + (u32)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from end */
  while (Address > (PageStartAddress + 2))
  {
    /* Get the current location content to be compared with virtual address */
    AddressValue = (*(vu16*)Address);

    /* Compare the read address with the virtual address */
    if (AddressValue == VirtAddress)
    {
      /* Get content of Address-2 which is variable value */
      *ReadData = (*(vu16*)(Address - 2));
      /* In case variable value is read, reset ReadStatus flag */
      ReadStatus = 0;
      break;
    }
    else
    {
      /* Next address location */
      Address = Address - 4;
    }
  }

  /* Return ReadStatus value: (0: variable exist, 1: variable doesn't exist) */
  return ReadStatus;
}

/*******************************************************************************
* Function Name  : EE_PageTransfer
* Description    : Transfers last updated variable data from the full Page to
*                  an empty one.
* Input          : - VirtAddress: 16 bit virtual address of the variable
*                  - Data: 16 bit data to be written as variable value
* Output         : None
* Return         : - Success or error status:
*                      - FLASH_COMPLETE: on success,
*                      - PAGE_FULL: if valid page is full
*                      - NO_VALID_PAGE: if no valid page was found
*                      - Flash error code: on write Flash error
*******************************************************************************/
u16 EE_PageTransfer(u16 VirtAddress, u16 Data)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  u32 NewPageAddress = 0x080103FF, OldPageAddress = 0x08010000;
  u16 ValidPage = PAGE0, VarIdx = 0;
  u16 EepromStatus = 0, ReadStatus = 0;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  if (ValidPage == PAGE1)        /* Page1 valid */
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE0_BASE_ADDRESS;
    /* Old page address where variable will be taken from */
    OldPageAddress = PAGE1_BASE_ADDRESS;

  }
  else if (ValidPage == PAGE0)   /* Page0 valid */
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE1_BASE_ADDRESS;
    /* Old page address where variable will be taken from */
    OldPageAddress = PAGE0_BASE_ADDRESS;
  }
  else
  {
    return NO_VALID_PAGE;       /* No valid Page */
  }

  /* Set the new Page status to RECEIVE_DATA status */
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, RECEIVE_DATA);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Write the variable passed as parameter in the new active page */
  EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddress, Data);
  /* If program operation was failed, a Flash error code is returned */
  if (EepromStatus != FLASH_COMPLETE)
  {
    return EepromStatus;
  }
/*
  // Transfer process: transfer variables from old to the new active page
  for (VarIdx = 0; VarIdx < NumbOfVar; VarIdx++)
  {
    if (VirtAddVarTab[VarIdx] != VirtAddress)  // Check each variable except the one passed as parameter
    {
      // Read the other last variable updates
      ReadStatus = EE_ReadVariable(VirtAddVarTab[VarIdx], &DataVar);
      // In case variable corresponding to the virtual address was found
      if (ReadStatus != 0x1)
      {
        // Transfer the variable to the new active page
        EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddVarTab[VarIdx], DataVar);
        // If program operation was failed, a Flash error code is returned
        if (EepromStatus != FLASH_COMPLETE)
        {
          return EepromStatus;
        }
      }
    }
  }

  // Erase the old Page: Set old Page status to ERASED status
  FlashStatus = FLASH_ErasePage(OldPageAddress);
  // If erase operation was failed, a Flash error code is returned
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  // Set new Page status to VALID_PAGE status
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, VALID_PAGE);
  // If program operation was failed, a Flash error code is returned
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  // Return last operation flash status*/
  return FlashStatus;
}

/*******************************************************************************
* Function Name  : EE_WriteVariable
* Description    : Writes/updates variable data in EEPROM.
* Input          : - VirtAddress: Variable virtual address
*                  - Data: 16 bit data to be written
* Output         : None
* Return         : - Success or error status:
*                      - FLASH_COMPLETE: on success,
*                      - PAGE_FULL: if valid page is full
*                      - NO_VALID_PAGE: if no valid page was found
*                      - Flash error code: on write Flash error
*******************************************************************************/
u16 EE_WriteVariable(u16 VirtAddress, u16 Data)
{
  u16 Status = 0;

  /* Write the variable virtual address and value in the EEPROM */
  Status = EE_VerifyPageFullWriteVariable(VirtAddress, Data);
  /* In case the EEPROM active page is full */
  if (Status == PAGE_FULL)
  {
    /* Perform Page transfer */
    Status = EE_PageTransfer(VirtAddress, Data);
  }

  /* Return last operation status */
  return Status;
}

/*
 * find string name in keys, skip values
 * retained last found
 * PAGE LAYOUT [status:2][keyLen:2][key,,,][valueLen:2][value,....]
 */
uint8_t* findVar(char *name, uint8_t* start, uint8_t* end) {
	uint8_t* pb = (uint8_t*) 0;
	uint8_t* pc = start;
	uint32_t slen;
	while (pc < end) {
		slen = *pc;
		if (*pc == 0xFF)
			break;
		pc++;
		if (strncmp(name, (char*) pc, slen) == 0) {
			pb = pc + slen;
		}
		pc += slen;
		if (*pc == 0xFF)
			break;
		if ((pc + *pc) > end)
			break;
		pc += *pc;
	}
	return pb;
}

Erc Eeprom::write(uint8_t* pDst, uint8_t* pSrc, uint32_t size) {
	uint32_t i;
	uint8_t* pb = (uint8_t*) pSrc;
	uint16_t hw = 0;
	FLASH_Unlock(); //unlock flash writing
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_ErasePage(currentPage); //erase the entire page before you can write as I //mentioned
	for (i = 0; i < size; i++) {
		if (i & 0x1) {
			hw += pb[i];
			FLASH_ProgramHalfWord(currentPage + i * 2, hw);
		} else {
			hw = pb[i] << 8;
		}
	}
	FLASH_Lock(); //lock the flash for writing
	return E_OK;
}

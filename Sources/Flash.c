/*
 /*! @file
 *
 *  @brief Routines for erasing and writing to the Flash.
 *
 *  This contains the functions needed for accessing the internal Flash.
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-04-14
 */
/*!
**  @addtogroup Flash_module Flash module documentation
**  @{
*/
/* MODULE Flash */

#include "types.h"
#include "IO_Map.h"
#include "Flash.h"

// Address of the start of the Flash block we are using for data storage
#define FLASH_DATA_START 0x00080000LU
// Address of the end of the Flash block we are using for data storage
#define FLASH_DATA_END   0x00080007LU

/*! @brief Copies the flash memory into RAM buffer
 *
 *  @param buffer is the array to hold all the flash data as a temporary storage
 *  @return void
 */
static void FlashToRAM (uint8_t buffer[])
{
  int i;
  uint8_t *temp;
  temp = (uint8_t *) FLASH_DATA_START;               // declare it as the start of memory
  for (i = 0; i < 8; i++)
  {
    buffer[i] = *temp;                               // copy everything in the sector to the buffer
    temp++;                                          // increase address
  }
}

/*! @brief Write the phrase which is 8 bytes into flash memory
 *
 *  @param buffer is the array of data that will be stored in flash
 *  @return BOOL - TRUE if the phrase was successfully written
 */
static BOOL WritePhrase (const uint8_t buffer[])
{
  if (Flash_Erase())                                // erase the sector
  {
    for (;;)
    {
      if (FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK)        // FCCOB Availability check
      {
        while ((FTFE_FSTAT & FTFE_FSTAT_ACCERR_MASK) && (FTFE_FSTAT & FTFE_FSTAT_FPVIOL_MASK))   //Access error and protection violation check
        {
   	  FTFE_FSTAT = 0x30;   // Clear the old errors
   	}
        FTFE_FCCOB0 = 0x07;    // program phrase
        FTFE_FCCOB1 = 0x08;    // address
        FTFE_FCCOB2 = 0x00;
        FTFE_FCCOB3 = 0x00;
        FTFE_FCCOB7 = buffer[0];   // write to registers
        FTFE_FCCOB6 = buffer[1];
        FTFE_FCCOB5 = buffer[2];
        FTFE_FCCOB4 = buffer[3];
        FTFE_FCCOBB = buffer[4];
        FTFE_FCCOBA = buffer[5];
        FTFE_FCCOB9 = buffer[6];
        FTFE_FCCOB8 = buffer[7];
        FTFE_FSTAT = 0x80;
        return bTRUE;
      }
    }
  }

}


BOOL Flash_Init(void)
{
  SIM_SCGC3 |= SIM_SCGC3_NFC_MASK;                          // enable NFC
  return bTRUE;
}


BOOL Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  static uint32_t currentPointer = 0x00080000;             // variable memory address

    for (;;)
    {
      if (currentPointer >= FLASH_DATA_END)                // allocate the address to start if it reaches the end
      {
	currentPointer = FLASH_DATA_START;
      }
      if (currentPointer % size == 0)                      // if its divisible by 1,2,4
      {
        *variable = (uint8_t *) currentPointer;            // allocate to that address
      	currentPointer += size;                            // increase the size of pointer
      	return bTRUE;
      }
      else
      {
        currentPointer ++;                                 // if its not add 1 and loop again
      }
    }
}


BOOL Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{

  int change;                                       // variable for the change position in the buffer array
  uint32union_t newData;
  uint16union_t data1;
  uint16union_t data2;

  uint8_t buffer[8];
  FlashToRAM (buffer);                             // variable for the change position in the buffer array
  change = (uint32_t) address - FLASH_DATA_START;  // find the position that needs to be changed in the array
  newData.l = data;
  data1.l = newData.s.Lo;
  data2.l = newData.s.Hi;
  buffer[change] = data1.s.Lo;                     // change the value
  buffer[change+1] = data1.s.Hi;
  buffer[change+2] = data2.s.Lo;
  buffer[change+3] = data2.s.Hi;
  return WritePhrase(buffer);                      // write it to the sector
}


BOOL Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
  int change;                                        // variable for the change position in the buffer array
  uint16union_t newData;
  uint8_t buffer[8];
  FlashToRAM (buffer);                               // variable for the change position in the buffer array
  change = (uint32_t) address - FLASH_DATA_START;    // find the position that needs to be changed in the array
  newData.l = data;
  buffer[change] = newData.s.Lo;                     // change the value
  buffer[change+1] = newData.s.Hi;                   // write it to the sector
  return WritePhrase(buffer);
}


BOOL Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{

  int change;                                         // variable for the change position in the buffer array
  uint8_t buffer[8];
  FlashToRAM (buffer);                                // copy from Flash to buffer
  change = (uint32_t) address - FLASH_DATA_START;     // find the position that needs to be changed in the array
  buffer[change] = data;                              // change the value
  return WritePhrase(buffer);                         // write it to the sector
}



BOOL Flash_Erase(void)
{
  for (;;)
  {
    if (FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK)                 // FCCOB Availability check
    {
      while ((FTFE_FSTAT & FTFE_FSTAT_ACCERR_MASK) && (FTFE_FSTAT & FTFE_FSTAT_FPVIOL_MASK))  //Access error and protection violation check
      {
	FTFE_FSTAT = 0x30;   // Clear the old errors
      }
      FTFE_FCCOB0 = 0x09;    // erase sector
      FTFE_FCCOB1 = 0x08;    // address
      FTFE_FCCOB2 = 0x00;
      FTFE_FCCOB3 = 0x00;
      FTFE_FSTAT = 0x80;     // clear CCIF to launch the command
      return bTRUE;
    }
  }

}
/* END Flash */
/*!
 * @}
*/




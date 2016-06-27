/*! @file
 *
 *  @brief Flash module: Routines for erasing and writing to the Flash.
 *
 *  This module contains the functions needed for accessing the internal Flash.
 *
 *  @author Liang Wang
 *  @date 2016-06-10
 */
/*!
**  @addtogroup Flash_module Flash module documentation
**  @{
*/
/* MODULE Flash */
#include "Flash.h"

#define FIRST_ADDRESS                0x00080000            /*!<  a FIRST_ADDRESS and give it value */
#define LAST_ADDRESS                 0x00080008            /*!<  a LAST_ADDRESS and give it value */
#define LAST_AVALIABLE_SIZE2_ADDRESS 0x80006               /*!<  a LAST_AVALIABLE_SIZE2_ADDRESS and give it value */
#define LAST_AVALIABLE_SIZE4_ADDRESS 0x80004               /*!<  LAST_AVALIABLE_SIZE4_ADDRESS and give it value */
static int ptrAdd= FIRST_ADDRESS;                          /*!<  a local variable */

BOOL Flash_Init(void)
{
  /*enable the flash memory gate*/
  SIM_SCGC3 |= SIM_SCGC3_NFC_MASK;
  return bTRUE;
}


BOOL Flash_AllocateVar(volatile void **variable, const uint8_t size)
{
  /* align flash address. */
  if(ptrAdd>LAST_ADDRESS)
    ptrAdd = FIRST_ADDRESS;
  if(size == 2)
  {
    if(ptrAdd>LAST_AVALIABLE_SIZE2_ADDRESS)
      ptrAdd = FIRST_ADDRESS;
    ptrAdd +=(ptrAdd%2);
  }
  if(size == 4 && ptrAdd%4!=0)
  {
    if(ptrAdd >LAST_AVALIABLE_SIZE4_ADDRESS)
      ptrAdd = FIRST_ADDRESS;
    ptrAdd =ptrAdd+ 4 - (ptrAdd%4);
  }
  /*give the data on ptrAdd to the position of the pointer point to */
  *variable= (int *)ptrAdd;
  /*allocate address to the next data, if size is 2, and the first address is 0x80000,the the next address is 0x80002*/
  ptrAdd +=size;
  return bTRUE;
}


/*! @brief copy the data that we have changed  in ram buffer back into flash.
 *
 *  @return BOOL - TRUE if data is written to flash successfully.
 */
BOOL Flash_RamToFlash(uint8_t ram[])
{
  uint8_t CCIFflag, ACCERRflag, FPVIOLflag;  /*!< CCIFflag, ACCERRflag, FPVIOLflag local variables*/

  /*!after the old flash memory erased, copy the new one into it*/
  if(Flash_Erase())
  {
    for(;;)
    {
      /* check if CCIF is 1,use mask to make it equal to x first and others 0*/
      CCIFflag = FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK;
      /*when x=1,which means CCIF is 1*/
      if(CCIFflag == FTFE_FSTAT_CCIF_MASK)
      {
        /*check if ACCERR and FPVIOL are both equal to 1,use mask to make them equal to x first and others 0*/
        ACCERRflag = FTFE_FSTAT &FTFE_FSTAT_ACCERR_MASK;
        FPVIOLflag = FTFE_FSTAT &FTFE_FSTAT_FPVIOL_MASK;
        /*when x=1,which means they are 1*/
        if(ACCERRflag == FTFE_FSTAT_ACCERR_MASK && FPVIOLflag == FTFE_FSTAT_FPVIOL_MASK)
	{
          /*clear the old errors and write 0x30 to FSTAT register*/
	  FTFE_FSTAT = 0x30;
	}
        /*write to the FCCOB register to load the required command parameter
        do write data follow the pin name of FCCOB*/
	FTFE_FCCOB0 = 0x07;
	/*follow the first address0x080000*/
	FTFE_FCCOB1 = 0x08;
	FTFE_FCCOB2 = 0x00;
	FTFE_FCCOB3 = 0x00;
	/*copy the data changed in ram buffer back into flash, follow THE typedef structure FTFE_MemMap inMK70F12,h*/
	FTFE_FCCOB7 = ram[0];
	FTFE_FCCOB6 = ram[1];
	FTFE_FCCOB5 = ram[2];
	FTFE_FCCOB4 = ram[3];
	FTFE_FCCOBB = ram[4];
	FTFE_FCCOBA = ram[5];
	FTFE_FCCOB9 = ram[6];
	FTFE_FCCOB8 = ram[7];
	/*clear the CCIF to launch the command write 0x80 to FSTAT register*/
	FTFE_FSTAT = 0x80;
	return bTRUE;
      }
    }
  }
}

BOOL Flash_Write32(uint32_t volatile * const address, const uint32_t data)
{
  TFloat data32;       /* a 32 bits data*/

  data32.d = data;     /*Separate 32 bit four 8 bit number*/

  int i = 0;           /*i to make a loop*/

  int startWriteAdd;   /*a ram address for each eight bit*/

  uint8_t ram[8];      /*each ram has eight bits*/

  /*!copy the data from flash into a ram buffer by each byte*/
  for(i;i<=7;i++)
  {
    ram[i] = *((uint32_t *)(FIRST_ADDRESS+i));
  }
  /*!get the address than change the data of that address in the buffer*/
  startWriteAdd = (uint32_t)address;
  /*!change the data each byte in ram buffer*/
  ram[startWriteAdd] =data32.dParts.dHi.s.Hi;
  ram[startWriteAdd+1] =data32.dParts.dHi.s.Lo;
  ram[startWriteAdd+2] =data32.dParts.dLo.s.Hi;
  ram[startWriteAdd+3] =data32.dParts.dLo.s.Lo;
  /*!copy the changed data from ram buffer back to flash*/
  return Flash_RamToFlash(ram);
}


BOOL Flash_Write16(uint16_t volatile * const address, const uint16_t data)
{
  uint16union_t data16;     /* a 16 bits data*/

  data16.l = data;          /*Separate 16 bits to two 8 bit number*/

  int i = 0;                /* i to make a loop*/

  int startWriteAdd;        /* a ram address for each eight bit*/

  uint8_t ram[8];           /*each ram has eight bits*/

  for(i;i<=7;i++)
  {
    /*copy the data from flash into a ram buffer by each byte*/
    ram[i] = *((uint32_t *)(FIRST_ADDRESS+i));
  }
  /* change the data in ram buffer by finding the address offset*/
  startWriteAdd = (uint32_t)address- FIRST_ADDRESS;
  /*change the data by each byte in ram buffer*/
  ram[startWriteAdd] =data16.s.Lo;
  ram[startWriteAdd+1] =data16.s.Hi;
  /*copy the changed data from ram buffer back to flash*/
  return Flash_RamToFlash(ram);
}


BOOL Flash_Write8(uint8_t volatile * const address, const uint8_t data)
{
  int i = 0;               /* i to make a loop*/

  int startWriteAdd;       /* a ram address for each eight bit*/

  uint8_t ram[8];          /*each ram has eight bits*/

  /*copy the data from flash into a ram buffer by each byte*/
  for(i;i<=7;i++)
  {
    ram[i] = *((uint32_t *)(FIRST_ADDRESS+i));
  }
  /*get the address that we want to change the data*/
  startWriteAdd=(uint32_t)address;
  /*do change the data in the right address in ram buffer*/
  ram[startWriteAdd] =data;
  /*copy the changed data from ram buffer back to flash*/
  return Flash_RamToFlash(ram);
}


BOOL Flash_Erase(void)
{
  uint8_t CCIFflag, ACCERRflag, FPVIOLflag;   /* CCIFflag, ACCERRflag, FPVIOLflag local variables*/

  for(;;)
  {
    /* check if CCIF is 1,use mask to make it equal to x first and others 0*/
    CCIFflag = FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK;
    /*when x=1,which means CCIF is 1*/
    if(CCIFflag == FTFE_FSTAT_CCIF_MASK)
    {
      /*check if ACCERR and FPVIOL are both equal to 1,use mask to make them equal to x first and others 0*/
      ACCERRflag = FTFE_FSTAT &FTFE_FSTAT_ACCERR_MASK;
      FPVIOLflag = FTFE_FSTAT &FTFE_FSTAT_FPVIOL_MASK;
      /*when x=1,which means they are 1 */
      if(ACCERRflag == FTFE_FSTAT_ACCERR_MASK && FPVIOLflag == FTFE_FSTAT_FPVIOL_MASK)
      {
	/*clear the old errors and write 0x30 to FSTAT register*/
	FTFE_FSTAT = 0x30;
      }
      /*write to the FCCOB register to load the required command parameter
      do erase data follow the pin name of FCCOB*/
      FTFE_FCCOB0 = 0x09;
      /*follow the first address0x080000*/
      FTFE_FCCOB1 = 0x08;
      FTFE_FCCOB2 = 0x00;
      FTFE_FCCOB3 = 0x00;
      /*clear the CCIF to launch the command write 0x80 to FSTAT register*/
      FTFE_FSTAT = 0x80;
      return bTRUE;
    }
  }
  //return bFALSE;
}
/* END Flash */
/*!
** @}
*/



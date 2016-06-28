/* ###################################################################
**     Filename    : main.c
**     Project     : Lab3
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-09-19, 13:27, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.0
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */

// CPU module - contains low level hardware initialization routines
#include "Cpu.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "packet.h"
#include "Flash.h"
#include "LEDs.h"
#include "RTC.h"
#include "PIT.h"
#include "FTM.h"
#include "accel.h"
#include "I2C.h"
#include "median.h"
#include "TSI.h"
#include "RNG.h"
#include "SW.h"
#define BAUDRATE 115200                               /*!< baud rate a value 115200*/
#define TOWER_STARTUP_CMD 0x04                        /*!<0x04 is TOWER_STARTUP_CMD*/
#define TOWER_GETVERSION_CMD 0x09                     /*!<0x09 is TOWER_GETVERSION_CMD*/
#define TOWER_NUMBER_CMD 0x0B                         /*!<0x0B is TOWER_NUMBER_CMD*/
#define TOWER_PROGRAMBYTE_CMD 0x07                    /*!<0x07 is TOWER_PROGRAMBYTE_CMD*/
#define TOWER_READBYTE_CMD 0x08                       /*!<0x08 is TOWER_READBYTE_CMD*/
#define TOWER_ACCELMODE_CMD 0x0A
#define TOWER_TOWERMODE_CMD 0x0D                      /*!<0x0D is TOWER_TOWERMODE_CMD*/
#define TOWER_TIME_CMD 0x0C                           /*!<0x0C is TOWER_TIME_CMD*/
#define TOWER_ACCEL_CMD 0x10
#define TOWER_GAME_CMD 0x0E
#define CR 0x0d                                       /*!<0x0d is CR*/
#define MAJOR_VERSION_NUMBER 0x01                     /*!<0x01 is MAJOR_VERSION_NUMBER*/
#define MINOR_VERSION_NUMBER 0x00                     /*!<0x00 is MINOR_VERSION_NUMBER*/
#define GET_TOWER_NUMBER 0x01                         /*!<0x01 is  GET_TOWER_NUMBER*/
#define SET_TOWER_NUMBER 0x02                         /*!<0x02 is SET_TOWER_NUMBER*/
#define ACK_MASK 0x80                                 /*!<0x80 is ACK_MASK*/
#define FIRST_ADDRESS 0x80000                         /*!<0x80000 is  FIRST_ADDRESS*/
#define STUDENT_NUMBER 6928                           /*!<6928 is STUDENT_NUMBER*/
#define UNPROGRAMED_NUMBER 0xFFFF                     /*!<0xFFFF is UNPROGRAMED_NUMBER*/
#define TOWER_INIT_MODE 0x01                          /*!<0x01 is TOWER_INIT_MODE*/
#define PERIOD 500000000                              /*!<5000000000 is PERIOD*/

static uint16_t *towerNb;                             /*!< pointer to tower number. */

static uint16_t *towerMd;                             /*!< pointer to tower towermode. */

static uint8_t h,m,s;                                 /*!< hours and seconds */
static uint8_t score;

static uint8_t accMode = 0;                            /*!< signal mode select */
static TFTMChannel aFTMChannel;		              /*!< pre seting aFTMChannel */

TPacket Packet;

static TI2CModule aI2CModule;
TAccelSetup accelSetup;
/*! @brief asynchronous read.
 *
 */
void Poll_Read(void)
{
  int i;
  static uint8_t data[3];//,datap[3];
  Accel_ReadXYZ(data);
    Packet_Put(TOWER_ACCEL_CMD, data[0], data[1], data[2]);

}
/*! @brief callback function to toggle green led.
 *
 */
void PIT_Callback()
{
  if (Mode() == 0)
    LEDs_Toggle(LED_GREEN);
}

/*! @brief callback function to turn off blue led.
 *
 */
void FTM0_Callback()
{
  if (Mode() == 0)
    LEDs_Off(LED_BLUE);
}


/*! @brief callback function to toggle yellow led.
 * get the time from RTC.
 * sends the current time to the PC.
 */
void RTC_Callback()
{
  RTC_Get(&h, &m, &s);
  if (Mode() == 0)
    LEDs_Toggle(LED_YELLOW);

  Packet_Put(TOWER_TIME_CMD, h, m,s);
}

void I2C_Callback()
{
  static int i = 0;
  uint8_t x,y,z;
  static uint8_t temp1[3] = {0,0,0},temp2[3] = {0,0,0},temp3[3] = {0,0,0};

  temp1[i] = data[i];
  i++;
  x = Median_Filter3(temp1[0], temp2[0], temp3[0]);
  y = Median_Filter3(temp1[1], temp2[1], temp3[1]);
  z = Median_Filter3(temp1[2], temp2[2], temp3[2]);
  if(i == 3)
  {
    for(int l = 0;l < 3;l++)
    {
      temp3[l] = temp2[l];
      temp2[l] = temp1[l];
    }
    i = 0;
  }
  Packet_Put(TOWER_ACCEL_CMD,x,y,z);
}

void AccCallback(void)
{
  I2C_IntRead(0x01, data, 3);
}
/*! @brief To set up the tower, we have to call packet initialize
 *
 *  @return BOOL - TRUE if the tower was setup successfully.
 */
BOOL Tower_Setup(void)
{
  /*!by checking the CPU.h, we can find that the way to Initial value of the bus clock frequency in Hz is  CPU_BUS_CLK_HZ*/
  aFTMChannel.channelNb = 0;			/*!choose channel 0*/
  aFTMChannel.timerFunction = TIMER_FUNCTION_OUTPUT_COMPARE; /*!choose timer function as OUTPUT_COMPARE.*/
  aFTMChannel.userFunction = &FTM0_Callback;   /*!choose user function as FTM0_Callback.*/
  aI2CModule.baudRate = 100000;                                          ////////////////////
  aI2CModule.primarySlaveAddress = 0x1D;
  aI2CModule.readCompleteCallbackFunction = I2C_Callback;             ///////////////&
  //accelSetup.dataReadyCallbackFunction = AccCallback;                  ////////////////&
  return Packet_Init(BAUDRATE, CPU_BUS_CLK_HZ) &&
	 Flash_Init() &&
	 PIT_Init(CPU_BUS_CLK_HZ, &PIT_Callback, NULL)&&
	 RTC_Init(&RTC_Callback, NULL)&&
	 FTM_Init()&&
	 LEDs_Init() &&
	 I2C_Init(&aI2CModule,CPU_BUS_CLK_HZ) &&                       /////////////////////////////
	 Accel_Init(&accelSetup) &&
	 RNG_Init() &&
	 SW_Init() &&
	 TSI_Init();
}


/*! @brief initialize the tower.
 *  when start up PC can get four packets from tower automatically
 *  @return BOOL - TRUE if the tower was initialize successfully.
 */
BOOL Tower_Init(void)
{
  uint16union_t towerNumber;        /*! a 16 bit towerNumber */
  uint16union_t towerMode;          /*! a 16 bit towerMode */
  towerNumber.l = *towerNb;
  /*!initialize  the tower number*/
  if (towerNumber.l == UNPROGRAMED_NUMBER)
    towerNumber.l = STUDENT_NUMBER;

  towerMode.l = *towerMd;
  /*!initialize the tower mode*/
  if (towerMode.l == UNPROGRAMED_NUMBER)
    towerMode.l = TOWER_INIT_MODE;

  FTM_Set(&aFTMChannel);
  TSI_SelfCalibration();
  /*!get four packets*/
  Packet_Put(TOWER_STARTUP_CMD, 0, 0, 0);
  Packet_Put(TOWER_GETVERSION_CMD,'v', MAJOR_VERSION_NUMBER, MINOR_VERSION_NUMBER);
  Packet_Put(TOWER_NUMBER_CMD, GET_TOWER_NUMBER, towerNumber.s.Lo, towerNumber.s.Hi);
  Packet_Put(TOWER_TOWERMODE_CMD,1,towerMode.s.Lo, towerMode.s.Hi);
  Packet_Put(TOWER_ACCELMODE_CMD, 1, accMode, 0);
  return bTRUE;
}


/*! @brief handle the start_up_packet.
 *  when tower is start up, it returns four packets.
 *  @return BOOL -  Tower_Init(),if the tower is start up.
 */
BOOL Handle_Startup_Packet(void)
{
  if (Packet_Parameter1 == 0 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0)
  {
    /*!this command is carried out*/
    return Tower_Init();
  }
}

/*! @brief handle the GetVersion_Packet.
 *  when receive get version packet, return the version number packet.
 *  @return BOOL - Packet_Put() to get version.
 */
BOOL Handle_GetVersion_Packet(void)
{
  if (Packet_Parameter1 == 'v' && Packet_Parameter2 == 'x' && Packet_Parameter3 == CR)
  {
    /*!tower will send back the tower version packet to PC*/
    return Packet_Put(TOWER_GETVERSION_CMD,'v', MAJOR_VERSION_NUMBER, MINOR_VERSION_NUMBER);
  }
}


/*! @brief handle the GetNumber_Packet.
 *  when receive get number packet, return the number packet.
 *  @return BOOL - Packet_Put() to GetNumber.
 */
BOOL Handle_GetNumber_Packet(void)
{
  /*!when we choose get*/
  if (Packet_Parameter1 == GET_TOWER_NUMBER && Packet_Parameter2 == 0 && Packet_Parameter3 == 0)
  {
    /*!get the tower number we have wrote into the flash*/
    uint16union_t towerNumber;
    towerNumber.l = *towerNb;
    /*!if we have not set the number, get the initial tower number, which is our student number*/
    if (towerNumber.l == UNPROGRAMED_NUMBER)
      towerNumber.l = STUDENT_NUMBER;
    /*!PC receive the tower number packet*/
    return Packet_Put(TOWER_NUMBER_CMD, GET_TOWER_NUMBER, towerNumber.s.Lo, towerNumber.s.Hi);
  }
  /*!choose set number,we can set our student number into parameter23*/
  if (Packet_Parameter1 == SET_TOWER_NUMBER)
  {
    /*!set the tower number by changing the value of parameter2 and parameter3 this have to write into flash*/
    Flash_Write16((towerNb), Packet_Parameter23);
    /*!PC receive the tower number we set*/
    return Packet_Put(TOWER_NUMBER_CMD, SET_TOWER_NUMBER, Packet_Parameter2,Packet_Parameter3);
  }
}

/*! @brief handle the ProgramByte_Packet.
 *  when receive program byte packet, put the data into flash memory.
 *  @return BOOL - Flash_Erase() if address offset is 0x08 to erase; Flash_Write8 if address offset is less than 0x08 to write.
 */
BOOL Handle_ProgramByte_Packet(void)
{
  /*!follow the table of packets transmitted from PC to Tower,when choose program byte, parameter2 should be 0*/
  if (Packet_Parameter2 == 0)
  {
    /*!when address offset is 0x08, it do erase the flash sector*/
    if (Packet_Parameter1 == 8)
      return Flash_Erase();
    /*!when address offset is less than 0x08, it can write*/
    if (Packet_Parameter1 < 8)
    {
      /*!find address by the address offset, and write data in parameter 3 in flash*/
      uint32_t address = Packet_Parameter1;
      /*!write 8 bits number into flash*/
      return Flash_Write8((uint8_t*)address, Packet_Parameter3);
    }
  }
}


/*! @brief handle the ReadByte_Packet.
 *  when receive read byte packet, read data from flash.
 *  @return BOOL - Packet_Put() to ReadByte.
 */
BOOL Handle_ReadByte_Packet(void)
{
  /*!follow the table of packets transmitted from PC to Tower,when choose program byte, parameter23 should be 0*/
  if (Packet_Parameter2 == 0 && Packet_Parameter3 == 0)
    /*!tower will send back the packet to PC that read from flash,and parameter1 is the address offset*/
    return Packet_Put(TOWER_READBYTE_CMD, Packet_Parameter1, 0, *(uint32_t*)(FIRST_ADDRESS+Packet_Parameter1));
}

/*! @brief handle the TowerMode_Packet.
 *  when receive tower mode packet, return the tower mode packet.
 *  @return BOOL - Packet_Put() to get the tower mode.
 */
BOOL Handle_TowerMode_Packet(void)
{
  /*!parameter1 is 1 means we choose get tower mode,when we choose get, parameter23 should be 0*/
  if (Packet_Parameter1 == 1 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0)
  {
    /*!get the tower mode we have wrote into the flash*/
    uint16union_t towerMode;
    towerMode.l = *towerMd;
    /*!if we have not set the number, get the initial tower number, which is 1*/
    if(towerMode.l == UNPROGRAMED_NUMBER)
      towerMode.l = TOWER_INIT_MODE;
    /*!PC receive the tower mode packet*/
    return Packet_Put(TOWER_TOWERMODE_CMD, 1, towerMode.s.Lo, towerMode.s.Hi);
  }
  /*!choose set number,we can set any number into parameter23*/
  if (Packet_Parameter1 == 2)
  {
    /*!set the tower mode by changing the value of parameter2 and parameter3 this have to write into flash*/
    Flash_Write16(towerMd, Packet_Parameter23);
    /*!PC receive the tower mode number we set before*/
    return Packet_Put(TOWER_TOWERMODE_CMD,2,Packet_Parameter2, Packet_Parameter3);
  }
}


/*! @brief handle the TowerTime_Packet.
 *  set the time for RTC
 *  @return BOOL - Packet_Put() to get time.
 */
BOOL Handle_TowerTime_Packet(void)
{
  if (Packet_Parameter1 >= 0 && Packet_Parameter1 <= 23 && Packet_Parameter2 >= 0 && Packet_Parameter2 <= 59 && Packet_Parameter3 >= 0 &&Packet_Parameter3 <= 59)
  {
    RTC_IER &= ~RTC_IER_TSIE_MASK;    /*!Seconds interrupt disable*/
    RTC_Set(Packet_Parameter1,Packet_Parameter2,Packet_Parameter3);
    RTC_IER |= RTC_IER_TSIE_MASK;    /*!Seconds interrupt enable*/
    return Packet_Put(TOWER_TIME_CMD, Packet_Parameter1, Packet_Parameter2,Packet_Parameter3);
  }
}
BOOL Handle_AccelMode_Packet(void)
{
  if (Packet_Parameter1 == 1 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0)
    return Packet_Put(TOWER_ACCELMODE_CMD, Packet_Parameter1, accMode,Packet_Parameter3);

  if (Packet_Parameter1 = 2 && Packet_Parameter3 == 0)
  {
    if (Packet_Parameter2 == 0)
    {
      accMode == 0;
      Accel_SetMode(ACCEL_POLL);
      return bTRUE;
    }
    if (Packet_Parameter2 == 1)
    {
      accMode == 1;
      Accel_SetMode(ACCEL_INT);
      return bTRUE;
    }
  }
}

BOOL Handle_Game_Packet(void)
{
  if (Packet_Parameter1 == 0 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0)
  {
    return Packet_Put(TOWER_GAME_CMD, Packet_Parameter1, score,Packet_Parameter3);
  }
}

/*! @brief Sets up TSI game .
 *
 */
void Game(void)
{
  TLED color[4] = {LED_BLUE, LED_GREEN, LED_YELLOW, LED_ORANGE};
  int sequence[32];
  int currentColor,i,n,s,j,k;
  BOOL keepgoing;
  FTM0_CnSC(0) &= ~FTM_CnSC_CHIE_MASK;
  FTM0_CnSC(0) &= ~FTM_CnSC_CHF_MASK;
  for (i = 0;i < 4;i++)
    LEDs_Off(color[i]);

  for (;;)
  {
   /* if(readStart()&&!keepgoing)
    {
      for(j=0;j<32;j++)
      {
        sequence[i]=4;
      }
    score=s;
    s=0;
    for(k=0;k<6;k++)
    {
      for(i=0;i<4;i++)
	LEDs_On(color[i]);
      FTM_StartTimer(&aFTMChannel,8138);
      while((FTM0_CnSC(0)& FTM_CnSC_CHF_MASK)==0) {}
      FTM0_CnSC(0)&= ~FTM_CnSC_CHF_MASK;
      for(i=0;i<4;i++)
	LEDs_Off(color[i]);
    }
    }
    */
    keepgoing =bFALSE;
    currentColor = (RNG_Number() % 4);            //generate random for color
    LEDs_On(color[currentColor]);                 //turn on led
    FTM_StartTimer(&aFTMChannel,24414);           //flash for 1 second
    while((FTM0_CnSC(0)& FTM_CnSC_CHF_MASK)==0) {}
    FTM0_CnSC(0)&= ~FTM_CnSC_CHF_MASK;
    LEDs_Off(color[currentColor]);
    //for(n =1;n<10000000; n++)
   // {
   //   __asm("nop");
   // }
    if (TSI_ReadStart())
    {
      sequence[s] = currentColor;
      s++;
      keepgoing = bTRUE;
    }
    if(Mode()!= 2)
    {
      FTM0_CnSC(0) |= FTM_CnSC_CHIE_MASK;
      FTM0_CnSC(0) |= FTM_CnSC_CHF_MASK;
      return;
    }
  }
}


/*! @brief check touch pad.
 *
 */
void CheckTouchPad(void)
{
  TLED color[4] = {LED_BLUE, LED_GREEN, LED_YELLOW, LED_ORANGE};
  int i;
  if (TSI_ReadChk())
  {
    TSI_WriteChk(bFALSE);
    FTM0_CnSC(0) &= ~FTM_CnSC_CHIE_MASK;
    FTM0_CnSC(0) &= ~FTM_CnSC_CHF_MASK;
    for (i = 0;i < 5;i++)
    {
      FTM_StartTimer(&aFTMChannel,24414);
      while((FTM0_CnSC(0) & FTM_CnSC_CHF_MASK) == 0) {}
      FTM0_CnSC(0) &= ~FTM_CnSC_CHF_MASK;
      if (TSI_ReadChk())
	return;
    }
    WriteMode(0);
    FTM0_CnSC(0) |= FTM_CnSC_CHIE_MASK;
    FTM0_CnSC(0) |= FTM_CnSC_CHF_MASK;
    for (i = 0;i < 4;i++)
      LEDs_Off(color[i]);
    LEDs_On(color[3]);
  }
}

/*! @brief main loop for receive and handle packet.
 *
 */
void Tower_HandlePackets(void)
{
  /*!used to check the bit7 of the Packet_Command is 0/1*/
  uint8 ACK = 0;
  /*!check if the packet from PC Tower is right can used, if not, bFALSE*/
  BOOL Carried_Out = bFALSE;

  if (Packet_Get())
  {
    if (Mode() == 0)
      LEDs_On(LED_BLUE);
    //aFTMChannel.initialCount = 0; 		/*!assign the tiemr initial value.*/
    FTM_StartTimer(&aFTMChannel,24414);
    /*!first block the first bit of the Packet_Command for the Packet Acknowledgment,XXXX XXXX& 1000 0000 to get ACK = X000 0000*/
    ACK = (Packet_Command & ACK_MASK);
    /*!Packet_Command = XXXX XXXX& 0111 1111 = 0XXX XXXX, use this way to block the bit7 of the command*/
    Packet_Command &= ~ACK_MASK;
    switch (Packet_Command)
    {
    /*!follow the table of packets transmitted from PC to Tower, if command is 0x04,data stream is 0 0 0,it do tower start up, this means tower will turn back four packets*/
    case (TOWER_STARTUP_CMD):
      Carried_Out = Handle_Startup_Packet();
      break;
    /*!when choose Get version*/
    case (TOWER_GETVERSION_CMD):
      Carried_Out = Handle_GetVersion_Packet();
      break;
    /*!when choose tower number*/
    case (TOWER_NUMBER_CMD):
      Carried_Out = Handle_GetNumber_Packet();
      break;
      /*!when choose write data into flash*/
    case (TOWER_PROGRAMBYTE_CMD):
      Carried_Out = Handle_ProgramByte_Packet();
      break;
      /*!when choose read data from flash*/
    case (TOWER_READBYTE_CMD):
      Carried_Out = Handle_ReadByte_Packet();
      break;
      /*!when choose get tower mode*/
    case (TOWER_TOWERMODE_CMD):
      Carried_Out = Handle_TowerMode_Packet();
      break;
    case (TOWER_TIME_CMD):
      Carried_Out = Handle_TowerTime_Packet();
      break;
    case (TOWER_ACCELMODE_CMD):
      Carried_Out = Handle_AccelMode_Packet();
      break;
    case (TOWER_GAME_CMD):
      Carried_Out = Handle_Game_Packet();
      break;
    default:
      break;
    }
  }
    /*!X000 0000=1000 0000 (means if bit7 is 1, the Packet_Command is 0x8X) and the packet from tower to PC is right ones*/
    if ((ACK == ACK_MASK) && Carried_Out)
      /*!if so, packets transmitted from PC to Tower will not only three before, another fourth packet will be transmitted with a packet command 0x8X*/
      Packet_Put(Packet_Command|ACK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    /*!X000 0000=1000 0000 (means if bit7 is 1, the Packet_Command is 0x8X) and the packet from tower to PC is not right ones*/
    if ((ACK == ACK_MASK) && !Carried_Out)
      /*!if so, the packet command of the packet transmitted from PC to Tower is 0x0X*/
      Packet_Put(Packet_Command&~ACK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
}


/*lint -save  -e970 Disable MISRA rule (6.3) checking. */

/*! @brief main function.
 *
 */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/

  PE_low_level_init();

  /*** End of Processor Expert internal initialization.                    ***/
  EnterCritical();
  Tower_Setup();

  if (Tower_Setup() && (Mode() == 0))    //if successful initial in default mode
    LEDs_On(LED_ORANGE);

  PIT_Set(PERIOD ,1);
  PIT_Enable(1);

  /*Allocate the address for tower number.*/
  Flash_AllocateVar((volatile void **)&towerNb, sizeof(*towerNb));
  /*Allocate the address for tower mode.*/
  Flash_AllocateVar((volatile void **)&towerMd, sizeof(*towerMd));
  Tower_Init();
  ExitCritical();
  /* Write your code here */
  for (;;)
  {
    if (Mode() == 1)
      CheckTouchPad();  //mode 1 touch toggle

    if (Mode() == 2)    //mode 2 game
      Game();

    Tower_HandlePackets();
  }

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/

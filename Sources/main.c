/* ###################################################################
**     Filename    : main.c
**     Project     : Lab4
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
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
** @version 4.0
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
#include "OS.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "packet.h"
#include "Flash.h"
#include "LEDs.h"
#include "PIT.h"
#include "timer.h"
#include "RTC.h"
#include "accel.h"
#include "I2C.h"
#include "median.h"


#define STARTUP 0x04
#define PROGRAM 0x07
#define READ 0x08
#define GETVERSION 0x09
#define NUMBER 0x0B
#define MODE 0x0D
#define TIME 0x0C
#define PMODE 0x0A
#define ACCEL 0x10
#define GAME 0x0E
#define DEFAULTNB 0x038a
#define DEFAULTMD 0x0001
#define THREAD_STACK_SIZE 1000
const uint32_t BAUD_RATE = 115200;
volatile uint16union_t *NvTowerNb;
volatile uint16union_t *NvTowerMd;
int pMode;
static uint8_t* data;
static TTimer timer;
static TAccelSetup accel;
static TI2CModule module;
static uint8_t values[3];

static OS_ECB *InitSemaphore;
static OS_ECB *PacketSemaphore;
static OS_ECB *AccelReadSemaphore;

static uint32_t HandlePacketThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));   // The stack for the handle packet thread
static uint32_t TowerInitThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));      // The stack for the initialization thread
static uint32_t PITCallbackThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));    // The stack for the PIT callback thread
static uint32_t RTCCallbackThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));    // The stack for the RTC callback thread
static uint32_t FTMCallbackThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));    // The stack for the FTM callback thread
static uint32_t AccelCallbackThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));  // The stack for the Accelerometer thread
static uint32_t AccelReadThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));      // The stack for the Accelerometer data ready thread
static uint32_t I2CCallbackThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));    // The stack for the I2C callback thread



/*! @brief Read Accelerometer values
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @return void
 */
static void AccelCallbackThread (void *pData)
{
  for(;;)
  {
    (void)OS_SemaphoreWait(AccelSemaphore, 0);
    I2C_IntRead (0x01, values, 3);
  }

}



/*! @brief Handles the interrupt for I2C
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @return void
 */
static void I2CCallbackThread (void *pData)
{
  for(;;)
  {
    (void)OS_SemaphoreWait(I2CSemaphore, 0);
    uint8_t data[3];
    if (!(I2C0_C1 & I2C_C1_TX_MASK))
    {
      Accel_ReadXYZ(data);                              // Collect accelerometer data
      Packet_Put (ACCEL, data[0], data[1], data[2]);    // Send accelerometer data

    }
  }
}




/*! @brief Turns the blue led off for timer interrupt
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @return void
 */

static void FTMCallbackThread (void *pData)
{
  for(;;)
  {
    (void)OS_SemaphoreWait(FTMSemaphore, 0);
    LEDs_Off(LED_BLUE);

  }
}

/*! @brief Sends hours, minutes, seconds and toggles yellow led for RTC interrupt
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @return void
 */
static void RTCCallbackThread (void *pData)
{
  for(;;)
  {
    (void)OS_SemaphoreWait(RTCSemaphore, 0);
    uint8_t  hours, minutes, seconds;
    RTC_Get (&hours, &minutes, &seconds);         // get time from RTC

    Packet_Put(TIME, hours, minutes, seconds);    // send the current time to PC

    LEDs_Toggle(LED_YELLOW);

  }
}

/*! @brief Toggle the green led for PIT interrupt
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @return void
 */
static void PITCallbackThread (void *pData)
{

  for(;;)
  {
    (void)OS_SemaphoreWait(PITSemaphore, 0);
    if (pMode == 0)
    {
      OS_SemaphoreSignal(AccelReadSemaphore);
    }


  }

}



/*! @brief Sends the first four packets from Tower to PC
 *
 *  @return void
 */
void StartupPacket(void)
{

  Packet_Put(STARTUP, 0, 0, 0);
  Packet_Put(GETVERSION, 'v', 1, 0);
  Packet_Put(NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
  Packet_Put(MODE, 1, NvTowerMd->s.Lo, NvTowerMd->s.Hi);
  Packet_Put(PMODE, 1, pMode, 0);

}

/*! @brief Initialize the LEDs, Flash, PIT, RTC, FTM and UART module
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @return void
 */
static void TowerInitThread(void *pData)
{
  for(;;)
  {
    (void)OS_SemaphoreWait(InitSemaphore, 0);
    LEDs_Init();                                                                 // Initialize LEDs

    if (Flash_Init() && Packet_Init (BAUD_RATE, CPU_BUS_CLK_HZ))
      LEDs_On(LED_ORANGE);                                                       // If flash and UART are successful, turn the orange led on
    Flash_AllocateVar((volatile void **)&NvTowerNb, sizeof(*NvTowerNb));
    Flash_AllocateVar((volatile void **)&NvTowerMd, sizeof(*NvTowerMd));         // allocate memory for tower number and mode
    PIT_Init(CPU_BUS_CLK_HZ);                                                    // Initialize , enable and set PIT clock
    PIT_Enable(bTRUE);
    PIT_Set(500000000, bTRUE);
    StartupPacket();
    RTC_Init();                                                // Initialize RTC

    timer.channelNb = 0;                                                         // Use channel 0 for FTM
    timer.timerFunction = TIMER_FUNCTION_OUTPUT_COMPARE;                         // Use output compare as a function

    Timer_Init();                                                                // Initialize timer
    Timer_Set(& timer);                                                          // Set the timer on
    module.primarySlaveAddress = 0x1D;
    module.baudRate = 100000;

    I2C_Init (&module, CPU_BUS_CLK_HZ);

    accel.moduleClk = CPU_BUS_CLK_HZ;
    Accel_Init(& accel);
    pMode = 0;
    Accel_SetMode (ACCEL_POLL);


    OS_ThreadDelete(OS_PRIORITY_SELF);
  }

}

/*! @brief Handles the packet that is sent from PC builds a packet that is going to be sent to the tower.
 *
 *  @param pData is not used but is required by the OS to create a thread.
 *  @return void
 */

 static void HandlePacketThread(void *pData)
 {
   for(;;)
   {

     uint8_t command;                      // new command to store the one which has no acknowledge mask
     BOOL success = bTRUE;                 // variable to store if the byte received is correct
     BOOL ack = bFALSE;                    // variable to store whether the command requires acknowledge

     if (Packet_Get())
     {

       LEDs_On(LED_BLUE);
       Timer_Start (&timer);
       command = Packet_Command;
       command &= ~ (0x80);              // remove packet mask
       if (command != Packet_Command)    // if the one received and the one after removing the mask are different
         ack = bTRUE;                    // then it requires acknowledge
       switch (command)
       {
         case STARTUP :
           if (Packet_Parameter1 == 0 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0) // checking parameters
	   {
	     StartupPacket();
	   }
	   else
	   {
	     success = bFALSE;
	   }
           break;
         case GETVERSION :
           if (Packet_Parameter1 == 'v' && Packet_Parameter2 == 'x' )                      // checking parameters
	   {
	     Packet_Put(GETVERSION, 'v', 1, 0);
	   }
	   else
	   {
	     success = bFALSE;
	   }
	   break;
         case NUMBER :
           if (Packet_Parameter1 == 1 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0 ) // checking parameters
           {
	     Packet_Put(NUMBER, 1, NvTowerNb->s.Lo, NvTowerNb->s.Hi);           // set
           }
           else if (Packet_Parameter1 == 2)
           {
	     Flash_Write16((uint16_t *)NvTowerNb, Packet_Parameter23);          // put it into flash
           }
	   else
	   {
	     success = bFALSE;
	   }
	   break;
         case MODE :
	   if (Packet_Parameter1 == 1 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0) // checking parameters
	   {
	     Packet_Put(MODE, 1, NvTowerMd->s.Lo, NvTowerMd->s.Hi);            // get
	   }
	   else if (Packet_Parameter1 == 2)
	   {
	     Flash_Write16((uint16_t *)NvTowerMd, Packet_Parameter23);         // set
	   }
	   else
	   {
	     success = bFALSE;
	   }
	   break;
         case PMODE:
	   if (Packet_Parameter1 == 1 && Packet_Parameter2 == 0 && Packet_Parameter3 == 0) // checking parameters
	   {
	     Packet_Put(PMODE, 1, pMode, 0);            // get
	   }
	   else if (Packet_Parameter1 == 2)
	   {
	     if (Packet_Parameter2 == 0)
	     {
	       Accel_SetMode(ACCEL_POLL);
	       pMode = 0;
	     }
	     else if (Packet_Parameter2 == 1 )
	     {
	       Accel_SetMode(ACCEL_INT);
	       pMode = 1;
	     }
	   }
	   else
	   {
	     success = bFALSE;
	   }
	   break;
         case PROGRAM :
	   if ((Packet_Parameter1 == 0x00 && Packet_Parameter1 <= 0x07) && Packet_Parameter2 == 0) // checking parameters
	   {
	     Flash_Write8((uint8_t *)(0x0080000 + Packet_Parameter1), Packet_Parameter3);  // program
	   }
	   else if (Packet_Parameter1 == 0x08)
	   {
	     Flash_Erase();                         // erase
	   }
	   else
	   {
	     success = bFALSE;
	   }
	   break;
         case READ :
	   if ((Packet_Parameter1 == 0x00 && Packet_Parameter1 <= 0x07) &&Packet_Parameter2 == 0 && Packet_Parameter3 == 0) // checking parameter
	   {
	     Packet_Put(READ, Packet_Parameter1, 0, *(uint8_t *) (0x0080000 + Packet_Parameter1));   //read
	   }
	   else
	   {
	     success = bFALSE;
	   }
	   break;
         case TIME :
	   EnterCritical();
	   RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
	   ExitCritical();
	   break;
         case GAME :
       	   Packet_Put(GAME, 0, 0, 0);
	   break;
         default:
	   success = bFALSE;
	   break;
       }
     }

     if (ack)                                                                                  //if ack is required
     {
       if (success)
         Packet_Put(Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);  // send ack
       else
         Packet_Put(command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);         // send nak
     }

   }

}

 /*! @brief Reads the accelerometer values and toggles the green LED
  *
  *  @param pData is not used but is required by the OS to create a thread.
  *  @return void
  */
static void AccelReadThread (void *pData)
{
  for(;;)
  {
    (void)OS_SemaphoreWait(AccelReadSemaphore, 0);
    uint8_t data[3];
    Accel_ReadXYZ(data);                                          // Collect accelerometer data
    if ((values[0] != data[0]) || (values[1] != data[1]) || (values[2] != data[2]))  // Send accelerometer data every second only if there is a difference from last time
    {
      values[0] = data[0];
      values[1] = data[1];
      values[2] = data[2];
      LEDs_Toggle(LED_GREEN);
      Packet_Put(ACCEL, data[0], data[1], data[2]);
    }

  }
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
   OS_ERROR error;    //Thread content
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
   PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
   OS_DisableInterrupts();

   OS_Init(CPU_CORE_CLK_HZ, false);            // Initialize the RTOS
   InitSemaphore = OS_SemaphoreCreate(1);
   AccelReadSemaphore = OS_SemaphoreCreate(0);
   /*create application threads vefore start RTOS*/
   error = OS_ThreadCreate(TowerInitThread,
			   NULL,
			   &TowerInitThreadStack[THREAD_STACK_SIZE - 1],      // 3st priority
			   2);

   error = OS_ThreadCreate(AccelReadThread,
			   NULL,
			   &AccelReadThreadStack[THREAD_STACK_SIZE - 1],      // 6st priority
			   5);

   error = OS_ThreadCreate(PITCallbackThread,
			   NULL,
			   &PITCallbackThreadStack[THREAD_STACK_SIZE - 1],    // 7st priority
			   6);

   error = OS_ThreadCreate(RTCCallbackThread,
			   NULL,
			   &RTCCallbackThreadStack[THREAD_STACK_SIZE - 1],    // 8st priority
			   7);

   error = OS_ThreadCreate(FTMCallbackThread,
			   NULL,
			   &FTMCallbackThreadStack[THREAD_STACK_SIZE - 1],    // 9st priority
			   8);

   error = OS_ThreadCreate(HandlePacketThread,
			   NULL,
			   &HandlePacketThreadStack[THREAD_STACK_SIZE - 1],   // 10st priority
			   9);

   error = OS_ThreadCreate(AccelCallbackThread,
			   NULL,
			   &AccelCallbackThreadStack[THREAD_STACK_SIZE - 1],  // 11st priority
			   10);

   error = OS_ThreadCreate(I2CCallbackThread,
			   NULL,
			   &I2CCallbackThreadStack[THREAD_STACK_SIZE - 1],    //Lowest priority
			   11);


   OS_EnableInterrupts();
   OS_Start();                 // Start multithreading - never returns



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

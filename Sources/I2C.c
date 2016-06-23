/*! @file
 *
 *  @brief I/O routines for the K70 I2C interface.
 *
 *  This contains the functions for operating the I2C (inter-integrated circuit) module.
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-05-16
 */
/*!
**  @addtogroup I2C_module I2C module documentation
**  @{
*/
/* MODULE I2C */
#include "I2C.h"
#include "types.h"
#include "IO_Map.h"
#include <stdlib.h>
#include "OS.h"

#define THREAD_STACK_SIZE 500

static uint8_t address;
static uint8_t registerAddress1;
static uint8_t data1;
static uint8_t registerAddress2;
static uint8_t *data2;
static uint8_t bytes;
static uint32_t I2CWriteThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
static uint32_t I2CPollReadThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));

/*! @brief Send a start bit to I2C Module
 *
 *  @return void
 */
void StartI2C()
{
  I2C0_C1 |= I2C_C1_MST_MASK;
  I2C0_C1 |= I2C_C1_TX_MASK;
}

/*! @brief Pause I2C Module
 *
 *  @return void
 */
void PauseI2C()
{
  int time = 0xFFFF;
  while(time--);
}
/*! @brief Send a stop bit to I2C Module
 *
 *  @return void
 */
void StopI2C()
{
  I2C0_C1 &=~ I2C_C1_MST_MASK;
  I2C0_C1 &=~ I2C_C1_TX_MASK;
}
/*! @brief Acknowledge the data transfer
 *
 *  @return void
 */
void AckI2C()
{
  while(!(I2C0_S & I2C_S_IICIF_MASK))
  {

  }
  I2C0_S = I2C_S_IICIF_MASK;

}
/*! @brief Send a restart bit to I2C Module
 *
 *  @return void
 */
void RestartI2C()
{
  I2C0_C1 |= I2C_C1_RSTA_MASK;
}

/*! @brief Write a byte of data to a specified register
 *
 *  @param pData is not used but is required by the OS to create a thread.
 */
static void I2CWriteThread(void *pData)
{
  for(;;)
  {
    (void) OS_SemaphoreWait(I2CInUse, 0);
    while (I2C0_S & I2C_S_BUSY_MASK)
    {

    }
    StartI2C();
    I2C0_D = (address << 1) & 0x10;
    AckI2C();
    I2C0_D = registerAddress1;
    AckI2C();
    I2C0_D = data1;
    AckI2C();
    StopI2C();
    PauseI2C();
  }

}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses polling as the method of data reception.
 * @param pData is not used but is required by the OS to create a thread.
 */
static void I2CPollReadThread(void *pData)
{
  for(;;)
  {
    (void) OS_SemaphoreWait(I2CInUse, 0);
     while (I2C0_S & I2C_S_BUSY_MASK)
     {

     }
     StartI2C();
     I2C0_D = (address << 1) & 0x10;    // Send slave address with write bit
     AckI2C();                          // Acknowledge the data transfer
     I2C0_D = registerAddress2;         // Send slave register address
     AckI2C();
     RestartI2C();                      // Send a restart bit to I2C Module
     I2C0_D = (address << 1) | 0x01;    // Send slave address with read bit
     AckI2C();
     I2C0_C1 &= ~I2C_C1_TX_MASK;       // Receive mode
     I2C0_C1 &= ~I2C_C1_TXAK_MASK;     // Turn on ACK from master
     *data2 = I2C0_D;                  // dummy read
     AckI2C();
     for (int i = 0; i < bytes -2; i++)
     {
       *data2 = I2C0_D;
        data2++;
        AckI2C();
     }
     I2C0_C1 |= I2C_C1_TXAK_MASK; // NACK from master
     *data2 = I2C0_D;
     data2++;
     AckI2C();
     StopI2C();           // STOP signal
     *data2 = I2C0_D;     // ignore last byte
     PauseI2C();
  }
}
/*! @brief Sets up the baudrate of I2C
 *
 *  @param aI2CModule is a structure containing the operating conditions for the module.
 *  @param moduleClk The module clock in Hz.
 *  @return void
 */
void SetBaudrate (const TI2CModule* const aI2CModule, const uint32_t moduleClk)
{
  uint8_t mult;                                     // value to calculate baudrate
  uint8_t icr;
  int val;
  int difference;
  int baudrate = 0;
  int mul[3] = {1,2,4};
  int scl[64] = {20,22,24,26,28,30,34,40,28,32,36,40,44,48,56,68,48,56,64,72,80,88,104,
                128,80,96,112,128,144,160,192,240,160,192,224,256,288,320,384,480,320,384,
		448.512,576,640,768,960,640,768,896,1024,1152,1280,1536,1920,1280,1536,1792,
		2048,2304,2560,3072,3840};
  int i,j;
  for (i = 0; i < 64; i++)
  {
    for (j = 0; j < 3; j++)
    {
      val = moduleClk / (mul[j] * scl[i]);                                  // finding the closest value for baudrate
      difference = aI2CModule -> baudRate - val;
      if (abs(difference) < abs (aI2CModule -> baudRate - baudrate))
      {
        mult = j;
        icr = i;
        baudrate = val;
      }
    }

  }
  I2C0_F |= mult << 6;
  I2C0_F |= icr;
}
BOOL I2C_Init(const TI2CModule* const aI2CModule, const uint32_t moduleClk)
{
  OS_ERROR error;
  SIM_SCGC4 |= SIM_SCGC4_IIC0_MASK;                   // enable IIC0 module
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;                  // enable PORTE
  PORTE_PCR18 = PORT_PCR_MUX(4);                      // enable IIC0_SDA
  PORTE_PCR19 = PORT_PCR_MUX(4);                      // enable IIC0_CLK
  SetBaudrate(aI2CModule, moduleClk);
  I2C0_C1 |= I2C_C1_IICEN_MASK;                       // enable I2C
  I2C_SelectSlaveDevice(aI2CModule -> primarySlaveAddress);
  I2CSemaphore = OS_SemaphoreCreate(0); // I2CSemaphore semaphore initialized to 1
  I2CInUse = OS_SemaphoreCreate(0);     // I2CInUse semaphore initialized to 1
  error = OS_ThreadCreate(I2CWriteThread,
			  NULL,
			  &I2CWriteThreadStack[THREAD_STACK_SIZE - 1],     // Highest priority
			  0);

  error = OS_ThreadCreate(I2CPollReadThread,
			  NULL,
			  &I2CPollReadThreadStack[THREAD_STACK_SIZE - 1],  // 2st priority
			  1);
  return bTRUE;

}


void I2C_SelectSlaveDevice(const uint8_t slaveAddress)
{
  address = slaveAddress; // Store the slave address globally(private)

}


void I2C_Write(const uint8_t registerAddress, const uint8_t data)
{
  registerAddress1 = registerAddress;
  data1 = data;
  (void)OS_SemaphoreSignal(I2CInUse); // Wait for previous write to slave device is complete

}


void I2C_PollRead(const uint8_t registerAddress, uint8_t* data, const uint8_t nbBytes)
{
  registerAddress2 = registerAddress;
  data2 = data;
  bytes = nbBytes;
  (void)OS_SemaphoreSignal(I2CInUse); // Wait for previous write to slave device is complete
}


void I2C_IntRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
  I2C0_C1 = registerAddress;
  *data = I2C0_C1;
  I2C0_C1 |= I2C_C1_IICIE_MASK; // I2C enable
  NVICICPR0 |= (1 << 24);       // Clear any pending interrupts on I2C0
  NVICISER0 |= (1 << 24);       // Enable interrupts on I2C0

}


void __attribute__ ((interrupt)) I2C_ISR(void)
{
  I2C0_S |= I2C_S_IICIF_MASK;                       // Clear interrupt flag
  OS_ISREnter();                                    // Start of servicing interrupt
  (void)OS_SemaphoreSignal(I2CSemaphore);
  OS_ISRExit();                                     // End of servicing interrupt
}
/* END I2C */
/*!
 * @}
*/


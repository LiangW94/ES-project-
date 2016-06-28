/*! @file
 *
 *  @brief I/O routines for the K70 I2C interface.
 *
 *  This contains the functions for operating the I2C (inter-integrated circuit) module.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
/*!
**  @addtogroup I2C_module I2C module documentation
**  @{
*/
/* MODULE I2C */

#include "I2C.h"
void(*userFunctionD)(void*);  	                /*!< Callback function. */
void* userArgumentsD;        		        /*!< Callback function. */
static uint8_t devadd;
int ABS(int x)
{
  if(x<0)
    x=-x;
  return x;
}
/*! @brief Sets up the I2C before first use.
 *
 *  @param aI2CModule is a structure containing the operating conditions for the module.
 *  @param moduleClk The module clock in Hz.
 *  @return BOOL - TRUE if the I2C module was successfully initialized.
 */
BOOL I2C_Init(const TI2CModule* const aI2CModule, const uint32_t moduleClk)
{
  uint8_t mul, icr;
  int baudRateError,baudRate,baudRateStoreError,baudRateStore = 0;
  int i,l;
  int scl[64] = {20,22,24,26,28,30,34,40,28,32,36,40,44,48,56,68,48,56,64,72,80,88,104,128,80,96,112,128,144,160,192,240,160,192,224,256,288,320,384,480,320,384,448,512,576,640,768,960,640,768,896,1024,1152,1280,1536,1920,1280,1536,1792,2048,2304,2560,3072,3840};
  int mulValue[3] = {1,2,4};
  SIM_SCGC4 |= SIM_SCGC4_IIC0_MASK; 	/*! Turn on clock to I2C0 module. */
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK; 	/*!enable the PORTE clock gate.*/
  PORTE_PCR18 = PORT_PCR_MUX(4);  	/*!Set MUX(bit 10 to 8) to 4(100) in PORTE18 to choose ALT4 to choose I2C0_SDA.*/
  PORTE_PCR19 = PORT_PCR_MUX(4);  	/*!Set MUX(bit 10 to 8) to 4(100) in PORTE19 to choose ALT4 to choose I2C0_SCL.*/
  userFunctionD = aI2CModule->readCompleteCallbackFunction;  	                /*!< Callback function. */
  userArgumentsD = aI2CModule->readCompleteCallbackArguments;
  devadd = aI2CModule->primarySlaveAddress;                  	/*Primary Slave-address*/
  for(i = 0; i <= 63; i++)
  {
    for(l = 0; l <= 2; l++)
    {
      baudRate = (int)moduleClk / (mulValue[l]*scl[i]);
      baudRateError = (int)aI2CModule->baudRate - baudRate;
      baudRateStoreError = (int)aI2CModule->baudRate - baudRateStore;
      if(ABS(baudRateError) < ABS(baudRateStoreError))
      {
	mul = l;
  	icr = i;
  	baudRateStore = baudRate;
      }
    }
  }
  I2C0_F |= mul<<6;
  I2C0_F |= icr;
  I2C0_C1 |= I2C_C1_IICEN_MASK;   	/* enable IIC */
  I2C0_C1 |= I2C_C1_IICIE_MASK;   	/* interrupt enable */
  I2C0_C1 |= I2C_C1_MST_MASK;		/* enable master mode*/
  I2C0_C1 |= I2C_C1_TX_MASK;		/* enable Transmit mode*/
  //NVICICPR0 |= (1<<24);
 // NVICISER0 |= (1<<24);
  return bTRUE;
}

void Pause(void)
{
  int i;
  for(i = 0; i < 50; i++)
  {
    __asm ("nop");
  }
}
void I2C_Wait(void)                                          
{
		while(!(I2C0_S & I2C_S_IICIF_MASK))
		{}
                I2C0_S |= I2C_S_IICIF_MASK;
}
/*! @brief Selects the current slave device
 *
 * @param slaveAddress The slave device address.
 */
void I2C_SelectSlaveDevice(const uint8_t slaveAddress)
{
  devadd = slaveAddress;
}

/*! @brief Write a byte of data to a specified register
 *
 * @param registerAddress The register address.
 * @param data The 8-bit data to write.
 */
void I2C_Write(const uint8_t registerAddress, const uint8_t data)
{
  //I2C_Start();
  start() ;
  //I2C_Wait();
  I2C0_D = registerAddress;
  I2C_Wait();
  I2C0_D = data;
  I2C_Wait();
  stop();
  Pause();
}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses polling as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_PollRead(const uint8_t registerAddress, uint8_t* data, const uint8_t nbBytes)
{
  static int i;
  uint8_t addBit,empdata;

  start() ;
  addBit = READ;                         
  addBit &= ~devadd<<1;                 
  I2C0_D = addBit;                
  I2C_Wait();                            
  I2C0_D =registerAddress;
  I2C_Wait();
  I2C0_C1 |= I2C_C1_RSTA_MASK;
  addBit = READ;
  addBit |= devadd<<1;
  I2C0_D = addBit;
  I2C_Wait();
  I2C0_C1 &= ~I2C_C1_TX_MASK;
  I2C0_C1 &= ~I2C_C1_TXAK_MASK;
  empdata = I2C0_D;
  I2C_Wait();
  for(i = 0; i < nbBytes-1; i++)
  {
    if(i == (nbBytes-2))
      I2C0_C1 |= I2C_C1_TXAK_MASK;
    *data = I2C0_D;
    data++;
    I2C_Wait();
  }
  stop();
  *data = I2C0_D;
  Pause();                                     
}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses interrupts as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_IntRead(const uint8_t registerAddress, uint8_t* data, const uint8_t nbBytes)
{
  I2C0_S |= I2C_S_IICIF_MASK;
  int i;
  uint8_t empdata, addBit;
  //I2C_Start();
  start() ;
  //I2C_Wait();
  addBit = READ;                       
  addBit &= ~devadd<<1;             
  I2C0_D = addBit;                  
  I2C_Wait();                        
  I2C0_D = registerAddress;
  I2C_Wait();
  I2C0_C1 |= I2C_C1_RSTA_MASK;  //restart
  addBit = READ;
  addBit |= devadd<<1;
  I2C0_D = addBit;
  
  I2C0_C1 &=~ I2C_C1_TX_MASK;
  I2C0_C1 &=~ I2C_C1_TXAK_MASK;
  empdata = I2C0_D;
  I2C_Wait();
  for(i = nbBytes; i > 1; i--)
  {
    if(i == 2)
      I2C0_C1 |= I2C_C1_TXAK_MASK;
    *data = I2C0_D;
    data++;
    I2C_Wait();
  }
  stop();
  *data = I2C0_D;
  Pause();
}

/*! @brief Interrupt service routine for the I2C.
 *
 *  Only used for reading data.
 *  At the end of reception, the user callback function will be called.
 *  @note Assumes the I2C module has been initialized.
 */
/*void __attribute__ ((interrupt)) I2C_ISR(void)
{
  I2C0_S |= I2C_S_IICIF_MASK;
  (*userFunctionD)(userArgumentsD);
}
*/
/* END I2C */
/*!
** @}
*/

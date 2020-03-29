/*! \file   i2c_srv.c

    \brief  Low level I2C support
        
*/




#include "i2c_srv.h"
#include "stm32h7xx_hal_i2c.h"
#include "stm32h7xx_ll_i2c.h"
#include "services.h"
#include "i2c.h"

#define SRV_I2C_TIMEOUT     20 /* ms */
#define SRV_I2C_HALF_PULSE   5 /* us */

#define SRV_I2C_SPEED_CH0   200000




extern I2C_HandleTypeDef hi2c1;


uint32_t    srv_i2c_reset_cnt[1] = {0};


I2C_HandleTypeDef * srv_i2c_iidx2instance(uint32_t iidx)
{
    if(iidx == 0)
    {
        return &hi2c1;
    }
    else
    {
        return NULL;
    }
}

static void     srv_i2c_reconfiguration(uint32_t iidx)
{

  if(iidx == 0)
  {
      
	  LL_I2C_Disable(I2C1);
      memset(&hi2c1,0,sizeof(hi2c1));
      LL_I2C_Enable(I2C1);
      MX_I2C1_Init();
  }
  else
  {

  }

}



void     srv_i2c_configuration(uint32_t iidx)
{
  srv_i2c_reconfiguration(iidx);
}

void     srv_i2c_reset_slaves(uint32_t iidx)
{
    int              ii;
    GPIO_InitTypeDef GPIO_InitStruct;

    // If I2C stuck then force recovery by pulsing 8 clock cycles

    if(iidx == 0)
    {
      srv_i2c_reset_cnt[0]++;


      // I2C1

      GPIO_InitStruct.Pin   = I2C1_SCL_Pin;
      GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
      GPIO_InitStruct.Pull  = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      HAL_GPIO_Init(I2C1_SCL_GPIO_Port, &GPIO_InitStruct);

      GPIO_InitStruct.Pin   = I2C1_SDA_Pin;
      GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
      GPIO_InitStruct.Pull  = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      HAL_GPIO_Init(I2C1_SDA_GPIO_Port, &GPIO_InitStruct);

      HAL_GPIO_WritePin(I2C1_SDA_GPIO_Port, I2C1_SDA_Pin, GPIO_PIN_RESET);

      for(ii =0; ii < 8;ii++)
      {
            HAL_GPIO_WritePin(I2C1_SCL_GPIO_Port, I2C1_SCL_Pin, GPIO_PIN_RESET);
            vTaskDelay(1);
            HAL_GPIO_WritePin(I2C1_SCL_GPIO_Port, I2C1_SCL_Pin, GPIO_PIN_SET);
            vTaskDelay(1);
      }

      HAL_GPIO_WritePin(I2C1_SCL_GPIO_Port, I2C1_SCL_Pin, GPIO_PIN_SET);
      vTaskDelay(1);
      HAL_GPIO_WritePin(I2C1_SDA_GPIO_Port, I2C1_SDA_Pin, GPIO_PIN_SET);
      vTaskDelay(1);


      GPIO_InitStruct.Pin = I2C1_SCL_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
      HAL_GPIO_Init(I2C1_SCL_GPIO_Port, &GPIO_InitStruct);


      GPIO_InitStruct.Pin = I2C1_SDA_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
      HAL_GPIO_Init(I2C1_SDA_GPIO_Port, &GPIO_InitStruct);


   }
   else
   {



   }

   srv_i2c_reconfiguration(iidx);
    

}

void     srv_i2c_recovery(uint32_t iidx)
{
  I2C_HandleTypeDef *  hi2c = srv_i2c_iidx2instance(iidx); 
    
  if(__HAL_I2C_GET_FLAG(hi2c,I2C_FLAG_BUSY) != 0)
  {
     srv_i2c_reset_slaves(iidx);
  }
}



uint32_t srv_i2c_read_buffer(uint32_t iidx,uint8_t * pBuffer, uint16_t DevAddr,uint16_t ReadAddr, volatile uint32_t * NumByteToRead)
{
  HAL_StatusTypeDef status = HAL_OK;
  int               result = 0;
  uint32_t          tout = 10;



  I2C_HandleTypeDef *  hi2c = srv_i2c_iidx2instance(iidx);

  status = HAL_I2C_Mem_Read_DMA(hi2c, DevAddr, ReadAddr, I2C_MEMADD_SIZE_8BIT, pBuffer, *NumByteToRead);
 
  /* Check the communication status */
  if(status == HAL_OK)
  {
    while(hi2c->State != HAL_I2C_STATE_READY)
    {
      vTaskDelay(1);
      if(tout-- == 0)
      {
          srv_i2c_reconfiguration(iidx);
          return 1;
      }
    }
    result =  0;
  }
  else
  {
    srv_i2c_reset_slaves(iidx);
    srv_i2c_reconfiguration(iidx);
    result = 1;
  }
     
  return result;

}




int      srv_i2c_write_buffer(uint32_t iidx,const uint8_t* pBuffer,uint16_t DevAddr, uint16_t WriteAddr, volatile uint32_t * NumByteToWrite)
{
  HAL_StatusTypeDef status = HAL_OK;
  int               result = 0;
  uint32_t          tout = 10;

  I2C_HandleTypeDef *  hi2c = srv_i2c_iidx2instance(iidx);

  status = HAL_I2C_Mem_Write_DMA(hi2c, DevAddr, WriteAddr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)pBuffer, *NumByteToWrite);//,50);

  if(status == HAL_OK)
  {
    while(hi2c->State != HAL_I2C_STATE_READY)
    {
      vTaskDelay(1);
      if(tout-- == 0)
      {
          srv_i2c_reconfiguration(iidx);
          return 1;
      }
    }

    result =  0;
  }
  else
  {
    srv_i2c_reset_slaves(iidx);
    srv_i2c_reconfiguration(iidx);


    result = 1;
  }

  return result;
}




void     srv_i2c_init(void)
{
  srv_i2c_configuration(I2C_IIDX_BASE);

}

void      srv_i2c_once(void)
{

}

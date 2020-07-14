/*! \file   srv_nov_qspi.c

    \brief  Low level QSPI support
    
    
*/ 

#include "system.h"
#include "nov_srv_qspi.h"
#include "quadspi.h"

extern QSPI_HandleTypeDef hqspi;

volatile uint8_t CmdCplt, RxCplt, TxCplt, StatusMatch, TimeOut;

static   int is_mapped;


static int QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi);
static int QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi);
static int QSPI_MemMap(QSPI_HandleTypeDef *hqspi,int32_t enable);


static int32_t srv_nov_qspi_setup_quad(void)
{

	  QSPI_CommandTypeDef s_command;
	  uint8_t reg=0;


	  /* Initialize the read status reg 2 */
	  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	  s_command.Instruction       = READ_STATUS_REG2_CMD;
	  s_command.AddressMode       = QSPI_ADDRESS_NONE;
	  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	  s_command.DataMode          = QSPI_DATA_1_LINE;
	  s_command.DummyCycles       = 0;
	  s_command.NbData            = 1;
	  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
	  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	  /* Configure the command */
	  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	  {
	     return -1;
	  }

	  /* Reception of the data */
	  if (HAL_QSPI_Receive(&hqspi, (uint8_t *)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	  {
		  return -1;
	  }

	  /* Enable quad mode if needed */
	  if( (reg & 0x02 /* QE - quad enable bit */) == 0)
	  {
		  reg |= 0x02;
		  /* Enable write operations */
		  QSPI_WriteEnable(&hqspi);

		  /* Update volatile configuration register (with new dummy cycles) */
		  s_command.Instruction = WRITE_STATUS_REG2_CMD;

		  /* Configure the write volatile configuration register command */
		  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		  {
			  return -1;
		  }

		  /* Transmission of the data */
		  if (HAL_QSPI_Transmit(&hqspi, (uint8_t *)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		  {
			return -1;
		  }
	  }

	  return 0;

}


int32_t  srv_nov_qspi_init(void)
{
	int32_t result;


	is_mapped = 0;

	result = srv_nov_qspi_setup_quad();
	if(result != 0)
	{
		return result;
	}

	result = QSPI_MemMap(&hqspi,1);

	return result;
}




int32_t  srv_nov_qspi_write(const uint8_t * data, size_t size, uint32_t flash_offset)
{
	QSPI_CommandTypeDef 	 sCommand;
	uint32_t				 ii;
	uint32_t				 total = 0;


	if( (flash_offset % QSPI_SECTOR_SIZE) !=0)
	{
		// We serve only aligned requests
		return -1;
	}


	QSPI_MemMap(&hqspi,0);


	for(ii = 0; ii < (flash_offset + QSPI_SECTOR_SIZE -1) / QSPI_SECTOR_SIZE;ii++)
	{
		/* Block erase */
		sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
		sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
		sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
		sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
		sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
		sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

		CmdCplt = 0;

		QSPI_WriteEnable(&hqspi);

		sCommand.Instruction = SECTOR_4K_ERASE_CMD;
		sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
		sCommand.Address     = flash_offset + QSPI_SECTOR_SIZE * ii;
		sCommand.DataMode    = QSPI_DATA_NONE;
		sCommand.DummyCycles = 0;

		if (HAL_QSPI_Command_IT(&hqspi, &sCommand) != HAL_OK)
		{
			return -1;
		}


		while(CmdCplt == 0)
		{

		}

		CmdCplt = 0;
		StatusMatch = 0;

		/* Configure automatic polling mode to wait for end of erase ------- */
		QSPI_AutoPollingMemReady(&hqspi);

		while(StatusMatch  == 0)
		{

		};
	}
    
	for(ii = 0; ii < (size + QSPI_PAGE_SIZE -1) / QSPI_PAGE_SIZE;ii++)
	{

		/* Page program */
		sCommand.Instruction = QUAD_IN_FAST_PROG_CMD;
		sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
		sCommand.Address     = flash_offset + QSPI_PAGE_SIZE * ii;
		sCommand.DataMode    = QSPI_DATA_4_LINES;

		total += QSPI_PAGE_SIZE;

		if(total < size)
		{
			sCommand.NbData = QSPI_PAGE_SIZE ;
		}
		else
		{
			sCommand.NbData = size % QSPI_PAGE_SIZE; // last chunk - not full page
		}

		sCommand.DummyCycles = 0;

		StatusMatch = 0;
		TxCplt = 0;

		if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			  return -1;
		}

		if (HAL_QSPI_Transmit_DMA(&hqspi, &(((uint8_t *)data)[QSPI_PAGE_SIZE*ii])) != HAL_OK)
		{
			  return -1;
		}

		while(CmdCplt == 0)
		{

		}

		TxCplt = 0;
		StatusMatch = 0;

		QSPI_AutoPollingMemReady(&hqspi);

		while(StatusMatch == 0)
		{
		}

	}

	QSPI_MemMap(&hqspi,1);


	for (ii = 0; ii < size; ii++)
	{
		 if (QSPI_START[flash_offset+ii] != data[ii])
		 {
		   return -2;
		 }
	}


	return 0;

}




/**
  * @brief  Command completed callbacks.
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_CmdCpltCallback(QSPI_HandleTypeDef *hqspi)
{
  CmdCplt++;
}

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
  RxCplt++;
}

/**
  * @brief  Tx Transfer completed callbacks.
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
  TxCplt++;
}

/**
  * @brief  Status Match callbacks
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
  StatusMatch++;
}


/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  hqspi: QSPI handle
  * @retval None
  */

static int QSPI_MemMap(QSPI_HandleTypeDef *hqspi,int32_t enable)
{
  QSPI_CommandTypeDef       sCommand;
  QSPI_MemoryMappedTypeDef  sMemMappedCfg;
  volatile uint32_t 		dummy;


  if(enable != 0)
  {
	  if(is_mapped != 0)
	  {
		  return 0;
	  }

	  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
	  sCommand.AddressMode 		 = QSPI_ADDRESS_1_LINE;
	  sCommand.DataMode    		 = QSPI_DATA_4_LINES;
	  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
	  sCommand.DummyCycles = 1;

	  sCommand.Instruction 			  = QUAD_OUT_FAST_READ_CMD;
	  sCommand.Address  			  = 0;
	  sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

	  if (HAL_QSPI_MemoryMapped(hqspi, &sCommand, &sMemMappedCfg) != HAL_OK)
	  {
		return -1;
	  }

	  // Force dummy read to actually enter the mode
	  dummy = *((uint32_t*)QSPI_START);
	  dummy++;

	  is_mapped = 1;
  }
  else
  {
	  if(is_mapped == 0)
	  {
		  return 0;
	  }

	  HAL_QSPI_Abort(hqspi);

	  is_mapped = 0;

  }

  return 0;
}



/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  hqspi: QSPI handle
  * @retval None
  */

static int QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Enable write operations ------------------------------------------ */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = WRITE_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode         = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return -1;
  }

  /* Configure automatic polling mode to wait for write enabling ---- */
  sConfig.Match           = 0x02;
  sConfig.Mask            = 0x02;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  sCommand.Instruction    = READ_STATUS_REG1_CMD;
  sCommand.DataMode       = QSPI_DATA_1_LINE;

  if (HAL_QSPI_AutoPolling(hqspi, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
	  return -1;
  }

  return 0;
}

/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static int  QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Configure automatic polling mode to wait for memory ready ------ */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_STATUS_REG1_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode         = QSPI_SIOO_INST_EVERY_CMD;

  sConfig.Match           = 0x00;
  sConfig.Mask            = 0x01;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling_IT(hqspi, &sCommand, &sConfig) != HAL_OK)
  {
    return -1;
  }

  return 0;
}



/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/ethernetif.c
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    13-March-2015
  * @brief   This file implements Ethernet network interface drivers for lwIP
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "lwip/opt.h"
//#include "lwip/lwip_timers.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "enc28j60/enc28j60.h"
#include <string.h>

/* Imported variables --------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT                 ( 100 )

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
ENC_HandleTypeDef EncHandle;
SPI_HandleTypeDef hspi4;
#ifdef USE_PROTOTHREADS
static struct pt transmit_pt;
#endif

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* SPI4 init function */
void MX_SPI4_Init(void)
{

  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLED;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
  HAL_SPI_Init(&hspi4);

}


/*******************************************************************************
                       ENC28J60 MSP Routines
*******************************************************************************/
/**
  * @brief  Initializes the ENC28J60 MSP.
  * @param  heth: ENC28J60 handle
  * @retval None
  */
void ENC_MSPInit(ENC_HandleTypeDef *heth)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOs clocks */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

/* ENC28J60 pins configuration ************************************************/
  /*
        ENC28J60_CS ----------------------> PB4
        ENC28J60_INT ---------------------> PG2
  */

  /*Configure GPIO pins : PB4 */
  GPIO_InitStructure.Pin = GPIO_PIN_4;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Deselect ENC28J60 module */
  HAL_GPIO_WritePin(GPIOB, ENC_CS_PIN, GPIO_PIN_SET);

#ifdef ENC28J60_INTERRUPT
  /*Configure GPIO pin : PG2 */
  GPIO_InitStructure.Pin = GPIO_PIN_2;
  GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);
#endif /* ENC28J60_INTERRUPT */

  /* Initialize SPI */
  MX_SPI4_Init();

#ifdef ENC28J60_INTERRUPT
  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0x0F, 0x0F);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
#endif /* ENC28J60_INTERRUPT */

}

/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
*******************************************************************************/
/**
  * @brief In this function, the hardware should be initialized.
  * Called from ethernetif_init().
  *
  * @param netif the already initialized lwip network interface structure
  *        for this ethernetif
  */
static void low_level_init(struct netif *netif)
{
  //uint8_t macaddress[6]= { MAC_ADDR0, MAC_ADDR1, MAC_ADDR2, MAC_ADDR3, MAC_ADDR4, MAC_ADDR5 };

  /* Initialize transmit protothread */
#ifdef USE_PROTOTHREADS
  PT_INIT(&transmit_pt);
#endif

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] =  MAC_ADDR0;
  netif->hwaddr[1] =  MAC_ADDR1;
  netif->hwaddr[2] =  MAC_ADDR2;
  netif->hwaddr[3] =  MAC_ADDR3;
  netif->hwaddr[4] =  MAC_ADDR4;
  netif->hwaddr[5] =  MAC_ADDR5;

  EncHandle.Init.MACAddr = netif->hwaddr;
  EncHandle.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
  EncHandle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  EncHandle.Init.InterruptEnableBits =  EIE_LINKIE | EIE_PKTIE;

  /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
  ENC_MSPInit(&EncHandle);
  /* Set netif link flag */
//  netif->flags |= NETIF_FLAG_LINK_UP;

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  /* Start the EN28J60 module */
  if (ENC_Start(&EncHandle)) {
    /* Set the MAC address */
    ENC_SetMacAddr(&EncHandle);

    /* Set netif link flag */
    netif->flags |= NETIF_FLAG_LINK_UP;
  }
}

/**
  * @brief This function should do the actual transmission of the packet. The packet is
  * contained in the pbuf that is passed to the function. This pbuf
  * might be chained.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
  * @return ERR_OK if the packet could be sent
  *         an err_t value if the packet couldn't be sent
  *
  * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
  *       strange results. You might consider waiting for space in the DMA queue
  *       to become available since the stack doesn't retry to send a packet
  *       dropped because of memory failure (except for the TCP timers).
  */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    /* TODO use netif to check if we are the right ethernet interface */
  err_t errval;
  struct pbuf *q;
  uint32_t framelength = 0;

  if (EncHandle.transmitLength != 0) {
#ifdef USE_PROTOTHREADS
     while (PT_SCHEDULE(ENC_Transmit(&transmit_pt, &EncHandle))) {
         /* Wait for end of previous transmission */
     }
#else
     do {
         ENC_Transmit(&EncHandle);
     } while (EncHandle.transmitLength != 0);
#endif
  }

  /* Prepare ENC28J60 Tx buffer */
  errval = ENC_RestoreTXBuffer(&EncHandle, p->tot_len);
  if (errval != ERR_OK) {
      return errval;
  }

  /* copy frame from pbufs to driver buffers and send packet */
  for(q = p; q != NULL; q = q->next) {
    ENC_WriteBuffer(q->payload, q->len);
    framelength += q->len;
  }

  if (framelength != p->tot_len) {
     return ERR_BUF;
  }

  EncHandle.transmitLength = p->tot_len;

  /* If PROTOTHREADS are use, actual transmission is triggered in main loop */
#ifndef USE_PROTOTHREADS
    ENC_Transmit(&EncHandle);
#endif

  return ERR_OK;
}

/**
  * @brief Should allocate a pbuf and transfer the bytes of the incoming
  * packet from the interface into the pbuf.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return a pbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;
  struct pbuf *q;
  uint16_t len;
  uint8_t *buffer;
  uint32_t bufferoffset = 0;

  if (!ENC_GetReceivedFrame(&EncHandle)) {
    return NULL;
  }

  /* Obtain the size of the packet and put it into the "len" variable. */
  len = EncHandle.RxFrameInfos.length;
  buffer = (uint8_t *)EncHandle.RxFrameInfos.buffer;

  if (len > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  if (p != NULL)
  {
    bufferoffset = 0;

    for(q = p; q != NULL; q = q->next)
    {
      /* Copy data in pbuf */
      memcpy( (uint8_t*)((uint8_t*)q->payload), (uint8_t*)((uint8_t*)buffer + bufferoffset), q->len);
      bufferoffset = bufferoffset + q->len;
    }
  }

  return p;
}

/**
  * @brief This function should be called when a packet is ready to be read
  * from the interface. It uses the function low_level_input() that
  * should handle the actual reception of bytes from the network
  * interface. Then the type of the received packet is determined and
  * the appropriate input function is called.
  *
  * @param netif the lwip network interface structure for this ethernetif
  */
void ethernetif_input_do(struct netif * netif)
{
    struct pbuf *p;

    do {
        p = low_level_input(netif);
        if (p != NULL)
        {
          if (netif->input(p, netif) != ERR_OK )
          {
            pbuf_free(p);
          }
        }
    }while(p!=NULL);
}

/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function low_level_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to netif_add().
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if the loopif is initialized
  *         ERR_MEM if private data couldn't be allocated
  *         any other err_t on error
  */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "stm32idisco";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/**
  * @brief  This function actually process pending IRQs.
  * @param  handler: Reference to the driver state structure
  * @retval None
  */
void ethernetif_process_irq_do(void const *argument)
{
    struct enc_irq_str *irq_arg = (struct enc_irq_str *)argument;

    /* Handle ENC28J60 interrupt */
    ENC_IRQHandler(&EncHandle);

    /* Check whether the link is up or down*/
    if ((EncHandle.interruptFlags & EIE_LINKIE) != 0) {
        if((EncHandle.LinkStatus & PHSTAT2_LSTAT)!= 0) {
            netif_set_link_up(irq_arg->netif);
        } else {
            netif_set_link_down(irq_arg->netif);
        }
    }

    /* Check whether we have received a packet */
    if((EncHandle.interruptFlags & EIR_PKTIF) != 0) {
        ethernetif_input_do(irq_arg->netif);
    }

    /* Renable global interrupts */
    ENC_EnableInterrupts(EIE_INTIE);
}

/**
  * @brief  This function triggers the interrupt service callback.
  * @param  netif: the network interface
  * @retval None
  */
void ethernetif_process_irq(void const *argument)
{
  struct enc_irq_str *irq_arg = (struct enc_irq_str *)argument;

  for(;;)
  {
    if (osSemaphoreWait(irq_arg->semaphore, TIME_WAITING_FOR_INPUT) == osOK)
    {
        /* Handle ENC28J60 interrupt */
        tcpip_callback((tcpip_callback_fn) ethernetif_process_irq_do, (void *) argument);
    }
  }
}

/**
  * @brief  This function unblocks ethernetif_process_irq when a new interrupt is received
  * @param  netif: the network interface
  * @retval None
  */
void ethernet_irq_handler(osSemaphoreId Netif_IrqSemaphore)
{
    /* Release thread to check interrupt flags */
     osSemaphoreRelease(Netif_IrqSemaphore);
}

/**
  * @brief  Link callback function, this function is called on change of link status
  *         to update low level driver configuration.
* @param  netif: The network interface
  * @retval None
  */
void ethernetif_update_config(struct netif *netif)
{
  if(netif_is_link_up(netif)) {
      /* Restart the EN28J60 module */
      low_level_init(netif);
  }

  ethernetif_notify_conn_changed(netif);
}

/**
  * @brief  This function notify user about link status changement.
  * @param  netif: the network interface
  * @retval None
  */
__weak void ethernetif_notify_conn_changed(struct netif *netif)
{
  /* NOTE : This is function could be implemented in user file
            when the callback is needed,
  */
}

/**
 * Implement actual transmission triggering
 */
 #if 0
void ethernet_transmit(void) {
#ifdef USE_PROTOTHREADS
    ENC_Transmit(&transmit_pt, &EncHandle);
#else
    ENC_Transmit(&EncHandle);
#endif
}
#endif

/**
  * Implement SPI single byte send and receive.
  * The ENC28J60 slave SPI must already be selected and wont be deselected after transmission
  * Must be provided by user code
  * param  command: command or data to be sent to ENC28J60
  * retval answer from ENC28J60
  */

uint8_t ENC_SPI_SendWithoutSelection(uint8_t command)
{
    HAL_SPI_TransmitReceive(&hspi4, &command, &command, 1, 1000);
    return command;
}

/**
  * Implement SPI single byte send and receive. Must be provided by user code
  * param  command: command or data to be sent to ENC28J60
  * retval answer from ENC28J60
  */

uint8_t ENC_SPI_Send(uint8_t command)
{
    /* Select ENC28J60 module */
    HAL_NVIC_DisableIRQ(EXTI2_IRQn);
    HAL_GPIO_WritePin(GPIOB, ENC_CS_PIN, GPIO_PIN_RESET);
    up_udelay(1);

    HAL_SPI_TransmitReceive(&hspi4, &command, &command, 1, 1000);

    /* De-select ENC28J60 module */
    HAL_GPIO_WritePin(GPIOB, ENC_CS_PIN, GPIO_PIN_SET);
    up_udelay(1);

    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    return command;
}

/**
  * Implement SPI buffer send and receive. Must be provided by user code
  * param  master2slave: data to be sent from host to ENC28J60, can be NULL if we only want to receive data from slave
  * param  slave2master: answer from ENC28J60 to host, can be NULL if we only want to send data to slave
  * retval none
  */

void ENC_SPI_SendBuf(uint8_t *master2slave, uint8_t *slave2master, uint16_t bufferSize)
{
    /* Select ENC28J60 module */
    HAL_NVIC_DisableIRQ(EXTI2_IRQn);
    HAL_GPIO_WritePin(GPIOB, ENC_CS_PIN, GPIO_PIN_RESET);
    up_udelay(1);

    /* Transmit or receuve data */
    if (slave2master == NULL) {
        if (master2slave != NULL) {
            HAL_SPI_Transmit(&hspi4, master2slave, bufferSize, 1000);
        }
    } else if (master2slave == NULL) {
        HAL_SPI_Receive(&hspi4, slave2master, bufferSize, 1000);
    } else {
        HAL_SPI_TransmitReceive(&hspi4, master2slave, slave2master, bufferSize, 1000);
    }

    /* De-select ENC28J60 module */
    HAL_GPIO_WritePin(GPIOB, ENC_CS_PIN, GPIO_PIN_SET);
    up_udelay(1);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}

/**
  * Implement SPI Slave selection and deselection. Must be provided by user code
  * param  select: true if the ENC28J60 slave SPI if selected, false otherwise
  * retval none
  */

void ENC_SPI_Select(bool select)
{
    /* Select or de-select ENC28J60 module */
    if (select) {
        HAL_NVIC_DisableIRQ(EXTI2_IRQn);
        HAL_GPIO_WritePin(GPIOB, ENC_CS_PIN, GPIO_PIN_RESET);
        up_udelay(1);
    } else {
        HAL_GPIO_WritePin(GPIOB, ENC_CS_PIN, GPIO_PIN_SET);
        up_udelay(1);
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

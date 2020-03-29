#include "serial_485_srv.h"
#include "services.h"


#define UF(inst_, flag_)  ( (((inst_)->CR1 & (flag_)) == (flag_) )? SET : RESET)

typedef struct
{    
    xSemaphoreHandle    sema;   
 
    
    serial_rec_485_char rcv_char;

    uint8_t   TxBuffer[128];
    int       TxHead;
    int       TxTail;

    USART_TypeDef     * usart;
    GPIO_TypeDef      * gpio;
    uint16_t            pin;

}srv_serial_485_ctx_t;

typedef struct
{   
    srv_serial_485_ctx_t u2;
    srv_serial_485_ctx_t u3;
}srv_serial_485_data_t;

extern   UART_HandleTypeDef huart2;
extern   UART_HandleTypeDef huart3;

static srv_serial_485_data_t  serial_485;


srv_serial_485_ctx_t   * srv_serial_485_id2ctx(uint32_t id)
{
    switch(id)
    {
        case SRV_SERIAL_485_UxART1_ID: return &serial_485.u2;
        case SRV_SERIAL_485_UxART2_ID: return &serial_485.u3;
    }

    return NULL;
}


void    srv_serial_485_low_init(srv_serial_485_ctx_t * ctx,USART_TypeDef * usart,GPIO_TypeDef * gpio,uint16_t pin)
{
    ctx->usart = usart;
    ctx->gpio  = gpio;
    ctx->pin   = pin;

    HAL_GPIO_WritePin(gpio,pin,GPIO_PIN_RESET);

    vSemaphoreCreateBinary(ctx->sema);
}



void    srv_serial_485_enable(uint32_t port_id,int enable)
{
    srv_serial_485_ctx_t  * ctx;

    ctx = srv_serial_485_id2ctx(port_id);
    if(ctx== NULL)
    {
      fw_assert(0);
      return;
    }
    
    if(enable != 0)
    {
      ctx->usart->CR1 |=  USART_CR1_RXNEIE;
    }
    else
    {
      ctx->usart->CR1 &=  ~USART_CR1_RXNEIE;
    }

}


/*!
    \brief  Low level serial_485 port initialization

*/

void    srv_serial_485_init(void)
{

    memset(&serial_485,0,sizeof(serial_485));
    
   


}

void    srv_serial_485_once(void)
{

    // serial_485 ports already low level initialized by Cube


    /* Initialize port engines */

    srv_serial_485_low_init(&serial_485.u2,USART2,USART2_CTRL_GPIO_Port,USART2_CTRL_Pin);
    srv_serial_485_low_init(&serial_485.u3,USART3,USART3_CTRL_GPIO_Port,USART3_CTRL_Pin);



    /* Enable port interrupt */
    HAL_NVIC_SetPriority(USART2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    /* Enable port interrupt */
    HAL_NVIC_SetPriority(USART2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);



    // Note - interrupts must be enabled with call to srv_serial_485_enable()
}


void    srv_serial_485_rcv_callback(uint32_t port_id,serial_rec_485_char  rcv_char)
{
    srv_serial_485_ctx_t  * ctx;

    ctx = srv_serial_485_id2ctx(port_id);
    if(ctx== NULL)
    {
      fw_assert(0);
      return;
    }

    ctx->rcv_char = rcv_char;
}


/*!
    \brief  Low level serial_485 port interrupt
            Our own, hal version not suitable 

*/




void UARTx_485_IRQHandler(uint32_t id,srv_serial_485_ctx_t  * pctx)
{
  uint8_t          cc;
  portBASE_TYPE    hpt_woken1 = pdFALSE;  
  
  while( UF(pctx->usart,UART_FLAG_RXNE)  != RESET)
  {
    /* Read one byte from the receive data register */
    cc          = (uint8_t )(pctx->usart->RDR & 0x00FF);
    
    if(pctx->rcv_char != NULL)
    {
        pctx->rcv_char(id,cc,&hpt_woken1);
    }
  }

  if( UF(pctx->usart,UART_FLAG_TXE)  != RESET )
  { 
    
    /* Write one byte to the transmit data register */
    if(pctx->TxHead != pctx->TxTail)
    {
      pctx->usart->TDR  =  pctx->TxBuffer[pctx->TxTail] & 0x00FF;
      pctx->TxTail = ( pctx->TxTail + 1) % DIM(pctx->TxBuffer);
    }
    else
    {
      HAL_GPIO_WritePin(pctx->gpio,pctx->pin,GPIO_PIN_RESET);

      /* Disable the USART1 Transmit interrupt */
      pctx->usart->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);     
      xSemaphoreGiveFromISR(pctx->sema,&hpt_woken1); 
    }
  }

  portEND_SWITCHING_ISR(hpt_woken1);
}


void USART2_IRQHandler(void)
{
   UARTx_485_IRQHandler(SRV_SERIAL_485_UxART1_ID,&serial_485.u2);
}

void USART3_IRQHandler(void)
{
   UARTx_485_IRQHandler(SRV_SERIAL_485_UxART2_ID,&serial_485.u3);
}






static int srv_serial_485_send_cc(srv_serial_485_ctx_t  * ctx,char cc)
{
    uint32_t                new_head;

    new_head = (ctx->TxHead + 1) % DIM(ctx->TxBuffer);

    if(new_head == ctx->TxTail)
    {
        // no more room
       return -1;
    }
    ctx->TxBuffer[ctx->TxHead] = cc;
    ctx->TxHead = new_head;

    return 0;
}


/*!
    \brief  Low level serial_485 port send function (non blocking)

    \param buffer   Data buffer
    \param length   Data buffer size

*/

void    srv_serial_485_send(uint32_t port_id,const char * buffer,int length)
{
    int                     ii = 0;

    srv_serial_485_ctx_t  * ctx;

    ctx = srv_serial_485_id2ctx(port_id);
    if(ctx== NULL)
    {
      fw_assert(0);
      return;
    }

    ctx->usart->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);   


    if(length == 0)
    {
    	length = strlen(buffer);
    }

    HAL_GPIO_WritePin(ctx->gpio,ctx->pin,GPIO_PIN_SET);

    // Send extra dummy character (pre)
    srv_serial_485_send_cc(ctx,' ');

    // send remaining characters
    while(ii < length )
    {
        if(srv_serial_485_send_cc(ctx,buffer[ii++]) != 0)
        {
            // no more room
            break;
        }
    }    

    // Send extra dummy character (post)
    srv_serial_485_send_cc(ctx,' ');

    ctx->usart->CR1 |= (USART_CR1_TXEIE | USART_CR1_TCIE);    
    



}


/*!
    \brief  Low level serial_485 port send function (blocking)

    \param buffer   Data buffer
    \param length   Data buffer size

*/



void    srv_serial_485_puts(uint32_t port_id,const char * buffer,int length)
{
    srv_serial_485_ctx_t  * ctx;

    ctx = srv_serial_485_id2ctx(port_id);
    if(ctx== NULL)
    {
      fw_assert(0);
      return;
    }

    if(length == -1)
    {
      length = strlen(buffer);
    }


    /* Disable the UART Transmit Complete Interrupt */    
    ctx->usart->CR1 &= ~ (USART_CR1_TCIE | USART_CR1_TXEIE);

    // Clear current transaction - if any 
    ctx->TxHead = 0;
    ctx->TxTail = 0;

    HAL_GPIO_WritePin(ctx->gpio,ctx->pin,GPIO_PIN_SET);

    // Send the string - in blocking mode
    while(1)
    {
        while(UF(ctx->usart,UART_FLAG_TXE) != SET)
        {
          ;
        }

        if(length <= 0)
        {
          break;
        }

        ctx->usart->TDR = (*buffer & (uint8_t)0xFF);
        buffer++;
        length--;

    }    

    HAL_GPIO_WritePin(ctx->gpio,ctx->pin,GPIO_PIN_RESET);

}

/*!
    \brief  Low level serial_485 port send function. Blocks if there is transmission in progress

    \param buffer   Data buffer
    \param length   Data buffer size

*/
void    srv_serial_485_send_blocked(uint32_t port_id,const char * buffer,int length)
{
    srv_serial_485_ctx_t  * ctx;

    ctx = srv_serial_485_id2ctx(port_id);
    if(ctx== NULL)
    {
      fw_assert(0);
      return;
    }

    xSemaphoreTake(ctx->sema,portMAX_DELAY);
    srv_serial_485_send(port_id,buffer,length);
}

#include "serial_srv.h"


#define UF(inst_, flag_)  ( (((inst_)->SR & (flag_)) == (flag_) )? SET : RESET)



typedef struct
{   
    xSemaphoreHandle    sema;   
    
    uint8_t               TxBuffer[256];
    int                   TxHead;
    int                   TxTail;

    UART_HandleTypeDef  * handle;
    serial_rec_char       rec_char;

}serial_usart_ctx_t;


typedef struct
{   
    serial_usart_ctx_t u1;
    serial_usart_ctx_t u6;

}srv_serial_data_t;


extern             UART_HandleTypeDef huart1;
extern             UART_HandleTypeDef huart6;


srv_serial_data_t  serial_pt;



void    srv_serial_low_init(serial_usart_ctx_t * ctx,UART_HandleTypeDef * handle)
{
    memset(ctx,0,sizeof(*ctx));


    ctx->handle = handle;

    vSemaphoreCreateBinary(ctx->sema);
}

serial_usart_ctx_t   * srv_serial_id2ctx(uint32_t id)
{
    switch(id)
    {
        case SRV_SERIAL_UxART1_ID: return &serial_pt.u1;
        case SRV_SERIAL_UxART6_ID: return &serial_pt.u6;
    }

    return NULL;
}
void    srv_serial_baudrate(uint32_t port_id,uint32_t baudrate)
{
    serial_usart_ctx_t  * ctx;

    ctx = srv_serial_id2ctx(port_id);
    if(ctx== NULL)
    {
      return;
    }

    ctx->handle->Init.BaudRate = baudrate;
    HAL_UART_Init(ctx->handle);
}




/*!
    \brief  Low level serial_pt port initialization

*/

void    srv_serial_init(void)
{

    memset(&serial_pt,0,sizeof(serial_pt));
    
 

}

void    srv_serial_once(void)
{
    srv_serial_low_init(&serial_pt.u1,&huart1);
    srv_serial_low_init(&serial_pt.u6,&huart6);


   /* 
        Initialize port 
        Most already initialized by Cube
    */


    HAL_NVIC_SetPriority(USART1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    USART1->CR1 |=  USART_CR1_RXNEIE;


    HAL_NVIC_SetPriority(USART6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);

    USART6->CR1 |=  USART_CR1_RXNEIE;
}






void    srv_serial_rcv_callback(uint32_t port_id,serial_rec_char rec_char)
{
    serial_usart_ctx_t  * ctx;

    ctx = srv_serial_id2ctx(port_id);
    if(ctx== NULL)
    {
      return;
    }

    ctx->rec_char = rec_char;

}

/*!
    \brief  Low level serial_pt port interrupt
            Our own, hal version not suitable 

*/
void ATTRIBUTE_IN_RAM UARTx_IRQHandler(uint32_t id,serial_usart_ctx_t * pctx)
{
  uint8_t          cc;
  portBASE_TYPE    hpt_woken1 = pdFALSE;  
  

  if( UF(pctx->handle->Instance,USART_SR_RXNE)  != RESET)
  {
    /* Read one byte from the receive data register */
    cc          = (pctx->handle->Instance->DR & (uint8_t)0x00FF);
    pctx->rec_char(id,cc,&hpt_woken1);
  }

  if( UF(pctx->handle->Instance,UART_FLAG_TXE)  != RESET )
  { 
    
    /* Write one byte to the transmit data register */
    if(pctx->TxHead != pctx->TxTail)
    {
      pctx->handle->Instance->DR  =  pctx->TxBuffer[pctx->TxTail] & 0x00FF;
      pctx->TxTail = ( pctx->TxTail + 1) % DIM(pctx->TxBuffer);
      
    }
    else
    {
      /* Disable the USART1 Transmit interrupt */
      pctx->handle->Instance->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);     
      xSemaphoreGiveFromISR(pctx->sema,&hpt_woken1); 
    }
  }

  portEND_SWITCHING_ISR(hpt_woken1);
  
}

void ATTRIBUTE_IN_RAM USART1_IRQHandler(void)
{
    UARTx_IRQHandler(SRV_SERIAL_UxART1_ID,&serial_pt.u1);
}


void ATTRIBUTE_IN_RAM UART6_IRQHandler(void)
{
    UARTx_IRQHandler(SRV_SERIAL_UxART6_ID,&serial_pt.u6);
}




/*!
    \brief  Low level serial_pt port send function (non blocking)

    \param buffer   Data buffer
    \param length   Data buffer size

*/

void    srv_serial_send(uint32_t port_id,const char * buffer,int length)
{
    int                 ii = 0;
    uint32_t            new_head;
    serial_usart_ctx_t  * ctx;

    ctx = srv_serial_id2ctx(port_id);
    if(ctx== NULL)
    {
      return;
    }

    ctx->handle->Instance->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);   

    while(ii < length )
    {
        new_head = (ctx->TxHead + 1) % DIM(ctx->TxBuffer);

        if(new_head == ctx->TxTail)
        {
            // no more room
            break;
        }
        
        ctx->TxBuffer[ctx->TxHead] = buffer[ii++];
        ctx->TxHead = new_head;       
    }    

    ctx->handle->Instance->CR1 |= (USART_CR1_TXEIE | USART_CR1_TCIE);    
    
}





/*!
    \brief  Low level serial_pt port send function (blocking)

    \param buffer   Data buffer
    \param length   Data buffer size

*/



void    srv_serial_puts(uint32_t port_id,const char * buffer,int length)
{
    serial_usart_ctx_t  * ctx;

    ctx = srv_serial_id2ctx(port_id);
    if(ctx== NULL)
    {
      return;
    }

    if(length <0)
    {
      length = strlen(buffer);
    }



    /* Disable the UART Transmit Complete Interrupt */    
    ctx->handle->Instance->CR1 &= ~ (USART_CR1_TCIE | USART_CR1_TXEIE);

    // Clear current transaction - if any 
    ctx->TxHead = 0;
    ctx->TxTail = 0;

    // Send the string - in blocking mode
    while(length > 0)
    {
        while(UF(ctx->handle->Instance,USART_SR_TXE) != SET)
        {
          ;
        }

        ctx->handle->Instance->DR = (*buffer & (uint8_t)0xFF);
        buffer++;
        length--;
    }               
}


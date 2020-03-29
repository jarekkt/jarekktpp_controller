 /*!
    \file cancommt_mwr.c
*/


#include "services.h"
#include "middleware.h"
#include "cancomm_task_can1.h"
#include "cancomm_prv.h"

#define  MWR_CANCOMM_RING_SIZE         10


typedef struct
{  

   CAN_HandleTypeDef  *   CanHandle;

   CAN_RxHeaderTypeDef    RxHeader;
   uint8_t                RxData[8];

   uint32_t               TxMailbox;
   volatile uint32_t      TxMailboxComplete[3];

   uint32_t               rcv_cntr;
   uint32_t               err_cntr;
   uint32_t               txc_cntr;
   uint32_t               ovrerr_cntr;

   uint32_t               test_mask;



   can_frame_t            cring[MWR_CANCOMM_RING_SIZE];
   uint32_t               cring_tail;
   uint32_t               cring_head;

  
}mwr_cancomm_channel_t;

typedef struct
{
    mwr_cancomm_channel_t  canCh[1];
    uint32_t               can_test;
}mwr_cancomm_type;


mwr_cancomm_type      ccm;


const var_ptable_t   mwr_cancomm_var_ptable[] SERMON_ATTR =  
{     
  { "can_test",             &ccm.can_test             ,E_VA_INT_FREE}
};


extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

static void mwr_cancomm_task_can1(void * params);


static void mwr_cancomm_config(void)
{
  CAN_FilterTypeDef  sFilterConfig;

  
  /*
  *   CAN1 channel
  */

  ccm.canCh[0].CanHandle = &hcan1;
  ccm.canCh[0].CanHandle->Instance                  = CAN1;
  ccm.canCh[0].CanHandle->Init.TimeTriggeredMode    = DISABLE;
  ccm.canCh[0].CanHandle->Init.AutoBusOff           = ENABLE;
  ccm.canCh[0].CanHandle->Init.AutoWakeUp           = DISABLE;
  ccm.canCh[0].CanHandle->Init.AutoRetransmission   = ENABLE;
  ccm.canCh[0].CanHandle->Init.ReceiveFifoLocked    = DISABLE;
  ccm.canCh[0].CanHandle->Init.TransmitFifoPriority = DISABLE;
  ccm.canCh[0].CanHandle->Init.Mode                 = CAN_MODE_NORMAL;

  // See http://www.bittiming.can-wiki.info/
  // CAN clock is 22.5MHz

  ccm.canCh[0].CanHandle->Init.SyncJumpWidth        = CAN_SJW_1TQ;
  ccm.canCh[0].CanHandle->Init.TimeSeg1             = CAN_BS1_12TQ;
  ccm.canCh[0].CanHandle->Init.TimeSeg2             = CAN_BS2_2TQ;
  ccm.canCh[0].CanHandle->Init.Prescaler            = 6;


  if (HAL_CAN_Init(ccm.canCh[0].CanHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }


  sFilterConfig.FilterBank            = 0;
  sFilterConfig.FilterMode            = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale           = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh          = 0x0000;
  sFilterConfig.FilterIdLow           = 0x0000;
  sFilterConfig.FilterMaskIdHigh      = 0x0000;
  sFilterConfig.FilterMaskIdLow       = 0x0000;
  sFilterConfig.FilterFIFOAssignment  = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation      = ENABLE;
  sFilterConfig.SlaveStartFilterBank  = 0;


  if (HAL_CAN_ConfigFilter(ccm.canCh[0].CanHandle, &sFilterConfig) != HAL_OK)
  {
    /* Filter configuration Error */
    Error_Handler();
  }

  
  if (HAL_CAN_Start(ccm.canCh[0].CanHandle) != HAL_OK)
  {
    /* Start Error */
    Error_Handler();
  }

  if (HAL_CAN_ActivateNotification(ccm.canCh[0].CanHandle, CAN_IT_RX_FIFO0_MSG_PENDING|CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
  {
    /* Notification Error */
    Error_Handler();
  }

  GPIO_Clr(CAN1_ENA); // LBL low
  GPIO_Clr(CAN1_ENA); // Slope control enabled


  
}

/*!
    \brief Can communication init function
    
*/

void mwr_cancomm_init(void)
{
    memset(&ccm,0,sizeof(ccm));


    mwr_cancomm_config();

    mwr_cancomm_init_can1_task();


    srv_sermon_register(mwr_cancomm_var_ptable,DIM(mwr_cancomm_var_ptable));

}




void mwr_cancomm_once(void)
{
    xTaskCreate( mwr_cancomm_task_can1,   "Can1",  3*configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY   + 1, NULL );
}


void mwr_cancomm_test(uint32_t ch_idx)
{
    CAN_TxHeaderTypeDef     TxHeader;
    uint8_t                 TxData[8];
    mwr_cancomm_channel_t * ch;
    uint8_t                chIdx;

    if(ch_idx != 0)
    {
           ch    = &ccm.canCh[1];
           chIdx = 0x20;
    }
    else
    {
    	  return;
    }

  
    TxHeader.RTR                = CAN_RTR_DATA;
    TxHeader.IDE                = CAN_ID_EXT;
    TxHeader.TransmitGlobalTime = DISABLE;
    TxHeader.ExtId              = 0x321;
    
    TxData[0]     = 0xDE;
    TxData[1]     = 0xAD;
    TxData[2]     = 0;
    TxData[3]     = ch->ovrerr_cntr;
    TxData[4]     = ch->rcv_cntr;
    TxData[5]     = ch->err_cntr;
    TxData[6]     = chIdx;
    TxHeader.DLC  = 7; 
          
    if(HAL_CAN_IsTxMessagePending(ch->CanHandle,ch->TxMailbox) != 0)
    {
      HAL_CAN_AbortTxRequest(ch->CanHandle,ch->TxMailbox);
    }

    if(HAL_CAN_AddTxMessage(ch->CanHandle,&TxHeader,TxData,&ch->TxMailbox) != HAL_OK)
    {
    
    }
   
}

void mwr_cancomm_send(uint32_t cidx,can_frame_t * raw_frame,uint32_t wait4ready)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t            tout;
    mwr_cancomm_channel_t * ch;

    if(cidx == 0)
    {
      ch =  &ccm.canCh[0];
      ccm.can_test |= 0x01;
    }
    else 
    {
      ch =  &ccm.canCh[1];
      ccm.can_test |= 0x02;
    }    

    TxHeader.TransmitGlobalTime = DISABLE;
    TxHeader.RTR                = CAN_RTR_DATA;
    TxHeader.DLC                = raw_frame->can_dlc;       


    if( (raw_frame->can_id & (1<<31)) != 0)
    {
        TxHeader.IDE     = CAN_ID_EXT;
        TxHeader.ExtId   = raw_frame->can_id;
    }
    else
    {
        TxHeader.IDE     = CAN_ID_STD;
        TxHeader.StdId   = raw_frame->can_id;
    }

    tout = 10;
    while( HAL_CAN_GetTxMailboxesFreeLevel(ch->CanHandle) == 0)
    {
        vTaskDelay(1);
        tout--;
        if(tout == 0)
        {
           return;
        }
    }

    memset((void*)ch->TxMailboxComplete,0,sizeof(ch->TxMailboxComplete));
    
    if(HAL_CAN_AddTxMessage(ch->CanHandle,&TxHeader,raw_frame->data,&ch->TxMailbox) != HAL_OK)
    {
        ch->txc_cntr++;
    }
    else
    {
      if(wait4ready != 0)
      {
          int32_t idx = -1;

          switch(ch->TxMailbox)
          {
              case 1: idx = 0;break;
              case 2: idx = 1;break;
              case 4: idx = 2;break;
          }

          if(idx >= 0)
          {
            tout = 10;
            while(ch->TxMailboxComplete[idx] == 0)
            {
                vTaskDelay(1);
                tout--;
                if(tout == 0)
                {
                   return;
                }
            }
          }
      }
    }
    
}

/*!
    \brief 
    
*/
static void mwr_cancomm_task_can1(void * params)
{
    //can_frame_t raw_frame;

    if (HAL_CAN_ActivateNotification(ccm.canCh[0].CanHandle, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
      /* Notification Error */
      Error_Handler();
    }

    mwr_cancomm_execute_can1_task();   

}



int mwr_cancomm_receive(uint32_t cidx,can_frame_t * raw_frame)
{
    mwr_cancomm_channel_t * ch;

    if(cidx == 0)
    {
     ch =  &ccm.canCh[0];
     
    }
    else 
    {
       return -1;
    }    

   if(ch->cring_head == ch->cring_tail)
   {
      // No frames available
      return -1;
   }
   else
   {
      memcpy(raw_frame,&ch->cring[ch->cring_tail],sizeof(*raw_frame));

      ch->cring_tail = (ch->cring_tail + 1)%MWR_CANCOMM_RING_SIZE;

      return 0;
   }
}




void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  uint32_t                new_head;
  mwr_cancomm_channel_t * ch;

  if(hcan == &hcan1)
  {
     ch =  &ccm.canCh[0];
  }
  else //hcan2
  {
     return;
  }

  ch->rcv_cntr++; 

  /* Get RX message */
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &ch->RxHeader, ch->RxData) != HAL_OK)
  {
      ch->err_cntr++;
  }
  else
  {
      new_head = (ch->cring_head + 1)%MWR_CANCOMM_RING_SIZE;

      if(new_head != ch->cring_tail)
      {
          can_frame_t * fr;

          fr = &ch->cring[ch->cring_head];

          fr->can_dlc = ch->RxHeader.DLC;

          if(ch->RxHeader.IDE == CAN_ID_STD)
          {
            fr->can_id  = ch->RxHeader.StdId;
          }
          else
          {
            fr->can_id  = ch->RxHeader.ExtId;
          }
          
          fw_assert(ch->RxHeader.DLC <= sizeof(fr->data));
          memcpy(fr->data,ch->RxData,fr->can_dlc);

          ch->cring_head = new_head;
      }
      else
      {
        ch->ovrerr_cntr++;
      }

      if(hcan == &hcan1)
      {
         mwr_cancomm_callback_can1();
      }
      else //hcan2
      {

      }
  }  
}


void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
  if(hcan == &hcan1)
  {
      ccm.canCh[0].TxMailboxComplete[0]++;
  }
  else //hcan2
  {

  }
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
  if(hcan == &hcan1)
  {
      ccm.canCh[0].TxMailboxComplete[1]++;
  }
  else //hcan2
  {

  }
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
  if(hcan == &hcan1)
  {
      ccm.canCh[0].TxMailboxComplete[2]++;
  }
  else //hcan2
  {

  }
}




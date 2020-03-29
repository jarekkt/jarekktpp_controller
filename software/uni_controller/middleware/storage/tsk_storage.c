/*!
    \file tsk_storage.c

    \brief This file process handles non volatile memory storage


*/


#include "system.h"
#include "services.h"
#include "middleware.h"


#define STORAGE_INTERVAL      (7/*minutes*/ *60 /*sec*/ * 1000/* ms*/)
#define STORAGE_DUMP_CHANGE   0
#define STORAGE_CAL_ITEM      (64/sizeof(uint32_t))

typedef struct
{

   xSemaphoreHandle    wait_sema;
   xSemaphoreHandle    mutex_sema;  
    
   int                 delete_storage; 
   int                 run_cntr;
   int                 run_shadow;
   
   union
   {
      uint32_t  dwords[STORAGE_CAL_ITEM];
      uint8_t   buffer[STORAGE_CAL_ITEM*sizeof(uint32_t)];
   }cal_params;


   char                var_name[SYS_VAR_NAME_LENGTH];
   char                var_idx[16];
   char                var_val[16];
   char                var_crc[4];

   
}storage_task_ctx_t;


void tsk_storage_task(void * params);


storage_task_ctx_t    sctx;


/*!
    \brief This function activates storage ( in non blocking way).
           All changed data will be updated in NOV in the backgoround.

*/
void tsk_storage_activate(void)
{
    xSemaphoreGive(sctx.wait_sema);    
}

void tsk_storage_shadow(uint32_t mode)
{
    sctx.run_shadow = mode;
    tsk_storage_activate();
}


void tsk_storage_shadow_execute(void)
{
    uint32_t            ii;
    uint32_t            var_idx = 0;
    uint32_t            cnt;
    var_caltable_t    * ct;
    uint32_t            len;
    uint32_t            jj;
    uint8_t             crc;

    srv_nov_store_shadow_init();

    do
    {
      ct = srv_nov_store_shadow_get(var_idx++);
      if(ct == NULL)
      {
        break;
      }

      switch(ct->basetype)
      {
          case BT_FLT:  cnt = ct->size / sizeof(float);     break;
          case BT_I32:  cnt = ct->size / sizeof(int32_t);   break;
          case BT_U32:  cnt = ct->size / sizeof(uint32_t);  break;
          case BT_I8:   cnt = ct->size / sizeof(int8_t);    break;
          case BT_U8:   cnt = ct->size / sizeof(uint8_t);   break;   
          default:      cnt = 0; break;
      }

      for(ii = 0; ii < cnt;ii++)
      {
          switch(ct->basetype)
          {
              case BT_FLT:  
              {
                len = snprintf(sctx.cal_params.buffer,sizeof(sctx.cal_params.buffer)-4,
                              "%s %3ld %12.6f ",ct->name,ii,((float*)ct->ptr)[ii]);
              }break;

              case BT_I32:
              {
                len = snprintf(sctx.cal_params.buffer,sizeof(sctx.cal_params.buffer)-4,
                              "%s %3ld %ld ",ct->name,ii,((int32_t*)ct->ptr)[ii]);
              }break;

              case BT_U32:
              {
                len = snprintf(sctx.cal_params.buffer,sizeof(sctx.cal_params.buffer)-4,
                              "%s %3ld %lu ",ct->name,ii,((uint32_t*)ct->ptr)[ii]);
              }break;

              case BT_I8: 
              {
                len = snprintf(sctx.cal_params.buffer,sizeof(sctx.cal_params.buffer)-4,
                              "%s %3ld %d ",ct->name,ii,((int8_t*)ct->ptr)[ii]);

              }break;

              case BT_U8: 
              {
                len = snprintf(sctx.cal_params.buffer,sizeof(sctx.cal_params.buffer)-4,
                              "%s %3ld %d ",ct->name,ii,((uint8_t*)ct->ptr)[ii]);
              }break;

              default:    
              {
                len = 0;
              }break;
          }

          if(len > 0)
          {
              // Align to 4 bytes
              while( (len % 4) != 1 )
              {
                sctx.cal_params.buffer[len++]=' ';
              }

              crc = 0;
              for(jj=0;jj< len;jj++)
              {
                crc = crc^sctx.cal_params.buffer[jj];
              }
         
              len += snprintf(&sctx.cal_params.buffer[jj],sizeof(sctx.cal_params.buffer)-jj-1,"%02X",crc);                
              sctx.cal_params.buffer[len++]='\n';

              srv_nov_store_shadow_add(sctx.cal_params.dwords,len / sizeof(uint32_t));
          }
       }
     }while(1);
}


int    tsk_split_line(const char * buffer, uint32_t * idx,uint32_t len_buffer,char * target_buffer,uint32_t target_size)
{
    int len_out = 0;
    
    while( (buffer[*idx] == ' ') && ( len_buffer > *idx))
    {
      (*idx)++;
    }

    while( (buffer[*idx] != ' ') &&  (buffer[*idx] != '\n')  && (buffer[*idx] != 0) && ( len_buffer > *idx))
    {
      if(len_out <  target_size - 1) 
      {
        target_buffer[len_out] = buffer[*idx];
        len_out++;
        (*idx)++;
      }
      else
      {
        return 0;
      }
    }

    target_buffer[len_out] = 0;

    return len_out;

}



int32_t tsk_storage_process_cal(const char * line,uint32_t len)
{
    uint32_t            idx = 0;
    uint32_t            ct_idx = 0;
    var_caltable_t    * ct;
    uint32_t            jj;
    uint8_t             crc;
    

    float               val_fl;
    int32_t             val_i32;
    uint32_t            val_u32;
    uint32_t            val_idx;

    


    // Split line

    if(tsk_split_line(line,&idx,len,sctx.var_name,sizeof(sctx.var_name)) ==0 )
    {
       return -1;
    }
   
    if(tsk_split_line(line,&idx,len,sctx.var_idx,sizeof(sctx.var_idx)) ==0 )
    {
       return -1;
    }

    if(tsk_split_line(line,&idx,len,sctx.var_val,sizeof(sctx.var_val)) ==0 )
    {
       return -1;
    }

    if(tsk_split_line(line,&idx,len,sctx.var_crc,sizeof(sctx.var_crc)) ==0 )
    {
       return -1;
    }

    // Check crc
    crc = 0;
    for(jj=0;jj< len-3;jj++)
    {
       crc = crc^line[jj];
    }

    if( crc != strtoul(sctx.var_crc,NULL,16) )
    {
        return -1;
    }

    do
    {
      ct = srv_nov_store_shadow_get(ct_idx++);
      if(ct == NULL)
      {
        return -1;
      }

      if( strcmp(sctx.var_name,ct->name) == 0)
      {
          // Found
          val_idx = strtoul(sctx.var_idx,NULL,10);

          if(val_idx > ct->size)
          {
             return -1;
          }

          switch(ct->basetype)
          {
              case BT_FLT:  
              {
                  val_fl = strtof(sctx.var_val,NULL);
                  ((float*)ct->ptr)[val_idx] = val_fl;
              }break;

              case BT_I32:
              {
                  val_i32 = strtol(sctx.var_val,NULL,10);
                  ((int32_t*)ct->ptr)[val_idx] = val_i32;                     
              }break;

              case BT_U32:
              {
                  val_u32 = strtol(sctx.var_val,NULL,10);
                  ((uint32_t*)ct->ptr)[val_idx] = val_u32;                     
              }break;

              case BT_I8: 
              {
                  val_i32 = strtol(sctx.var_val,NULL,10);
                  ((int8_t*)ct->ptr)[val_idx] = (int8_t)val_i32;                     
              }break;

              case BT_U8: 
              {
                  val_u32 = strtol(sctx.var_val,NULL,10);
                  ((uint8_t*)ct->ptr)[val_idx] = (uint8_t)val_u32;                     
              }break;

              default:    
              {
              }break;
          }

          break;
      }       
    }while(1);


    return 0;
}


/*!
    \brief Storage task intialization function

*/
void tsk_storage_init(void)
{
    memset(&sctx,0,sizeof(sctx));
      
}

void tsk_storage_once(void)
{
  vSemaphoreCreateBinary(sctx.wait_sema);        
  sctx.mutex_sema = xSemaphoreCreateMutex();

  xTaskCreate( tsk_storage_task, ( signed char * ) "Store", 8* configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
}



/*!
    \brief This function activates storage ( in blocking way).
           It will not return until all data is stored.

*/
void tsk_storage_execute(void)
{
   int result;


   
   xSemaphoreTake(sctx.mutex_sema,portMAX_DELAY);
   
   if( (srv_nov_is_changed()!= 0)|| (sctx.delete_storage != 0))
   {
#if STORAGE_DUMP_CHANGE != 0
       srv_nov_dump_change();
#endif
     
       result = srv_nov_store(sctx.delete_storage);
       sctx.delete_storage = 0;

       if(result != 0)
       {
         burst_log_printf(1,"nov: storage error!\r\n");
       }

       sctx.run_cntr++;
    }

    if(sctx.run_shadow != 0)
    {
       switch(sctx.run_shadow)
       {
           case 1:
           {
                srv_nov_store_shadow_binary();
                burst_log_printf(1,"nov: storage shadow executed (binary  mode)\r\n");
           }break;
 
           default:
           {
                tsk_storage_shadow_execute();
                burst_log_printf(1,"nov: storage shadow executed (calibrate mode)\r\n");
           }break;  
        }
       
       

       sctx.run_shadow = 0;
    }
  
    xSemaphoreGive(sctx.mutex_sema);
}


/*!
    \brief Storage task. Peridically stores data to NOV ( with #STORAGE_INTERVAL interval) or by explicit request.

*/

void tsk_storage_task(void * params)
{


    // Forced storage check during startup - to be sure that we do not loose anything 
    // which might have been changed during init phase ( e.g. supervison or test results)


    /*!
        \sto  Storage activation once when staring storage task - to store all changes from startup phase
    */

    
    srv_nov_start();

    tsk_storage_activate();

    while(1)
    {

        xSemaphoreTake(sctx.wait_sema, STORAGE_INTERVAL / portTICK_RATE_MS);

        // Ignore result - either it was activation or timeout
        tsk_storage_execute();
        
        fw_stack_check();
    }
}


void tsk_storage_kill(void)
{
   burst_log_printf(1,"\r\nnov: killing storage\r\n");

   sctx.delete_storage = 0x01|0x02;

   tsk_storage_activate();

}

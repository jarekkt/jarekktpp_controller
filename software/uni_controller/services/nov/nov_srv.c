/*! \file   srv_nov.c

    \brief  Low level NOV support
    
    
*/ 

#include "system.h" 
#include "services.h"
#include "middleware.h"
 



/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */


extern const char      __storage1_start__[];
extern const char      __storage1_end__[];
extern const uint32_t  __storage1_size__;

extern const char      __storage2_start__[];
extern const char      __storage2_end__[];
extern const uint32_t  __storage2_size__;


extern const char      __storage_shadow_start__[];
extern const char      __storage_shadow_end__[];


extern const char      __caltable_start__[];
extern const char      __caltable_end__[];
extern const uint32_t  __caltable_size__;


#define   STORE_FIRST_COPY_1    0
#define   STORE_FIRST_COPY_2    1


typedef struct
{
    uint32_t start;
    uint32_t size;
}flash_sect_t;


const flash_sect_t  flash_layout[] =
{ 
    { ADDR_FLASH_SECTOR_0, 16*1024 },
    { ADDR_FLASH_SECTOR_1, 16*1024 },
    { ADDR_FLASH_SECTOR_2, 16*1024 },
    { ADDR_FLASH_SECTOR_3, 16*1024 },
    { ADDR_FLASH_SECTOR_4, 64*1024 },
    { ADDR_FLASH_SECTOR_5, 128*1024 },
    { ADDR_FLASH_SECTOR_6, 128*1024 },
    { ADDR_FLASH_SECTOR_7, 128*1024 },
    { ADDR_FLASH_SECTOR_8, 128*1024 },
    { ADDR_FLASH_SECTOR_9, 128*1024 },
    { ADDR_FLASH_SECTOR_10, 128*1024 },
    { ADDR_FLASH_SECTOR_11, 128*1024 }
};

typedef void (* default_fn)(void);
typedef void (* idle_fn)(void);

static default_fn  default_functions[64];
static uint32_t    default_functions_cnt = 0; 
static idle_fn     idle_function = NULL;
static int         done = 0; 
static int32_t     cal_table_offset = -1;
const  char *      cal_table_copy = __storage_shadow_start__;
static uint32_t    cal_recovered = 0;
static uint32_t    cal_faulty    = 0;
static uint32_t    destroyed     = 0;
static uint32_t    store_first   = STORE_FIRST_COPY_1;




#define cal_table          ((var_caltable_t*)__caltable_start__)
#define cal_table_size     ((uint32_t)(__caltable_end__ - __caltable_start__))



uint32_t   nov_block_masks[3];


static int srv_nov_store_copy(const char * copy,const char * src,uint32_t size,int destroy_nov);


/*! Table of CRC values for highorder byte */
static unsigned char auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40
} ;

/*! Table of CRC values for loworder byte */
static char auchCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
0x40
} ;


typedef struct
{
    uint32_t nvCounter;
    uint32_t serial_nr;
}srv_nov_nv_t;


srv_nov_nv_t     srv_nov_nv VAR_NV_ATTR;
static   int     state = 0;


/*!

    \brief  Standard CRC16 routine

    \param [in] puchMsg character stream
    \param [in] usDataLen stream length

    \retval CRC16 value of given stream

    
*/

static uint16_t crc16(const unsigned char *puchMsg,uint16_t usDataLen)
{
    uint8_t uchCRCHi = 0xFF ; /* high byte of CRC initialized */
    uint8_t uchCRCLo = 0xFF ; /* low byte of CRC initialized */
    int uIndex ;             /* will index into CRC lookup table */
    
    while (usDataLen--)
    {
        uIndex = uchCRCHi ^ *puchMsg++ ; /* calculate the CRC */
        uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
        uchCRCLo = auchCRCLo[uIndex] ;
    }
    return (uchCRCHi << 8 | uchCRCLo) ;
}




/*!

    \brief  Reads NOV storage from EEPROM


    \retval 0 if succesfull NOV retrival was performed

    Note: Test mode is not preserved across resets (test_mode variable is always cleared).
          Emulation mode however persists ( if, after enabling emulation, the reset was executed by serial command)


    \page nov  Non volatile storage

    The Nov block is responsible for non volatile storage of device data.
    There are two types of data Nov block stores in EEPROM:
    - Device parameters
        - The parameters are stored in two identical copies. 
            - During startup, first copy is read and checked for proper checksum ( crc 16). If valid, these parameters are used.
              If the copy is not valid, second copy is used. If this one is faulty - the it is fatal error, default values are used.
            - When write request arrives, all parameters are written at once. Starting from second copy, then the first copy 
              ( so there is guarantee always one copy is valid even if power cuts in random moment ). 
              We start from second copy as we know that first one was valid (we do not have such guarantee about second) 


              
*/

int  srv_nov_read_flash(void)
{
    uint8_t              result = 1;
    uint32_t             crc;
    uint32_t             dwords;
    uint32_t             r_crc;
    uint32_t             valid_copy = 0;


     
     
    dwords = ( __var_nv_size__+ sizeof(uint32_t) -1)/sizeof(uint32_t);
    
    state = 0;
   
    // Try first copy
    crc  = crc16(__storage1_start__,__var_nv_size__);
    r_crc = ((uint32_t *)__storage1_start__)[dwords];

    if(crc == r_crc)
    {
        // Copy valid
        memcpy(__var_nv_start__,__storage1_start__,__var_nv_size__);
        result = 0;
        state |= 0x01;

        valid_copy++;
    }
    else
    {
        state |= 0x02;
    }


    // Try backup copy
    crc   = crc16(__storage2_start__,__var_nv_size__);
    r_crc = ((uint32_t *)__storage2_start__)[dwords];

    if(crc == r_crc)
    {
        state |= 0x04;
        valid_copy++;

        if(result != 0)
        {
            state |= 0x08;
            // Only second copy valid - restore first copy to guarantee that the first one is always valid
            memcpy(__var_nv_start__,__storage2_start__,__var_nv_size__);
            store_first = STORE_FIRST_COPY_1;
            result      = 0;
        }
    }
    else
    {

        state |= 0x10;
        if(result == 0)
        {
            state |= 0x20;
            // First was valid, second corrupted. Restore second.
            store_first = STORE_FIRST_COPY_2;
        }        
    }

    return result;    
}


void srv_nov_init_default(int mode)
{
    uint32_t  sn;
    uint32_t  nvCounter;

    if( (mode & 0x01) == 0) 
    {
          /* Not a fresh invalid NV storage */  
          sn        = srv_nov_nv.serial_nr;
          nvCounter = srv_nov_nv.nvCounter;      
    }
    else
    {
          /* Unknown previous state */
          sn          = 0;
          nvCounter   = 0;
    }


    memset(&srv_nov_nv,0,sizeof(srv_nov_nv));


    // Restore some vital variables
    srv_nov_nv.serial_nr = sn;
    srv_nov_nv.nvCounter = nvCounter;  

}

int32_t srv_nov_store_shadow_binary_read() 
{
   int32_t     result = -1;
   uint32_t    crc;
   uint32_t    r_crc;
   uint32_t             dwords;
    
         
   dwords = ( __var_nv_size__+ sizeof(uint32_t) -1)/sizeof(uint32_t);

   // Binary mode
   crc   = crc16(__storage_shadow_start__,__var_nv_size__);
   r_crc = ((uint32_t *)__storage_shadow_start__)[dwords];

   if(crc == r_crc)
   {
      memcpy(__var_nv_start__,__storage_shadow_start__,__var_nv_size__);

      result = 0;

      state |= 0x80;
   }

   return result;
}



int  srv_nov_init(void)
{
   int status;
   int ii;


   status = srv_nov_read_flash();

   if(status != 0)
   {
      for(ii = 0; ii < default_functions_cnt;ii++)
      {
          default_functions[ii]();
      }
      srv_nov_init_default(1);


      // Try to look for shadow      
      if(srv_nov_store_shadow_read() == 0)
      {
          // Calibration text mode
          state |= 0x40;
      }
      else if(srv_nov_store_shadow_binary_read() == 0)
      {
          // Binary full mode
          state |= 0x80;
      }
   
   }

   done = 1;

   return status;
}




/*!

    \brief  Function which prints nov status - acquired when logging was not available yet


    \param  state  Status set by #srv_nov_init()
*/    


void  srv_nov_print_info(srv_printf_t pprintf)
{
    if(state& 0x01)(*pprintf)(1,"nov: first  copy OK\r\n");
    if(state& 0x02)(*pprintf)(1,"nov: first  copy faulty\r\n");
    if(state& 0x04)(*pprintf)(1,"nov: second copy OK\r\n");
    if(state& 0x08)(*pprintf)(1,"nov: first  copy recovery\r\n");
    if(state& 0x10)(*pprintf)(1,"nov: second copy faulty\r\n");
    if(state& 0x20)(*pprintf)(1,"nov: second copy recovery\r\n");
    if(state& 0x40)(*pprintf)(1,"nov: shadow copy(text cal mode) recovery ( Ok:%d Faulty: %d)\r\n",cal_recovered,cal_faulty);
    if(state& 0x80)(*pprintf)(1,"nov: shadow copy(binary mode) recovery\r\n");
}





#define SRV_FLASH_TIMEOUT 50000



static int32_t ATTRIBUTE_IN_RAM srv_FLASH_WaitForLastOperation(uint32_t Timeout)
{ 
  srv_hwio_timestamp_t tickstart = 0U;

  
  /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
     Even if the FLASH operation fails, the BUSY flag will be reset and an error
     flag will be set */

  /* Get tick */
  tickstart = srv_hwio_timestamp_ms();

  while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET) 
  { 
      if((srv_hwio_timestamp_ms() - tickstart ) > Timeout)
      {
        return -1;
      }     

      if(idle_function != NULL)
      {
         idle_function(); 
      }
  }

  /* Check FLASH End of Operation flag  */
  if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP) != RESET)
  {
    /* Clear FLASH End of Operation pending bit */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
  }

  if(__HAL_FLASH_GET_FLAG((FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR | FLASH_FLAG_RDERR)) != RESET)
  {
    return -1;
  }

  /* If there is no error flag set */
  return 0;
  
} 

static int32_t ATTRIBUTE_IN_RAM srv_FLASH_Unlock(void)
{
  uint32_t status = HAL_OK;

  if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
  {
    /* Authorize the FLASH Registers access */
    WRITE_REG(FLASH->KEYR, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR, FLASH_KEY2);

    /* Verify Flash is unlocked */
    if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
    {
      status = -1;
    }
  }
  return status;
}

static void ATTRIBUTE_IN_RAM srv_FLASH_Lock(void)
{
  /* Set the LOCK Bit to lock the FLASH Registers access */
  FLASH->CR |= FLASH_CR_LOCK;
}



static int32_t  ATTRIBUTE_IN_RAM srv_FLASH_Erase_Sector(uint32_t Sector, uint8_t VoltageRange)
{
  uint32_t tmp_psize = 0U;
  int32_t  status;

  /* Check the parameters */
  assert_param(IS_FLASH_SECTOR(Sector));
  assert_param(IS_VOLTAGERANGE(VoltageRange));
  


  if(VoltageRange == FLASH_VOLTAGE_RANGE_1)
  {
     tmp_psize = FLASH_PSIZE_BYTE;
  }
  else if(VoltageRange == FLASH_VOLTAGE_RANGE_2)
  {
    tmp_psize = FLASH_PSIZE_HALF_WORD;
  }
  else if(VoltageRange == FLASH_VOLTAGE_RANGE_3)
  {
    tmp_psize = FLASH_PSIZE_WORD;
  }
  else
  {
    tmp_psize = FLASH_PSIZE_DOUBLE_WORD;
  }

  /* Need to add offset of 4 when sector higher than FLASH_SECTOR_11 */
  if(Sector > FLASH_SECTOR_11) 
  {
    Sector += 4U;
  }
  /* If the previous operation is completed, proceed to erase the sector */
  CLEAR_BIT(FLASH->CR, FLASH_CR_PSIZE);
  FLASH->CR |= tmp_psize;
  CLEAR_BIT(FLASH->CR, FLASH_CR_SNB);
  FLASH->CR |= FLASH_CR_SER | (Sector << FLASH_CR_SNB_Pos);
  FLASH->CR |= FLASH_CR_STRT;


  status = srv_FLASH_WaitForLastOperation((uint32_t)SRV_FLASH_TIMEOUT);

  /* If the erase operation is completed, disable the SER and SNB Bits */
  CLEAR_BIT(FLASH->CR, (FLASH_CR_SER | FLASH_CR_SNB));
  FLASH_FlushCaches(); 

  return status;

 
}

static int32_t ATTRIBUTE_IN_RAM  srv_FLASH_Program_Word(uint32_t Address, uint32_t Data)
{
  
  uint32_t status = HAL_OK;

    
  /* If the previous operation is completed, proceed to program the new data */
  CLEAR_BIT(FLASH->CR, FLASH_CR_PSIZE);
  FLASH->CR |= FLASH_PSIZE_WORD;
  FLASH->CR |= FLASH_CR_PG;
  *(__IO uint32_t*)Address = Data;

  /* Wait for last operation to be completed */
  status = srv_FLASH_WaitForLastOperation((uint32_t)SRV_FLASH_TIMEOUT);
    
  /* If the program operation is completed, disable the PG Bit */
  FLASH->CR &= (~FLASH_CR_PG);  


  return status;
}


static int ATTRIBUTE_IN_RAM srv_nov_store_copy(const char * copy,const char *src,uint32_t size,int destroy_nov)
{
     
    int                     ii;
    int                     sector = -1;    
    int                     result = 0;
    uint32_t                crc;
    uint32_t                status;

    int                     dwords;



   


    if(destroyed )
    {
       // keep it this way and do not touch flash anymore
       return 0;
    }



     /*!
    
        \wdg  Watchdog kicks - Before each NOV storage operations - once, as erase may be 
              relatively ( few miliseconds).
                           
     */
     
    srv_wdg_kick();

    for(ii = 0; ii < DIM(flash_layout);ii++)
    {
        if( (uint32_t)copy == flash_layout[ii].start )
        {
            sector = ii;
            break;
        }
    }


    if( sector < 0) 
    {
        // Not found 
        return -1;
    }

    if(size > flash_layout[sector].size)
    {
        // too big
        return -1;
    }

    __disable_irq();


    /*!
            \tst  Variable 'nvCounter' Incremented each time NV storage contents is written to the flash.
                                       Note that it is written for each NV copy, so each update will increment it by factor 2.        
    */ 

    srv_nov_nv.nvCounter ++;
    crc = crc16(src,size);
    if(destroy_nov !=0)
    {
      crc = ~crc;
    }
    
    srv_FLASH_Unlock();      
    status = srv_FLASH_Erase_Sector(sector, VOLTAGE_RANGE_3);
    srv_FLASH_Lock();

    if(status != 0)
    {
        result |= 2;
    }
    else
    {
        /* Define the number of dwords to be programmed  */
        dwords = (size  + sizeof(uint32_t) -1)/sizeof(uint32_t);
   
        srv_FLASH_Unlock();  
        /* Program flash data */
        for(ii = 0; ii < dwords;ii++)
        {
           srv_FLASH_Program_Word(((uint32_t)copy) + ii* sizeof(uint32_t), ((uint32_t*)src)[ii]);
        }
        srv_FLASH_Program_Word( ((uint32_t)copy) + ii* sizeof(uint32_t), crc);   
        srv_FLASH_Lock();

    }


    // Check data
    result = memcmp(src,copy,size);  


    //Check crc
    if( (crc != ((uint32_t *)copy)[ii]) && (destroy_nov ==0) )
    {
        result |= 1;
    }
        



    __enable_irq();

    /*!
               
          \wdg Watchdog kick - after NOVoperation                                                      
    */
     
    srv_wdg_kick();

    return result;

}


/*!

    \brief  Writes NOV storage to EEPROM

    \retval  O in case of success

    Note: Updates both copies - one after another    
*/

int srv_nov_store(uint32_t destroy_mask)
{
    int result;

    if(store_first ==  STORE_FIRST_COPY_1)
    {
        result = srv_nov_store_copy(__storage1_start__,__var_nv_start__,__var_nv_size__,destroy_mask & 0x01);

        if(result == 0)
        {
            result = srv_nov_store_copy(__storage2_start__,__var_nv_start__,__var_nv_size__,destroy_mask & 0x02);
        }
    }
    else
    {
        result = srv_nov_store_copy(__storage2_start__,__var_nv_start__,__var_nv_size__,destroy_mask & 0x02);

        if(result == 0)
        {
            result = srv_nov_store_copy(__storage1_start__,__var_nv_start__,__var_nv_size__,destroy_mask & 0x01);
        }
    }   


    if(destroy_mask != 0)
    {
      destroyed = 1;
    }


    return result;
}


int srv_nov_store_shadow_binary(void)
{
    return srv_nov_store_copy(__storage_shadow_start__,__var_nv_start__,__var_nv_size__,0);
}


int   srv_nov_store_shadow_init(void)
{
    int                     ii;
    int                     sector = -1;    
    int                     result = 0;
    uint32_t                status;


    
     /*!
    
        \wdg  Watchdog kicks - Before each NOV storage operations - once, as erase may be 
              relatively ( few miliseconds).
                           
     */
     
    srv_wdg_kick();

    for(ii = 0; ii < DIM(flash_layout);ii++)
    {
        if( (uint32_t)cal_table_copy == flash_layout[ii].start )
        {
            sector = ii;
            break;
        }
    }


    if( sector < 0) 
    {
        // Not found 

        return -1;
    }

    __disable_irq();

    /*!
            \tst  Variable 'nvCounter' Incremented each time NV storage contents is written to the flash.
                                       Note that it is written for each NV copy, so each update will increment it by factor 2.        
    */ 

    srv_FLASH_Unlock();      

    status = srv_FLASH_Erase_Sector(sector, VOLTAGE_RANGE_3);

    if(status != 0)
    {
        result |= 2;
    }
  
    srv_FLASH_Lock();

    cal_table_offset = 0;
        
    __enable_irq();

    /*!
               
          \wdg Watchdog kick - after NOVoperation                                                      
    */
     
    srv_wdg_kick();

    return result;

}

int   srv_nov_store_shadow_add(const uint32_t * words,uint32_t cnt)
{
    int                     ii;
    int                     result;





     /*!
    
        \wdg  Watchdog kicks - Before each NOV storage operations - once, as erase may be 
              relatively ( few miliseconds).
                           
     */
     
    srv_wdg_kick();


    __disable_irq();

    /*!
            \tst  Variable 'nvCounter' Incremented each time NV storage contents is written to the flash.
                                       Note that it is written for each NV copy, so each update will increment it by factor 2.        
    */ 

    
    srv_FLASH_Unlock();      
          
    /* Program flash data */
    for(ii = 0; ii < cnt;ii++)
    {
        srv_FLASH_Program_Word(((uint32_t)cal_table_copy) + (cal_table_offset + ii)* sizeof(uint32_t), words[ii]);
    }
    srv_FLASH_Lock();

    // Check data
    result = memcmp(words,cal_table_copy + cal_table_offset* sizeof(uint32_t) ,cnt*sizeof(uint32_t));  

    cal_table_offset += ii;
      
    __enable_irq();

    /*!
               
          \wdg Watchdog kick - after NOVoperation                                                      
    */
     
    srv_wdg_kick();

    return result;
}

var_caltable_t * srv_nov_store_shadow_get(uint32_t ii)
{
    if(ii * sizeof(var_caltable_t) < cal_table_size)
    {
        return  &cal_table[ii];
    }
    else
    {
        return NULL;
    }
}

int   srv_nov_store_shadow_read(void)
{
    uint8_t   line_buffer[64];
    uint8_t   cc;
    uint32_t  line_size; 
    int       result = -1;
    uint32_t  idx = 0;
    int       ii;

    const char * buffer = __storage_shadow_start__;


    while(1)
    {

next_line:

        line_size = 0;

        for(ii = 0; ii < sizeof(line_buffer);ii++)
        {
           cc =  buffer[idx++];
           line_buffer[line_size++] = cc;

           switch(cc)
           {
              case '\n':
              {
                  line_buffer[line_size]  = 0;
                  if(tsk_storage_process_cal(line_buffer,line_size) == 0)
                  {
                    cal_recovered++;
                    result = 0;                   
                  }
                  else
                  {
                    cal_faulty++;
                  }

                  goto next_line;
              }break;
              

              case 0xFF:
              {
                 // End of data
                 goto no_more;
              }break;

              default:
              {
                 // Just eat the character 
              }break;
           }
        }

       // This means overrun error

       goto no_more;
    }


no_more:


    return result;
}



int   srv_nov_is_changed(void)
{
    return memcmp(__var_nv_start__,__storage1_start__,__var_nv_size__);    
}


void   srv_nov_dump_change(srv_printf_t pprintf)
{
    int ii;
    const int32_t * storage_flash;
    const int32_t * storage_mem;


    for(ii = 0;ii < __var_nv_size__/sizeof(uint32_t);ii++)
    {
        storage_flash = &((const uint32_t*)__storage1_start__)[ii];
        storage_mem   = &((const uint32_t*)__var_nv_start__)[ii];    

        if(*storage_flash != *storage_mem)
        {
             (*pprintf)(1,"nov: change offset 0x%02x (0x%08x -> 0x%08x)\r\n",ii,*storage_flash,*storage_mem);   
        }        
    }  
}



//Unused on target platform
void  srv_nov_set_idle_hook(void (*idle_fn)(void))
{
    idle_function = idle_fn;
}


void  srv_nov_register(void * data, size_t size, void (*default_fn)(void))
{
    if(default_fn != NULL)
    {
      if(default_functions_cnt < DIM(default_functions))
      {
          default_functions[default_functions_cnt++] = default_fn;
      }
      else
      {
        fw_assert(0);
      }
    }

    fw_assert(done == 0);
}


//Unused on target platform
void srv_nov_cal_register(const var_caltable_t * data, size_t size)
{

}


//Unused on target platform
void srv_nov_start()
{

}

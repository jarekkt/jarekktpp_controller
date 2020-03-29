#ifndef NOV_SRV_H
#define NOV_SRV_H

/*! \file   nov_srv.h

    \brief  Header for low level NOV support
    
    
*/



#ifndef _WIN32

#define VAR_NV_ATTR      __attribute__ ((section (".var_nv")))


#define BT_FLT    0
#define BT_I32    1
#define BT_U32    2
#define BT_I8     3
#define BT_U8     4


typedef struct 
{
    char             name[SYS_VAR_NAME_LENGTH];
    void           * ptr;     
    uint32_t         size;   
    uint32_t         basetype; 
}var_caltable_t;



extern char __var_nv_start__[];
extern char __var_nv_end__[];

#define 	__var_nv_size__ (__var_nv_end__ - __var_nv_start__)

#else

#define VAR_NV_ATTR      

#endif




/*
    Note - not to be used directly - only through calls specified in  tsk_storage.h

*/

typedef void (*srv_printf_t) (uint32_t level_mask,const char * format, ...);


int   srv_nov_init(void);
void  srv_nov_init_default(int mode);
void  srv_nov_print_info(srv_printf_t pprintf);
int   srv_nov_is_changed(void);
int   srv_nov_store(void);
void  srv_nov_start(void);
void  srv_nov_register(void * data, size_t size, void (*default_fn)(void));







void  srv_nov_dump_change(srv_printf_t pprintf);

#endif //NOV_SRV_H



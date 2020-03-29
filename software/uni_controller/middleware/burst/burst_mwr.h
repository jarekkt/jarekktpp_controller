/*!
    \file burst_mwr.h


*/


#ifndef BURST_MWR_H
#define BURST_MWR_H


#include "system.h"


void mwr_burst_init(void);
void mwr_burst_once(void);


void burst_log_printf(uint32_t level_mask,const char  * format, ...);

int  burst_process_variable(const char* var_name, uint32_t var_name_len,const char * var_value,char * resp_buffer,uint32_t resp_buffer_len,uint32_t * execute_store);

int  burst_execute_var(char* command, int length);



#endif /* BURST_MWR_H */


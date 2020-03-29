/*!
    \file tsk_storage.h

*/



#ifndef TSK_STORAGE_H
#define TSK_STORAGE_H



#include "system.h"

#define STORAGE_MODE_BINARY  1
#define STORAGE_MODE_TEXT    2


void tsk_storage_init(void);
void tsk_storage_once(void);
void tsk_storage_activate(void);
void tsk_storage_shadow(uint32_t mode);

void tsk_storage_execute(void);
void tsk_storage_kill(void);
int32_t tsk_storage_process_cal(const char * line,uint32_t len);





#endif /* TSK_STORAGE_H */


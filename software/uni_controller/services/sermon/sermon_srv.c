 /*! \file   srv_sermon.c

    \brief  Configures variables to be sent in burst mode
      
*/

#include "system.h"
#include "services.h"


sermon_var_nv_t sermon_var_nv VAR_NV_ATTR;

volatile const var_ptable_t  * ptr;

const var_ptable_t   sermon_nvar_ptable[] SERMON_ATTR =  
{
  { "gui_mask0",       &sermon_var_nv.gui_mask[0],               E_VA_UINT_FREE},
  { "gui_mask1",       &sermon_var_nv.gui_mask[1],               E_VA_UINT_FREE},
  { "gui_mask2",       &sermon_var_nv.gui_mask[2],               E_VA_UINT_FREE},
  { "gui_mask3",       &sermon_var_nv.gui_mask[3],               E_VA_UINT_FREE},
  { "gui_mask4",       &sermon_var_nv.gui_mask[4],               E_VA_UINT_FREE},
  { "gui_mask5",       &sermon_var_nv.gui_mask[5],               E_VA_UINT_FREE},
  { "gui_mask6",       &sermon_var_nv.gui_mask[6],               E_VA_UINT_FREE},
  { "gui_mask7",       &sermon_var_nv.gui_mask[7],               E_VA_UINT_FREE},
  { "gui_mask8",       &sermon_var_nv.gui_mask[8],               E_VA_UINT_FREE},
  { "gui_mask9",       &sermon_var_nv.gui_mask[9],               E_VA_UINT_FREE},
  { "gui_mask10",      &sermon_var_nv.gui_mask[10],              E_VA_UINT_FREE},
  { "gui_mask11",      &sermon_var_nv.gui_mask[11],              E_VA_UINT_FREE},
  { "gui_mask12",      &sermon_var_nv.gui_mask[12],              E_VA_UINT_FREE},
  { "gui_mask13",      &sermon_var_nv.gui_mask[13],              E_VA_UINT_FREE}
};

void srv_sermon_init_default(void)
{
  memset(&sermon_var_nv,0,sizeof(sermon_var_nv));
}



void srv_sermon_init(void)
{
    srv_sermon_register(sermon_nvar_ptable,DIM(sermon_nvar_ptable));
    srv_nov_register(&sermon_var_nv,sizeof(sermon_var_nv),srv_sermon_init_default);
}

void srv_sermon_once(void)
{

}

void srv_sermon_register(const var_ptable_t  table[],int cnt)
{
   // otherwise optimized
   ptr = table;
}







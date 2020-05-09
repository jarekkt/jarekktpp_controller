#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__


#include "lwip/err.h"
#include "lwip/netif.h"
#include "cmsis_os.h"


#define  MAC_ADDR0	0x40
#define  MAC_ADDR1  0x74
#define  MAC_ADDR2  0xE0
#define  MAC_ADDR3  0x26
#define  MAC_ADDR4  0x89
#define  MAC_ADDR5  0xa0


/* Exported types ------------------------------------------------------------*/
/* Structure that include link thread parameters */
struct enc_irq_str {
  struct netif *netif;
  osSemaphoreId semaphore;
};
/* Exported functions ------------------------------------------------------- */
err_t ethernetif_init(struct netif *netif);
void ethernetif_process_irq(void const *argument);
void ethernetif_update_config(struct netif *netif);
void ethernetif_notify_conn_changed(struct netif *netif);
void ethernet_transmit(void);
void ethernet_irq_handler(osSemaphoreId Netif_LinkSemaphore);

/* Activate ENC28J60 interrupts processing */
#define ENC28J60_INTERRUPT

/** @defgroup PIN configuration for ENC28J60
  * @{
  */
  /*
        ENC28J60_CS ----------------------> PB4
        ENC28J60_INT ---------------------> PG2
  */
#define ENC_CS_PIN    GPIO_PIN_4
#define ENC_INT_PIN   GPIO_PIN_2
/**
  * @}
  */

#endif

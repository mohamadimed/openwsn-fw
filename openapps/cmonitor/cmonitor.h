#ifndef __CMONITOR_H
#define __CMONITOR_H

/**
\addtogroup AppUdp
\{
\addtogroup cmonitor
\{
*/

#include "opencoap.h"

//=========================== define ==========================================

//=========================== typedef =========================================

typedef struct {
   coap_resource_desc_t         desc;
   opentimers_id_t              timerId;
   bool                         busySendingCexample;
} cmonitor_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void cmonitor_init(void);

/**
\}
\}
*/

#endif


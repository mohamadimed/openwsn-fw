#ifndef __UGATEWAY_H
#define __UGATEWAY_H

/**
\addtogroup AppUdp
\{
\addtogroup uinject
\{
*/

#include "opentimers.h"
#include "openudp.h"

//=========================== define ==========================================

// the period to fetch the packet from the radio
#define UGATEWAY_PERIOD_MS 10000

//=========================== typedef =========================================

//=========================== variables =======================================


//=========================== prototypes ======================================

void ugateway_init(void);
void ugateway_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void ugateway_receive(OpenQueueEntry_t* msg);
/**
\}
\}
*/

#endif


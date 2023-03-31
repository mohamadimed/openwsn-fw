#ifndef __CMONITOR_H
#define __CMONITOR_H

/**
\addtogroup AppUdp
\{
\addtogroup cmonitor
\{
*/

#include "opencoap.h"
#define NBMETRICS 5
//=========================== define ==========================================

//=========================== typedef =========================================

typedef enum{
    TRACK_LIST,
    ROUTE_LIST,
    NUM_TICS,
    NEIGHBORS_LIST,
    CELL_LIST
} metrics_t;

typedef struct {
   coap_resource_desc_t         desc;
   //opensensors_resource_desc_t* opensensors_resource; //if needed will be implemented accordingly later
   uint16_t                     period;
   opentimers_id_t              timerId;
   //metrics_t                    resourceId; //id of a given ressouce, eg: 0 for nb, neighbors...
} cmonitor_resource_t;


typedef struct {
   coap_resource_desc_t         desc;
   cmonitor_resource_t          cmonitor_resource[NBMETRICS];
   opentimers_id_t              timerId;
   uint8_t                      nbMetrics;
   bool                         busySendingCexample;
   uint8_t                      medType;
} cmonitor_vars_t;


//=========================== module variables ================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void cmonitor_init(void);

/**
\}
\}
*/

#endif


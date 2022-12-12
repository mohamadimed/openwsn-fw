/**
\brief CoAP 6top application

\author Xavi Vilajosana <xvilajosana@eecs.berkeley.edu>, February 2013.
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, July 2014
*/

#ifndef __CINSTRUMENT_H
#define __CINSTRUMENT_H

/**
\addtogroup AppCoAP
\{
\addtogroup cinstrument
\{
*/

#include "opendefs.h"
#include "opencoap.h"
#include "schedule.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   coap_resource_desc_t desc;
} cinstrument_vars_t;

//=========================== prototypes ======================================

void cinstrument_init(void);

/**
\}
\}
*/

#endif

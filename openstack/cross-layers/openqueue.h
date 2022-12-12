#ifndef __OPENQUEUE_H
#define __OPENQUEUE_H

/**
\addtogroup cross-layers
\{
\addtogroup OpenQueue
\{
*/

#include "opendefs.h"
#include "IEEE802154.h"

//=========================== define ==========================================

#define QUEUELENGTH  20
#define BIGQUEUELENGTH  2

//=========================== typedef =========================================

typedef struct {
   uint8_t  creator;
   uint8_t  owner;
   cellRadioSetting_t l2_cellRadioSetting;
   open_addr_t   l2_nextORpreviousHop;
} debugOpenQueueEntry_t;

//=========================== module variables ================================

typedef struct {
   OpenQueueEntry_t queue[QUEUELENGTH];
   OpenQueueBigEntry_t big_queue[BIGQUEUELENGTH];
} openqueue_vars_t;

typedef struct 
{
	uint8_t                   maxBuffSize;              // max packets in the buffer
    uint8_t                   minBuffSize;              // min packets in the buffer
} openqueue_stats_t;
//=========================== prototypes ======================================

// admin
void               openqueue_init(void);
bool               debugPrint_queue(void);
// called by any component
OpenQueueEntry_t*  openqueue_getFreePacketBuffer(uint8_t creator);
OpenQueueEntry_t*  openqueue_getFreeBigPacketBuffer(uint8_t creator);
owerror_t          openqueue_freePacketBuffer(OpenQueueEntry_t* pkt);
void               openqueue_removeAllCreatedBy(uint8_t creator);
bool               openqueue_isHighPriorityEntryEnough(void);
openqueue_stats_t  openqueue_get_stats(void);
void  openqueue_reset_stats(void);

// called by ICMPv6
void               openqueue_updateNextHopPayload(open_addr_t* newNextHop, cellRadioSetting_t* newNextHopRadio);
// called by res
OpenQueueEntry_t*  openqueue_sixtopGetSentPacket(void);
OpenQueueEntry_t*  openqueue_sixtopGetReceivedPacket(void);
uint8_t            openqueue_getNum6PResp(void);
uint8_t            openqueue_getNum6PReq(open_addr_t* neighbor);
void               openqueue_remove6PrequestToNeighbor(open_addr_t* neighbor);
// called by IEEE80215E
OpenQueueEntry_t*  openqueue_macGetEBPacket(void);
OpenQueueEntry_t*  openqueue_macGetKaPacket(open_addr_t* toNeighbor);
OpenQueueEntry_t*  openqueue_macGetDIOPacket(void);
OpenQueueEntry_t*  openqueue_macGetUnicastPakcet(open_addr_t* toNeighbor);
/**
\}
\}
*/

#endif

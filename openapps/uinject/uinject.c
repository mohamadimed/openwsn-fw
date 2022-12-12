#include "opendefs.h"
#include "uinject.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "schedule.h"
#include "neighbors.h"
#include "icmpv6rpl.h"
#include "idmanager.h"
#include "openrandom.h"

#include "msf.h"

//=========================== defines =========================================

#define UINJECT_TRAFFIC_RATE 1 ///> the value X indicates 1 packet/X minutes

//=========================== variables =======================================

uinject_vars_t uinject_vars;

static const uint8_t uinject_payload[]    = "uinject";
static const uint8_t uinject_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};
//Added by mm
/*static const uint8_t uinject_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x12, 0x4b, 0x00, 0x14, 0xb5, 0xd3, 0x19
};*/
//=========================== prototypes ======================================

void uinject_timer_cb(opentimers_id_t id);
void uinject_task_cb(void);

//=========================== public ==========================================

void uinject_init(void) {

    // clear local variables
    memset(&uinject_vars,0,sizeof(uinject_vars_t));

    // register at UDP stack
    uinject_vars.desc.port              = WKP_UDP_INJECT;
    uinject_vars.desc.callbackReceive   = &uinject_receive;
    uinject_vars.desc.callbackSendDone  = &uinject_sendDone;
    openudp_register(&uinject_vars.desc);

    uinject_vars.period = UINJECT_PERIOD_MS;
    // start periodic timer
    uinject_vars.timerId = opentimers_create(TIMER_GENERAL_PURPOSE, TASKPRIO_UDP);
    opentimers_scheduleIn(
        uinject_vars.timerId,
        UINJECT_PERIOD_MS,
        TIME_MS,
        TIMER_PERIODIC,
        uinject_timer_cb
    ); 
}

void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error) {

    if (error==E_FAIL){
        openserial_printError(
            COMPONENT_UINJECT,
            ERR_MAXRETRIES_REACHED,
            (errorparameter_t)uinject_vars.counter,
            (errorparameter_t)0
        );
    }

    // free the packet buffer entry
    openqueue_freePacketBuffer(msg);

    // allow send next uinject packet
    uinject_vars.busySendingUinject = FALSE;
}

void uinject_receive(OpenQueueEntry_t* pkt) {

    openqueue_freePacketBuffer(pkt);
}

//=========================== private =========================================

void uinject_timer_cb(opentimers_id_t id){
    // calling the task directly as the timer_cb function is executed in
    // task mode by opentimer already
    if(openrandom_get16b()<(0xffff/UINJECT_TRAFFIC_RATE)){
              uinject_task_cb();//printf("IamHERE1\n");
    }//printf("IamHERE2\n");
}

void uinject_task_cb(void) {
    OpenQueueEntry_t*    pkt;
    uint8_t              asnArray[5];
    uint8_t              numCellsUsed;
    uint8_t              numNeighbors;
    uint16_t             DAGRank;
    open_addr_t          parentNeighbor;
    bool                 foundNeighbor;
    
    uint32_t             ticksTx;
    uint32_t             ticksOn;
    uint32_t             ticksTx_0;
    uint32_t             ticksOn_0;
    uint32_t             ticksTx_1;
    uint32_t             ticksOn_1;
    uint32_t             ticksTx_2;
    uint32_t             ticksOn_2;
    uint32_t             ticksInTotal;
      //char msg[5]="test";
     //openserial_print_str(msg,5*sizeof(char));
    // don't run if not synch
    if (ieee154e_isSynch() == FALSE) {
        return;
    }

    // don't run on dagroot
    if (idmanager_getIsDAGroot()) {
        opentimers_destroy(uinject_vars.timerId);
        return;
    }

    foundNeighbor = icmpv6rpl_getPreferredParentEui64(&parentNeighbor);
    if (foundNeighbor==FALSE) {
        return;
    }

    if (schedule_hasNegotiatedCellToNeighbor(&parentNeighbor, CELLTYPE_TX) == FALSE) {
        return;
    }

    if (uinject_vars.busySendingUinject==TRUE) {
        // don't continue if I'm still sending a previous uinject packet
        return;
    }

    // if you get here, send a packet

    // get a free packet buffer
    pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);
    if (pkt==NULL) {
        openserial_printError(
            COMPONENT_UINJECT,
            ERR_NO_FREE_PACKET_BUFFER,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return;
    }

    pkt->owner                         = COMPONENT_UINJECT;
    pkt->creator                       = COMPONENT_UINJECT;
    pkt->l4_protocol                   = IANA_UDP;
    pkt->l4_destination_port           = WKP_UDP_INJECT;
    pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_INJECT;
    pkt->l3_destinationAdd.type        = ADDR_128B;
    memcpy(&pkt->l3_destinationAdd.addr_128b[0],uinject_dst_addr,16);

    // add payload
    packetfunctions_reserveHeaderSize(pkt,sizeof(uinject_payload)-1);
    memcpy(&pkt->payload[0],uinject_payload,sizeof(uinject_payload)-1);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    pkt->payload[1] = (uint8_t)((uinject_vars.counter & 0xff00)>>8);
    pkt->payload[0] = (uint8_t)(uinject_vars.counter & 0x00ff);
    uinject_vars.counter++;

    packetfunctions_reserveHeaderSize(pkt,sizeof(asn_t));
    ieee154e_getAsn(asnArray);
    pkt->payload[0] = asnArray[0];
    pkt->payload[1] = asnArray[1];
    pkt->payload[2] = asnArray[2];
    pkt->payload[3] = asnArray[3];
    pkt->payload[4] = asnArray[4];

    // number of cells
//    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
//    numCellsUsed = msf_getPreviousNumCellsUsed(CELLTYPE_TX);
//    pkt->payload[0] = numCellsUsed;

//    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
//    numCellsUsed = msf_getPreviousNumCellsUsed(CELLTYPE_RX);
//    pkt->payload[0] = numCellsUsed;

    // number of neighbors
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
    numNeighbors = neighbors_getNumNeighbors();
    pkt->payload[0] = numNeighbors;
    
    // queue stats
    openqueue_stats_t oqs = openqueue_get_stats();
    openqueue_reset_stats();

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
    pkt->payload[0] = oqs.maxBuffSize;

//    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
//    pkt->payload[0] = oqs.minBuffSize;

    // address
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    pkt->payload[1] = (uint8_t)(idmanager_getMyID(ADDR_16B)->addr_16b[0]);
    pkt->payload[0] = (uint8_t)(idmanager_getMyID(ADDR_16B)->addr_16b[1]);
    
    // DAG Rank
    DAGRank = icmpv6rpl_getMyDAGrank();
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    pkt->payload[1] = (uint8_t)((DAGRank & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( DAGRank & 0x000000ff);

    // duty cycle info
    ieee154e_getRadioTicsInfo(
                               &ticksTx, 
                               &ticksOn,
                               &ticksTx_0, 
                               &ticksOn_0,
                               &ticksTx_1, 
                               &ticksOn_1,
                               &ticksTx_2, 
                               &ticksOn_2,
                               &ticksInTotal
                               );
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksOn & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksOn & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksOn & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksOn & 0x000000ff);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksTx & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksTx & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksTx & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksTx & 0x000000ff);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksOn_0 & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksOn_0 & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksOn_0 & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksOn_0 & 0x000000ff);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksTx_0 & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksTx_0 & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksTx_0 & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksTx_0 & 0x000000ff);
    
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksOn_1 & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksOn_1 & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksOn_1 & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksOn_1 & 0x000000ff);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksTx_1 & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksTx_1 & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksTx_1 & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksTx_1 & 0x000000ff);
    
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksOn_2 & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksOn_2 & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksOn_2 & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksOn_2 & 0x000000ff);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksTx_2 & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksTx_2 & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksTx_2 & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksTx_2 & 0x000000ff);
    
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
    pkt->payload[3] = (uint8_t)((ticksInTotal & 0xff000000) >> 24);
    pkt->payload[2] = (uint8_t)((ticksInTotal & 0x00ff0000) >> 16);
    pkt->payload[1] = (uint8_t)((ticksInTotal & 0x0000ff00) >> 8);
    pkt->payload[0] = (uint8_t)( ticksInTotal & 0x000000ff);
    // resettin mac stats
    ieee154e_resetStats();
                 
  
    if ((openudp_send(pkt))==E_FAIL) {
        openqueue_freePacketBuffer(pkt);
    } else {
        // set busySending to TRUE
     // printf("send done\n");
        uinject_vars.busySendingUinject = TRUE;

    }
}




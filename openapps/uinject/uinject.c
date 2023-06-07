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

/*added by mm to enable collection of KPIs if we node is not the root*/

uint16_t uinject_rx_counter; //counter for rceived uinject packets
uint16_t uinject_tx_counter; //counter for sent uinject packets -egt it from packet payload
uint32_t uinject_total_latency_counter; //calculated as the cumulative number of slots : diffrence ASN of received pkt - ASN of sent packet (of all packets)
uint16_t uinject_diff_latency; //This hsould not be global var, it's here just for live watch
uint16_t uinject_min_latency; //minimum latency recorded
uint16_t uinject_max_latency; //maximum latency recorded
asn_t source_asn,current_asn;
uint8_t in_tx_mode = 0 ; //set this variable to 1 if we are programming a node to send uinject packets, otherwise set it to 0 -- By default =0 and should be changed through cinstrument coap PUT

//next need to select destination node: root or address of the node to target

uint8_t byte0_dest = 0 ;
uint8_t byte1_dest = 0 ;

static const uint8_t uinject_payload[]    = "uinject";
/*static const uint8_t uinject_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};*/


//Added by mm
uint8_t uinject_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x12, 0x4b, 0x00, 0x14, 0xb5, 0x00, 0x00
};
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
    
    //added by mm; init kpi vars
    uinject_rx_counter = 0;
    uinject_tx_counter = 0;
    uinject_total_latency_counter = 0;
    uinject_min_latency = 999;
    uinject_max_latency = 0;
    
    //added by mm: this periodic timer should be disabled in reception side when transmitting (point-to-point uinject pkt)
    //Besides, the reception function of uinject pkt should be implemented so we can get and aggregate the metrics
    
    // start periodic timer
    {
 uinject_vars.timerId = opentimers_create(TIMER_GENERAL_PURPOSE, TASKPRIO_UDP);
    opentimers_scheduleIn(
        uinject_vars.timerId,
        UINJECT_PERIOD_MS,
        TIME_MS,
        TIMER_PERIODIC,
        uinject_timer_cb
          ); }


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
   
  //toggle Yellow LED to know if we received a uinjecxt packet
    leds_debug_toggle();
    
    //declare asn to calculate slot differencies
   //asn_t source_asn,current_asn;

   
   //diffrence between TX and RX: number of slots

   uinject_diff_latency = 0;    
   uinject_rx_counter++; //increment number of received uinject pkts
    
   
   //Seems like if the destination is directly reacheable from the source without passing by the root, the packet size is 58 instead of 56.
   //as a stupid solution just to make it work, I will try to adapt the treatment to the uinject payload length (which is not a definitive solution)
   //for this I will use a condition and an offset to adapte it according to the need
   
   uint8_t offset;
   
   if(pkt->length == 56)
     offset = 42; 
      if(pkt->length == 58)
     offset = 44;  
     
   
   //update tx_counter 
   
   uinject_tx_counter =((pkt->payload[offset+5] & 0x00ff) + ((pkt->payload[offset+6] & 0x00ff)<<8)); //still to be defined which position in vector
    
    current_asn = pkt->l2_asn; //get asn on rx of uinjct pkt
    

    
    //getting the ASN bytes from pkt to compare them with l2_asn as in parserdata.py
    source_asn.bytes0and1 = pkt->payload[offset+1]<<8|pkt->payload[offset];  //((pkt->payload[42] & 0x00ff) + ((pkt->payload[43] & 0x00ff)<<8)); //regrouping bytes0and1 
    source_asn.bytes2and3 = pkt->payload[offset+3]<<8|pkt->payload[offset+2];//((pkt->payload[44] & 0x00ff) + ((pkt->payload[45] & 0x00ff)<<8)); //regrouping bytes2and3 
    source_asn.byte4 = (pkt->payload[offset+4] & 0x00ff); //regrouping bytes2and3 
    
                        
   //process ASN diff to get number of slots
     if(current_asn.byte4==source_asn.byte4) 
    
       uinject_diff_latency = 0x10000 * (current_asn.bytes2and3 - source_asn.bytes2and3) + (current_asn.bytes0and1 - source_asn.bytes0and1);
       uinject_total_latency_counter = uinject_total_latency_counter + uinject_diff_latency;
       
       if(uinject_diff_latency < uinject_min_latency)
       uinject_min_latency = uinject_diff_latency;
       
       if(uinject_diff_latency > uinject_max_latency )
       uinject_max_latency = uinject_diff_latency;

    //first send diff (current_asn-source_asn) and send it over coap to get the number and then calculate latency
    //uinject_latency_counter += asn_diff;
    
    //at coap req: send total latencies & uniject rx counter & source counter; so on reception in python we calculate PDR based on counters, and average latency using total latencies and rx_counter
    
    openqueue_freePacketBuffer(pkt);
}

//=========================== private =========================================

void uinject_timer_cb(opentimers_id_t id){
    // calling the task directly as the timer_cb function is executed in
    // task mode by opentimer already
    if(openrandom_get16b()<(0xffff/UINJECT_TRAFFIC_RATE)){
             if (in_tx_mode) uinject_task_cb();
    }
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
    uinject_vars.counter++;
    pkt->payload[1] = (uint8_t)((uinject_vars.counter & 0xff00)>>8);
    pkt->payload[0] = (uint8_t)(uinject_vars.counter & 0x00ff);
    

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


/*-----------------Getters for KPIS------------------*/
/*--These functions are meant to be run for uinject_reciever--*/

//Get the number of TX uinject_packets sent from source
uint16_t uinject_get_NumTx(void){

return uinject_tx_counter;

}
 //Get the number of RX uinject_packets recieved by the destination           
uint16_t uinject_get_NumRx(void){

return uinject_rx_counter;

}
//Get the total latency counter: gives the cumulation of all recorded diffrencies between a source ASN and on reception ASN. this gives total number of diffrencies ASN,\
should be divided by number of RX packets to get the average number of ASN diffrencies , to get latency multply it by 10 ms to get time in ms
uint32_t uinject_get_total_latency_counter(void){

return uinject_total_latency_counter;

}

//Get the minimum latency recorded (just for statistics)
uint16_t uinject_get_min_latency(void){

return uinject_min_latency;

}
//Get the maximum latency recorded (just for statistics)    
uint16_t uinject_get_max_latency(void){

return uinject_max_latency;

}


/*-----------------Setters for KPIS------------------*/
/*--These functions are meant to be run for uinject_reciever--*/

//Reset the number of TX uinject_packets sent from source
void uinject_reset_NumTx(void){

uinject_tx_counter =0 ;

}
 //reset the number of RX uinject_packets recieved by the destination           
void uinject_reset_NumRx(void){

uinject_rx_counter=0 ;

}
//reset the total latency counter
void uinject_reset_total_latency_counter(void){

uinject_total_latency_counter=0 ;

}

//reset the minimum latency recorded (just for statistics)
void uinject_reset_min_latency(void){

uinject_min_latency=999 ;

}
//reset the maximum latency recorded (just for statistics)    
void uinject_reset_max_latency(void){

uinject_max_latency=0 ;

}


/*--These functions are meant to be run for uinject_sender--*/
//Trigger node to start sending uinject packets , so sert it in tx mode and specify the destination node byr the last 2-bytes of its address
void uinject_start_sending(uint8_t byte0, uint8_t byte1){
uinject_dst_addr[14] = byte0;
uinject_dst_addr[15] = byte1;
in_tx_mode = 1;

}

//Trigger node to stop sending uinject packets , so sert it in rx mode and reset counter
void uinject_stop_sending(void){

in_tx_mode = 0;

//Reset uinject counter -->  reset the uinject_counter on asking node to stop sending uinject packets
uinject_vars.counter = 0;
}


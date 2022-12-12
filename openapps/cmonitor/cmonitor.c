/**
\brief A CoAP resource which indicates the board its running on.
*/

#include "opendefs.h"
#include "cmonitor.h"
#include "opencoap.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "board.h"
#include "idmanager.h"
#include "IEEE802154E.h"
#include "neighbors.h"
#include "icmpv6rpl.h"

//=========================== defines =========================================

const uint8_t cmonitor_path0[] = "m";

//=========================== variables =======================================

cmonitor_vars_t cmonitor_vars;

//=========================== prototypes ======================================

owerror_t     cmonitor_receive(
        OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
);
void          cmonitor_sendDone(
   OpenQueueEntry_t* msg,
   owerror_t error
);

//=========================== public ==========================================

/**
\brief Initialize this module.
*/
void cmonitor_init(void) {
   // do not run if DAGroot
   if(idmanager_getIsDAGroot()==TRUE) return; 
   
   // prepare the resource descriptor for the /i path
   cmonitor_vars.desc.path0len             = sizeof(cmonitor_path0)-1;
   cmonitor_vars.desc.path0val             = (uint8_t*)(&cmonitor_path0);
   cmonitor_vars.desc.path1len             = 0;
   cmonitor_vars.desc.path1val             = NULL;
   cmonitor_vars.desc.componentID          = COMPONENT_CMONITOR;
   cmonitor_vars.desc.securityContext      = NULL;
   cmonitor_vars.desc.discoverable         = TRUE;
   cmonitor_vars.desc.callbackRx           = &cmonitor_receive;
   cmonitor_vars.desc.callbackSendDone     = &cmonitor_sendDone;
   
   // register with the CoAP module
   opencoap_register(&cmonitor_vars.desc);
}

//=========================== private =========================================

/**
\brief Called when a CoAP message is received for this resource.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t cmonitor_receive(
        OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
) {
   owerror_t outcome;
   uint8_t              numNeighbors;
   open_addr_t          Neighbor;
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
    uint8_t              current_cell_radio_setting;
    uint8_t              parrent_radio;
    uint8_t              neighbor_radio;
    uint8_t              neighbor_counter;
    
    
   switch (coap_header->Code) {
      case COAP_CODE_REQ_GET:
         //=== reset packet payload (we will reuse this packetBuffer)
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         
         //=== prepare  CoAP response
         
                      
         //Cell Radio Setting
         current_cell_radio_setting = schedule_getCellRadioSetting();
          
         packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));

          msg->payload[0] = current_cell_radio_setting;
          
         //Max buff Size
          openqueue_stats_t oqs = openqueue_get_stats();
          openqueue_reset_stats();

          packetfunctions_reserveHeaderSize(msg,2*sizeof(uint8_t));
          msg->payload[1] = oqs.maxBuffSize;
          msg->payload[0] = oqs.minBuffSize;
         
          
         //Num Neighbors
         packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
         numNeighbors = neighbors_getNumNeighbors();
         msg->payload[0] = numNeighbors;
        
         /*
         //List of neighbors        
         //Need to refactor it so it returns only last 2 bytes and radio to not overflow the buffer (so costly)       
         for (neighbor_counter = 0; neighbor_counter < numNeighbors; neighbor_counter++)
         {
         packetfunctions_reserveHeaderSize(msg,9*sizeof(uint8_t));   
         foundNeighbor = neighbors_getNeighborKey(&Neighbor,ADDR_64B,&neighbor_radio,neighbor_counter);
         
         if (foundNeighbor)
         {
         msg->payload[8] = neighbor_radio;
         msg->payload[7] = Neighbor.addr_64b[7];
         msg->payload[6] = Neighbor.addr_64b[6];
         msg->payload[5] = Neighbor.addr_64b[5];
         msg->payload[4] = Neighbor.addr_64b[4];
         msg->payload[3] = Neighbor.addr_64b[3];
         msg->payload[2] = Neighbor.addr_64b[2];
         msg->payload[1] = Neighbor.addr_64b[1];
         msg->payload[0] = Neighbor.addr_64b[0];
         }
         }
          */

         //Preffered Parent
         packetfunctions_reserveHeaderSize(msg,9*sizeof(uint8_t));

         foundNeighbor = icmpv6rpl_getPreferredParentKey(&parentNeighbor,&parrent_radio); //retrive parent address and radio
         //foundNeighbor = icmpv6rpl_getPreferredParentEui64(&parentNeighbor); //retrive only parent address
         if (foundNeighbor)
         {
         msg->payload[8] = parrent_radio;
         msg->payload[7] = parentNeighbor.addr_64b[7];
         msg->payload[6] = parentNeighbor.addr_64b[6];
         msg->payload[5] = parentNeighbor.addr_64b[5];
         msg->payload[4] = parentNeighbor.addr_64b[4];
         msg->payload[3] = parentNeighbor.addr_64b[3];
         msg->payload[2] = parentNeighbor.addr_64b[2];
         msg->payload[1] = parentNeighbor.addr_64b[1];
         msg->payload[0] = parentNeighbor.addr_64b[0];
         }


         
       
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
    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksOn & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksOn & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksOn & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksOn & 0x000000ff);

    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksTx & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksTx & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksTx & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksTx & 0x000000ff);

    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksOn_0 & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksOn_0 & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksOn_0 & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksOn_0 & 0x000000ff);

    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksTx_0 & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksTx_0 & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksTx_0 & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksTx_0 & 0x000000ff);
    
    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksOn_1 & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksOn_1 & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksOn_1 & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksOn_1 & 0x000000ff);

    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksTx_1 & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksTx_1 & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksTx_1 & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksTx_1 & 0x000000ff);
    
    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksOn_2 & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksOn_2 & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksOn_2 & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksOn_2 & 0x000000ff);

    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksTx_2 & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksTx_2 & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksTx_2 & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksTx_2 & 0x000000ff);
    
    packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
    msg->payload[3] = (uint8_t)((ticksInTotal & 0xff000000) >> 24);
    msg->payload[2] = (uint8_t)((ticksInTotal & 0x00ff0000) >> 16);
    msg->payload[1] = (uint8_t)((ticksInTotal & 0x0000ff00) >> 8);
    msg->payload[0] = (uint8_t)( ticksInTotal & 0x000000ff);
    // resettin mac stats
    //ieee154e_resetStats();

         
         
        /* 
         // radio name
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '\n';
         packetfunctions_reserveHeaderSize(msg,sizeof(infoRadioName)-1);
         memcpy(&msg->payload[0],&infoRadioName,sizeof(infoRadioName)-1);
         
         // uC name
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '\n';
         packetfunctions_reserveHeaderSize(msg,sizeof(infouCName)-1);
         memcpy(&msg->payload[0],&infouCName,sizeof(infouCName)-1);
         
         // board name
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '\n';
         packetfunctions_reserveHeaderSize(msg,sizeof(infoBoardname)-1);
         memcpy(&msg->payload[0],&infoBoardname,sizeof(infoBoardname)-1);
         
         // stack name and version
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '\n';
         packetfunctions_reserveHeaderSize(msg,sizeof(infoStackName)-1+6);
         memcpy(&msg->payload[0],&infoStackName,sizeof(infoStackName)-1);
         msg->payload[sizeof(infoStackName)-1+6-6] = '0'+OPENWSN_VERSION_MAJOR;
         msg->payload[sizeof(infoStackName)-1+6-5] = '.';
         msg->payload[sizeof(infoStackName)-1+6-4] = '0'+OPENWSN_VERSION_MINOR / 10;
         msg->payload[sizeof(infoStackName)-1+6-3] = '0'+OPENWSN_VERSION_MINOR % 10;
         msg->payload[sizeof(infoStackName)-1+6-2] = '.';
         msg->payload[sizeof(infoStackName)-1+6-1] = '0'+OPENWSN_VERSION_PATCH;
         */
         // set the CoAP header
         coap_header->Code                = COAP_CODE_RESP_CONTENT;
         
         outcome                          = E_SUCCESS;
         break;
      default:
         // return an error message
         outcome = E_FAIL;
   }
   
   return outcome;
}

/**
\brief The stack indicates that the packet was sent.

\param[in] msg The CoAP message just sent.
\param[in] error The outcome of sending it.
*/
void cmonitor_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
  {debugpins_frame_toggle();debugpins_frame_toggle();}
   openqueue_freePacketBuffer(msg);
}

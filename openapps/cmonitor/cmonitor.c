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
const uint8_t cmonitor_nighborsCount_path1[]      = "nc";
const uint8_t cmonitor_DAGRank_path1[]      = "dr";
const uint8_t cmonitor_preferredParent_path1[]         = "pr"; //here we return parent@ (last 2 Bytes), parent radio
const uint8_t cmonitor_nighborsList_path1[]         = "nl"; //here we return list of neighbors@ (last 2 Bytes), neighbors radio, and neighbors RSSI [@,radio,RSSI] x neighborsCount

//=========================== variables =======================================

cmonitor_vars_t cmonitor_vars;

//=========================== prototypes ======================================

void cmonitor_register(
   cmonitor_resource_t* cmonitor_resource, uint8_t id
);



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

void cmonitor_fillpayload(
   OpenQueueEntry_t* msg,
   uint8_t           id
);

//=========================== public ==========================================

/**
\brief Initialize this module.
*/
void cmonitor_init(void) {
   
   uint8_t i;
   
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
   
   
   cmonitor_vars.nbMetrics                 = 0;
   
   for(i=0;i<NBMETRICS;i++)
      {
      cmonitor_vars.cmonitor_resource[i].period                = i;
      cmonitor_register(&cmonitor_vars.cmonitor_resource[i],i);
      cmonitor_vars.nbMetrics++;
      }
}

//=========================== private =========================================
/**
   \brief Register a cmonitor resource into opencoap.
   \param[in] cmonitor_resource The resource to be registered.
*/
void cmonitor_register(
      cmonitor_resource_t* cmonitor_resource,uint8_t id
   ) {
   cmonitor_resource->desc.path0len         = sizeof(cmonitor_path0)-1;
   cmonitor_resource->desc.path0val         = (uint8_t*)(&cmonitor_path0);
   switch (id) {
      case NEIGHBORS_COUNT:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_nighborsCount_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_nighborsCount_path1);
         break;
       case DAGRANK:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_DAGRank_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_DAGRank_path1);
         break;
      case PREFERRED_PARENT:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_preferredParent_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_preferredParent_path1);
         break;
      case NEIGHBORS_LIST:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_nighborsList_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_nighborsList_path1);
         break;      
      default:
         break;

   }
   cmonitor_resource->desc.componentID          = COMPONENT_CMONITOR;
   cmonitor_resource->desc.discoverable         = TRUE;
   cmonitor_resource->desc.callbackRx           = &cmonitor_receive;
   cmonitor_resource->desc.callbackSendDone     = &cmonitor_sendDone;


   // register with the CoAP module
   opencoap_register(&cmonitor_resource->desc);
}


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
   uint8_t              id;

    
    
   switch (coap_header->Code) {
      case COAP_CODE_REQ_GET:
         //=== reset packet payload (we will reuse this packetBuffer)
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         
         
         if (coap_incomingOptions[1].type != COAP_OPTION_NUM_URIPATH) { //If the URI Path is not set, then return the list of available URI paths

            // have CoAP module write links to cmonitor resources
            opencoap_writeLinks(msg,COMPONENT_CMONITOR);

          
            // add return option
            cmonitor_vars.medType = COAP_MEDTYPE_APPLINKFORMAT;
            coap_outgoingOptions[0].type = COAP_OPTION_NUM_CONTENTFORMAT;
            coap_outgoingOptions[0].length = 1;
            coap_outgoingOptions[0].pValue = &cmonitor_vars.medType;
            *coap_outgoingOptionsLen = 1;
            
            coap_header->Code   = COAP_CODE_RESP_CONTENT;
         
         outcome             = E_SUCCESS;
         
            
         } else { 
         
           //prepar coap response
           //---------------------------------------------------------------------------------
           
           {
            for(id=0;id<cmonitor_vars.nbMetrics;id++) {
               if (
                  memcmp(
                     coap_incomingOptions[1].pValue,
                     cmonitor_vars.cmonitor_resource[id].desc.path1val,
                     cmonitor_vars.cmonitor_resource[id].desc.path1len
                  )==0
                  ) {
                  break;
               }
            }
              cmonitor_fillpayload(msg,id);
              // add return option
              cmonitor_vars.medType = COAP_MEDTYPE_APPOCTETSTREAM;
              coap_outgoingOptions[0].type = COAP_OPTION_NUM_CONTENTFORMAT;
              coap_outgoingOptions[0].length = 1;
              coap_outgoingOptions[0].pValue = &cmonitor_vars.medType;
              *coap_outgoingOptionsLen = 1;
         }
         
         
         //---------------------------------------------------------------------------------
 
    
         }
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
\brief Called when a CoAP message has to fill the payload with csensors data.

\param[in] msg          The message to be filled.
\param[in] id           Resource id in sensors array.

*/
void cmonitor_fillpayload(OpenQueueEntry_t* msg,
      uint8_t id) {
   uint8_t              numNeighbors;
   open_addr_t          Neighbor;
   open_addr_t          parentNeighbor;
   bool                 foundNeighbor;
   dagrank_t             MyDAGRANK;
   
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
        
         //=== prepare  CoAP response
         
        switch (id) {

                      
         //Cell Radio Setting
        /* current_cell_radio_setting = schedule_getCellRadioSetting();
          
         packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));

          msg->payload[0] = current_cell_radio_setting;
          
         //Max buff Size
          openqueue_stats_t oqs = openqueue_get_stats();
          openqueue_reset_stats();

          packetfunctions_reserveHeaderSize(msg,2*sizeof(uint8_t));
          msg->payload[1] = oqs.maxBuffSize;
          msg->payload[0] = oqs.minBuffSize;*/
         
          case NEIGHBORS_COUNT:       
          //Num Neighbors
          packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
          numNeighbors = neighbors_getNumNeighbors();
          msg->payload[0] = numNeighbors;
          break; 
          
          
          case DAGRANK:       
          //Return my DAGRank
          packetfunctions_reserveHeaderSize(msg,sizeof(uint16_t));
          MyDAGRANK = icmpv6rpl_getMyDAGrank(); 
          msg->payload[1] = (uint8_t)((MyDAGRANK & 0xff00) >> 8);
          msg->payload[0] = (uint8_t)( MyDAGRANK & 0x00ff);
          break;
          
          
        case PREFERRED_PARENT:
          //Get the preferred parent( returning last two bytes of his address and the radio)
         packetfunctions_reserveHeaderSize(msg,3*sizeof(uint8_t));

         foundNeighbor = icmpv6rpl_getPreferredParentKey(&parentNeighbor,&parrent_radio); //retrive parent address and radio
         if (foundNeighbor)
         {
         msg->payload[2] = parrent_radio;
         msg->payload[1] = parentNeighbor.addr_64b[7];
         msg->payload[0] = parentNeighbor.addr_64b[6];
         }
         
         case NEIGHBORS_LIST:
         //returning list of neighbors for each [num_neighbors]+ liste:[@(last 2-bytes), radio, RSSI]
          packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
          numNeighbors = neighbors_getNumNeighbors();
          msg->payload[0] = numNeighbors;

         for (neighbor_counter = 0; neighbor_counter < numNeighbors; neighbor_counter++)
              {
                
                if(neighbor_counter < 13){ //Considering 12 is MAXNUM of Neighbors we autorise on the packets to not overflow
                
         packetfunctions_reserveHeaderSize(msg,4*sizeof(uint8_t));   
         foundNeighbor = neighbors_getNeighborKey(&Neighbor,ADDR_64B,&neighbor_radio,neighbor_counter);
         
         if (foundNeighbor)
          {
            msg->payload[3] = 55;
            msg->payload[2] = neighbor_radio;
            msg->payload[1] = Neighbor.addr_64b[7];
            msg->payload[0] = Neighbor.addr_64b[6];
          }                               }
              }//END loop-for neighbors check

         
         } //switch END
/*
            
       
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
*/
         
         
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



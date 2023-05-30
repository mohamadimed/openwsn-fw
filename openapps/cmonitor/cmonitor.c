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
#include "schedule.h"
#include "track.h"
//#include "uinject.h"

//=========================== defines =========================================

const uint8_t cmonitor_path0[] = "m";
const uint8_t cmonitor_trackList_path1[]      = "tl"; //here we return TrackID, owner@ 
const uint8_t cmonitor_routeList_path1[]      = "rl";  //here we return parent@ (last 2 Bytes), parent radio, DAGRank (2 bytes) [route List]
const uint8_t cmonitor_numTics_path1[]         = "nt"; //here we return parent@ (last 2 Bytes), parent radio [numTics]
const uint8_t cmonitor_nighborsList_path1[]         = "nl"; //here we return list of neighbors@ (last 2 Bytes), neighbors radio, and neighbors RSSI [@,radio,RSSI] x neighborsCount
const uint8_t cmonitor_cellList_path1[]         = "cl"; //here we return parent@ (last 2 Bytes), parent radio
const uint8_t cmonitor_kpi_path1[]         = "kpi"; ////here we return KPIS, namely pdr and latency: []
const uint8_t cmonitor_numTicsKpi_path1[]         = "dc"; ////here we return numTics KPIS (duty cycle DC),return total tics and tics On
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
      case TRACK_LIST:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_trackList_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_trackList_path1);
         break;
       case ROUTE_LIST:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_routeList_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_routeList_path1);
         break;
      case NUM_TICS:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_numTics_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_numTics_path1);
         break;
      case NEIGHBORS_LIST:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_nighborsList_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_nighborsList_path1);
         break;      
      case CELL_LIST:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_cellList_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_cellList_path1);
         break;
      case KPI_LIST:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_kpi_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_kpi_path1);
         break;
         case NUM_TICS_KPI:
         cmonitor_resource->desc.path1len   = sizeof(cmonitor_numTicsKpi_path1)-1;
         cmonitor_resource->desc.path1val   = (uint8_t*)(&cmonitor_numTicsKpi_path1);
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
         
         
         

//         leds_circular_shift();
  //       leds_sync_on();
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
   dagrank_t            MyDAGRANK;
   int8_t               neighbor_rssi;
   
    uint32_t             ticksTx;
    uint32_t             ticksOn;
    uint32_t             ticksTx_0;
    uint32_t             ticksOn_0;
    uint32_t             ticksTx_1;
    uint32_t             ticksOn_1;
    uint32_t             ticksTx_2;
    uint32_t             ticksOn_2;
    uint32_t             ticksInTotal;
    //uint8_t              current_cell_radio_setting;
    uint8_t              parrent_radio;
    uint8_t              neighbor_radio;
    uint8_t              neighbor_counter;
    
    //For Kpis if uinject receiver is not root
    uint16_t uinject_rx_counter; 
    uint16_t uinject_tx_counter; 
    uint32_t uinject_total_latency_counter; 

    uint16_t uinject_min_latency; 
    uint16_t uinject_max_latency; 
    
    
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
         
          case TRACK_LIST:       
          
          track_vars_t track_vars;
          uint8_t c1,c2,nb_subTracks;
          
          track_vars = track_getTracks();
          
          if(track_vars.nb_tracks == 0)
          {
          packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
              
              msg->payload[0] = 0; //return 0 if no track is already created
          } 
          else 
          {
          for (c1=1;c1<=track_vars.nb_tracks;c1++)

          if (track_vars.track_list[c1].track_id == c1){
                  
            nb_subTracks = track_vars.track_list[c1].num_subtracks;
            
            for (c2=1;c2<=nb_subTracks;c2++)
              
            { if (track_vars.track_list[c1].subtrack_list[c2].subtrack_id == c2)
            {
              packetfunctions_reserveHeaderSize(msg,10*sizeof(uint8_t));
              
              msg->payload[0] = track_vars.track_list[c1].track_id;
              msg->payload[1] = track_vars.track_list[c1].subtrack_list[c2].source_addr.addr_64b[6];
              msg->payload[2] = track_vars.track_list[c1].subtrack_list[c2].source_addr.addr_64b[7];
              msg->payload[3] = track_vars.track_list[c1].subtrack_list[c2].is_ingress;
              msg->payload[4] = track_vars.track_list[c1].subtrack_list[c2].is_egress;
              msg->payload[5] = track_vars.track_list[c1].subtrack_list[c2].cell_info.slotOffset;
              msg->payload[6] = track_vars.track_list[c1].subtrack_list[c2].cell_info.channelOffset;
              msg->payload[7] = track_vars.track_list[c1].subtrack_list[c2].cell_info.address.addr_64b[6];
              msg->payload[8] = track_vars.track_list[c1].subtrack_list[c2].cell_info.address.addr_64b[7];
              msg->payload[9] = track_vars.track_list[c1].subtrack_list[c2].cell_info.cellRadioSetting;
            
            }
           
          }
          } }//end else
          
          break; 
          
          
          case ROUTE_LIST:       
            
          //Return my DAGRank
          packetfunctions_reserveHeaderSize(msg,sizeof(uint16_t));
          MyDAGRANK = icmpv6rpl_getMyDAGrank(); 
          msg->payload[1] = (uint8_t)((MyDAGRANK & 0xff00) >> 8);
          msg->payload[0] = (uint8_t)( MyDAGRANK & 0x00ff);
          
         packetfunctions_reserveHeaderSize(msg,3*sizeof(uint8_t));

         foundNeighbor = icmpv6rpl_getPreferredParentKey(&parentNeighbor,&parrent_radio); //retrive parent address and radio
         if (foundNeighbor)
         {
         msg->payload[2] = parrent_radio;
         msg->payload[1] = parentNeighbor.addr_64b[7];
         msg->payload[0] = parentNeighbor.addr_64b[6];
         }
          
          break;
          
              
         case NEIGHBORS_LIST:
           
   
         //returning list of neighbors for each node:  liste:[@(last 2-bytes), radio, RSSI] + [num_neighbors] 
          packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
          numNeighbors = neighbors_getNumNeighbors();
          msg->payload[0] = numNeighbors;

         for (neighbor_counter = 0; neighbor_counter < numNeighbors; neighbor_counter++)
              {
                
                if(neighbor_counter < 13){ //Considering 12 is MAXNUM of Neighbors we autorise on the packets to not overflow
                
         packetfunctions_reserveHeaderSize(msg,4*sizeof(uint8_t));   
         foundNeighbor = neighbors_getNeighborKey(&Neighbor,ADDR_64B,&neighbor_radio,neighbor_counter);
         neighbor_rssi = neighbors_getRssi(neighbor_counter);
         if (foundNeighbor)
          {
            msg->payload[3] = neighbor_rssi;
            msg->payload[2] = neighbor_radio;
            msg->payload[1] = Neighbor.addr_64b[7];
            msg->payload[0] = Neighbor.addr_64b[6];
          }                               }
              }//END loop-for neighbors check
         
         break;
         
         
         
         case CELL_LIST:
         //returning list of celss for each [num_cells]+ liste:[slot,channel, is_hard,linkoption,@(last 2-bytes),cellRadioSetting]

         
          
          
          uint8_t i,num_cells;
          num_cells = 0;
          schedule_vars_t schedule_vars;
          
          schedule_vars = schedule_getScheduleVars();
          
          INTERRUPT_DECLARATION();
          DISABLE_INTERRUPTS();
                                            //MAX payload is 56
          for(i=0;i<MAXACTIVESLOTS;i++) {                                       //to avoid sending already known information (shared slots we know where it's scheduled)
          if((schedule_vars.scheduleBuf[i].type          != CELLTYPE_OFF)  && !( (schedule_vars.scheduleBuf[i].slotOffset == 0 && schedule_vars.scheduleBuf[i].channelOffset == 0)||(schedule_vars.scheduleBuf[i].slotOffset == 2 && schedule_vars.scheduleBuf[i].channelOffset == 0) || (schedule_vars.scheduleBuf[i].slotOffset == 6 && schedule_vars.scheduleBuf[i].channelOffset == 0)))
          
          //CELLTYPE_TXRX, CELLTYPE_RX, CELLTYPE_TX, CELLTYPE_OFF
           // schedule_vars.scheduleBuf[i].shared                         &&
           // schedule_vars.scheduleBuf[i].neighbor.type == ADDR_64B      &&
            
        {       if (num_cells<8){ //To not crash if we overload (MAX 56 entery)
            num_cells ++;
            packetfunctions_reserveHeaderSize(msg,7*sizeof(uint8_t)); 
            msg->payload[6] = schedule_vars.scheduleBuf[i].cellRadioSetting;
            msg->payload[5] = schedule_vars.scheduleBuf[i].neighbor.addr_64b[7];
            msg->payload[4] = schedule_vars.scheduleBuf[i].neighbor.addr_64b[6];
            msg->payload[3] = schedule_vars.scheduleBuf[i].type;
            msg->payload[2] = schedule_vars.scheduleBuf[i].isHardCell;
            msg->payload[1] = schedule_vars.scheduleBuf[i].channelOffset;
            msg->payload[0] = schedule_vars.scheduleBuf[i].slotOffset;
        }
            ENABLE_INTERRUPTS();

        }
           }//End fetching schedule

          ENABLE_INTERRUPTS();

          break;
          
          
         case NUM_TICS:

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

          //To process average duty cyle I dont need to send all details only On and total tics are needed
          
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
         
         break;
         
         case KPI_LIST:       
                          //packet format:[nTx(16B),nRx(16B),totalLatency(32B),MinLatency(16B),MaxLatency(16B)]
          //get Max recorded latency
          packetfunctions_reserveHeaderSize(msg,sizeof(uint16_t));
          uinject_max_latency = uinject_get_max_latency(); 
          msg->payload[1] = (uint8_t)((uinject_max_latency & 0xff00) >> 8);
          msg->payload[0] = (uint8_t)( uinject_max_latency & 0x00ff);
          
          //get Min recorded latency
          packetfunctions_reserveHeaderSize(msg,sizeof(uint16_t));
          uinject_min_latency = uinject_get_min_latency(); 
          msg->payload[1] = (uint8_t)((uinject_min_latency & 0xff00) >> 8);
          msg->payload[0] = (uint8_t)( uinject_min_latency & 0x00ff);
          
          //get total number of all recorded diff ASN
          packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
          uinject_total_latency_counter = uinject_get_total_latency_counter();      
          msg->payload[3] = (uint8_t)((uinject_total_latency_counter & 0xff000000) >> 24);
          msg->payload[2] = (uint8_t)((uinject_total_latency_counter & 0x00ff0000) >> 16);
          msg->payload[1] = (uint8_t)((uinject_total_latency_counter & 0x0000ff00) >> 8);
          msg->payload[0] = (uint8_t)( uinject_total_latency_counter & 0x000000ff);
          
          
          //get number of RX uinject packets
          packetfunctions_reserveHeaderSize(msg,sizeof(uint16_t));
          uinject_rx_counter = uinject_get_NumRx(); 
          msg->payload[1] = (uint8_t)((uinject_rx_counter & 0xff00) >> 8);
          msg->payload[0] = (uint8_t)( uinject_rx_counter & 0x00ff);
          
          //get number of TX uinject packets
          packetfunctions_reserveHeaderSize(msg,sizeof(uint16_t));
          uinject_tx_counter = uinject_get_NumTx(); 
          msg->payload[1] = (uint8_t)((uinject_tx_counter & 0xff00) >> 8);
          msg->payload[0] = (uint8_t)( uinject_tx_counter & 0x00ff);
          
          
          //reset the values after reporing the recodings
          uinject_reset_total_latency_counter();
          uinject_reset_max_latency();
          uinject_reset_min_latency();
          uinject_reset_NumTx();
          uinject_reset_NumRx();
          
          break;
          
          case NUM_TICS_KPI:

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

          //To process average duty cyle I dont need to send all details only On and total tics are needed

          packetfunctions_reserveHeaderSize(msg,sizeof(uint32_t));
          msg->payload[3] = (uint8_t)((ticksInTotal & 0xff000000) >> 24);
          msg->payload[2] = (uint8_t)((ticksInTotal & 0x00ff0000) >> 16);
          msg->payload[1] = (uint8_t)((ticksInTotal & 0x0000ff00) >> 8);
          msg->payload[0] = (uint8_t)( ticksInTotal & 0x000000ff);
          // resettin mac stats
          //ieee154e_resetStats();
         
         break;
          
          
        default:
         break; 
          
         } //switch END
       
/*
            
       
        
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
  //{debugpins_frame_toggle();debugpins_frame_toggle();}
   openqueue_freePacketBuffer(msg);
}



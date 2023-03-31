/**
\brief CoAP 6top application.

\author Xavi Vilajosana <xvilajosana@eecs.berkeley.edu>, February 2013.
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, July 2014
*/

#include "opendefs.h"
#include "cinstrument.h"
#include "sixtop.h"
#include "idmanager.h"
#include "openqueue.h"
#include "neighbors.h"
#include "msf.h"
#include "track.h"


#include "schedule.h"
//=========================== defines =========================================

const uint8_t cinstrument_path0[] = "ci";

//=========================== variables =======================================

cinstrument_vars_t cinstrument_vars;

//=========================== prototypes ======================================

owerror_t cinstrument_receive(OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
);
void    cinstrument_sendDone(
   OpenQueueEntry_t* msg,
   owerror_t         error
);

//=========================== public ==========================================

void cinstrument_init(void) {
   if(idmanager_getIsDAGroot()==TRUE) return;

   // prepare the resource descriptor for the /6t path
   cinstrument_vars.desc.path0len            = sizeof(cinstrument_path0)-1;
   cinstrument_vars.desc.path0val            = (uint8_t*)(&cinstrument_path0);
   cinstrument_vars.desc.path1len            = 0;
   cinstrument_vars.desc.path1val            = NULL;
   cinstrument_vars.desc.componentID         = COMPONENT_CINSTRUMENT;
   cinstrument_vars.desc.securityContext     = NULL;
   cinstrument_vars.desc.discoverable        = TRUE;
   cinstrument_vars.desc.callbackRx          = &cinstrument_receive;
   cinstrument_vars.desc.callbackSendDone    = &cinstrument_sendDone;

   opencoap_register(&cinstrument_vars.desc);
}

//=========================== private =========================================

/**
\brief Receives a command and a list of items to be used by the command.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t cinstrument_receive(OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
) {
   owerror_t            outcome;
   open_addr_t    neighbor;
   cellRadioSetting_t neighborRadio;
   bool           foundNeighbor;
   cellInfo_ht          celllist_add[CELLLIST_MAX_LEN];
   cellInfo_ht          celllist_delete[CELLLIST_MAX_LEN];
  //added by mm
    uint16_t  slotoffset;
    uint8_t        cellOptions;
   uint8_t return_value;
   
   switch (coap_header->Code) {

      case COAP_CODE_REQ_PUT:
         // add a slot
         
        
        // if (msg->payload[10] == 0x01)//means add track
         { return_value = track_installOrUpdateTrack(msg); }
        // else 
           //return_value = track_deleteTrack(msg);
         msg->payload                  = &(msg->packet[127]);
         msg->length                   = 0;
         
         //send success state 
         packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
         msg->payload[0] =  return_value;
         
         // set the CoAP header
         coap_header->Code             = COAP_CODE_RESP_CHANGED;
         
         if (return_value) outcome = E_SUCCESS; else outcome = E_FAIL;
         
         break;

      case COAP_CODE_REQ_DELETE:
         // delete a slot
          
        
                 return_value = track_deleteTrack(msg); 
         
         msg->payload                  = &(msg->packet[127]);
         msg->length                   = 0;
         
         //send success state 
         packetfunctions_reserveHeaderSize(msg,sizeof(uint8_t));
         msg->payload[0] =  return_value;
         
         // set the CoAP header
         coap_header->Code             = COAP_CODE_RESP_DELETED;//COAP_CODE_RESP_CHANGED;
         
         if (return_value) outcome = E_SUCCESS; else outcome = E_FAIL;
         
         break;
      default:
         outcome = E_FAIL;
         break;
   }

   return outcome;
}

void cinstrument_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

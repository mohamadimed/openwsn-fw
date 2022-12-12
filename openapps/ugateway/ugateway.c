/**
\brief uGateway is a simple gateway app for Openmote B. It listens to the 2.4-GHz cc2538 radio and it forwards the cinoming packets as UDP packets over g6TiSCH network.

\author Mina Rady mina1.rady@orange.com>, November 2020.

*/

#include "stdint.h"
#include "string.h"
#include "board.h"
#include "radio_cc2538rf.h"
#include "leds.h"
#include "debugpins.h"
#include "uart.h"
#include "uinject.h"
#include "ugateway.h"
#include "openqueue.h"
#include "opendefs.h"
#include "schedule.h"
#include "neighbors.h"
#include "IEEE802154E.h"
#include "openserial.h"
#include "idmanager.h"
//#include "sctimer.h"

//=========================== defines =========================================

#define LENGTH_PACKET        125+LENGTH_CRC // maximum length is 127 bytes
#define CHANNEL              11             // 24ghz: 11 = 2.405GHz, subghz: 11 = 865.325 in  FSK operating mode #1
#define LENGTH_SERIAL_FRAME  9              // length of the serial frame

//=========================== variables =======================================
uinject_vars_t ugateway_vars;

static const uint8_t ugateway_payload[]    = "gateway";
static const uint8_t ugateway_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};


typedef struct {
    uint8_t    num_radioTimerCompare;
    uint8_t    num_startFrame;
    uint8_t    num_endFrame;
} app_dbg_t;

app_dbg_t app_dbg;

typedef struct {
    // rx packet
    volatile    uint8_t    rxpk_done;
                uint8_t    rxpk_buf[LENGTH_PACKET];
                uint8_t    rxpk_len;
                uint8_t    rxpk_num;
                int8_t     rxpk_rssi;
                uint8_t    rxpk_lqi;
                bool       rxpk_crc;
                uint8_t    rxpk_freq_offset;
    // uart
                uint8_t    uart_txFrame[LENGTH_SERIAL_FRAME];
                uint8_t    uart_lastTxByte;
    volatile    uint8_t    uart_done;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

// radiotimer
void cb_radioTimerOverflows(void);
// radio
void cb_startFrame(PORT_TIMER_WIDTH timestamp);
void cb_endFrame(PORT_TIMER_WIDTH timestamp);

void ugateway_send(void);
//=========================== main ============================================

/**
\brief The program starts executing here.
*/
void ugateway_init (void) {
    // clear local variables
    memset(&app_vars,0,sizeof(app_vars_t));
    memset(&ugateway_vars,0,sizeof(uinject_vars_t));

    // register at UDP stack
    ugateway_vars.desc.port              = WKP_UDP_INJECT;
    ugateway_vars.desc.callbackReceive   = &ugateway_receive;
    ugateway_vars.desc.callbackSendDone  = &ugateway_sendDone;
    openudp_register(&ugateway_vars.desc);

    // prepare radio
    radio_setConfig_cc2538rf (RADIOSETTING_24GHZ);
    radio_rfOn_cc2538rf();
    // freq type only effects on scum port
    radio_setFrequency_cc2538rf(CHANNEL, FREQ_RX);
        // add callback functions radio
    radio_setStartFrameCb_cc2538rf(cb_startFrame);
    radio_setEndFrameCb_cc2538rf(cb_endFrame);
    // switch in RX
    radio_rxEnable_cc2538rf();
    radio_rxNow_cc2538rf();
    debugpins_radio_clr();

    // prepare uinject packet

    // clear local variables




}

//=========================== callbacks =======================================

//===== radio

void cb_startFrame(PORT_TIMER_WIDTH timestamp) {
    debugpins_radio_toggle();
    debugpins_fsm_set();
    leds_sync_on();
    // update debug stats
    app_dbg.num_startFrame++;
}

void cb_endFrame(PORT_TIMER_WIDTH timestamp) {
    debugpins_radio_toggle();
    debugpins_fsm_clr();
    uint8_t  i;
    bool     expectedFrame;

    // update debug stats
    app_dbg.num_endFrame++;

    memset(&app_vars.rxpk_buf[0],0,LENGTH_PACKET);

    app_vars.rxpk_freq_offset = radio_getFrequencyOffset_cc2538rf();

    // get packet from radio
    radio_getReceivedFrame_cc2538rf(
        app_vars.rxpk_buf,
        &app_vars.rxpk_len,
        sizeof(app_vars.rxpk_buf),
        &app_vars.rxpk_rssi,
        &app_vars.rxpk_lqi,
        &app_vars.rxpk_crc
    );

    // check the frame is sent by radio_tx project
    expectedFrame = TRUE;

    if (app_vars.rxpk_len>LENGTH_PACKET){
        expectedFrame = FALSE;
    } else {
        for(i=1;i<10;i++){
            if(app_vars.rxpk_buf[i]!=i){
                expectedFrame = FALSE;
                break;
            }
        }
    }

    // read the packet number
    app_vars.rxpk_num = app_vars.rxpk_buf[0];

    // toggle led if the frame is expected
    if (expectedFrame){
        // indicate I just received a packet from bsp_radio_tx mote
        app_vars.rxpk_done = 1;

        leds_debug_toggle();
        app_vars.uart_txFrame[i++] = app_vars.rxpk_len;  // packet length
        app_vars.uart_txFrame[i++] = app_vars.rxpk_num;
        app_vars.uart_txFrame[i++] = app_vars.rxpk_rssi;


        // form the UDP packet
        // send udp
        ugateway_send();
        // return to listening

        // keep listening (needed for at86rf215 radio)
        radio_rxEnable_cc2538rf();
        radio_rxNow_cc2538rf();

    }

    // led
    leds_sync_off();
}


void ugateway_sendDone(OpenQueueEntry_t* msg, owerror_t error) {

    if (error==E_FAIL){
        openserial_printError(
            COMPONENT_UINJECT,
            ERR_MAXRETRIES_REACHED,
            (errorparameter_t)ugateway_vars.counter,
            (errorparameter_t)0
        );
    }

    // free the packet buffer entry
    openqueue_freePacketBuffer(msg);

    // allow send next uinject packet
    ugateway_vars.busySendingUinject = FALSE;

    // clear local variables
    memset(&app_vars,0,sizeof(app_vars_t));
}

void ugateway_receive(OpenQueueEntry_t* pkt) {

    openqueue_freePacketBuffer(pkt);
}

//=========================== private =========================================



void ugateway_send(void) {
    OpenQueueEntry_t*    pkt;
    uint8_t              asnArray[5];
    uint8_t i = 0;
    open_addr_t          parentNeighbor;
    bool                 foundNeighbor;

    // don't run if not synch
    if (ieee154e_isSynch() == FALSE) {
        return;
    }

    // don't run on dagroot
    if (idmanager_getIsDAGroot()) {
        opentimers_destroy(ugateway_vars.timerId);
        return;
    }

    foundNeighbor = icmpv6rpl_getPreferredParentEui64(&parentNeighbor);
    if (foundNeighbor==FALSE) {
        return;
    }

    if (schedule_hasNegotiatedCellToNeighbor(&parentNeighbor, CELLTYPE_TX) == FALSE) {
        return;
    }

    if (ugateway_vars.busySendingUinject==TRUE) {
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
    
    memcpy(&pkt->l3_destinationAdd.addr_128b[0],ugateway_dst_addr,16);

    // add payload
    packetfunctions_reserveHeaderSize(pkt,sizeof(ugateway_payload)-1);
    memcpy(&pkt->payload[0],ugateway_payload,sizeof(ugateway_payload)-1);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    pkt->payload[1] = (uint8_t)((ugateway_vars.counter & 0xff00)>>8);
    pkt->payload[0] = (uint8_t)(ugateway_vars.counter & 0x00ff);
    ugateway_vars.counter++;

    packetfunctions_reserveHeaderSize(pkt,sizeof(asn_t));
    ieee154e_getAsn(asnArray);
    pkt->payload[0] = asnArray[0];
    pkt->payload[1] = asnArray[1];
    pkt->payload[2] = asnArray[2];
    pkt->payload[3] = asnArray[3];
    pkt->payload[4] = asnArray[4];

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    pkt->payload[1] = (uint8_t)(idmanager_getMyID(ADDR_16B)->addr_16b[0]);
    pkt->payload[0] = (uint8_t)(idmanager_getMyID(ADDR_16B)->addr_16b[1]);

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
    pkt->payload[0] = app_vars.rxpk_len;

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
    pkt->payload[0] = app_vars.rxpk_num;

    packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
    pkt->payload[0] = app_vars.rxpk_rssi;

    packetfunctions_reserveHeaderSize(pkt,sizeof(app_vars.rxpk_len));
    for (i=0;i<app_vars.rxpk_len ; i++)
    {
        pkt->payload[i] = app_vars.rxpk_buf[i];

    }
    
    if ((openudp_send(pkt))==E_FAIL) {
        openqueue_freePacketBuffer(pkt);
    } else {
        // set busySending to TRUE
        ugateway_vars.busySendingUinject = TRUE;
    }
}



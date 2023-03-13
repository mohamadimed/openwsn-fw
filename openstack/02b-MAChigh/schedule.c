#include "opendefs.h"
#include "schedule.h"
#include "openserial.h"
#include "openrandom.h"
#include "packetfunctions.h"
#include "sixtop.h"
#include "idmanager.h"
#include "msf.h"
#include "IEEE802154E.h"
#include "icmpv6rpl.h"
#include "neighbors.h"
// telosb need debugpins to indicate the ISR activity
#include "debugpins.h"

//=========================== definition ======================================

//=========================== variables =======================================

schedule_vars_t schedule_vars;

//=========================== prototypes ======================================

void schedule_resetEntry(scheduleEntry_t* pScheduleEntry);
void schedule_resetBackupEntry(backupEntry_t* pBackupEntry);

//=========================== public ==========================================

//=== admin

/**
\brief Initialize this module.

\post Call this function before calling any other function in this module.
*/
void schedule_init(void) {
    uint8_t i;
    uint8_t running_slotOffset;

    // reset local variables
    memset(&schedule_vars,0,sizeof(schedule_vars_t));
    for (running_slotOffset=0;running_slotOffset<MAXACTIVESLOTS;running_slotOffset++) {
        schedule_resetEntry(&schedule_vars.scheduleBuf[running_slotOffset]);
        for (i=0;i<MAXBACKUPSLOTS;i++){
            schedule_resetBackupEntry(&schedule_vars.scheduleBuf[running_slotOffset].backupEntries[i]);
        }
    }
    schedule_vars.backoffExponenton   = MINBE-1;
    schedule_vars.maxActiveSlots = MAXACTIVESLOTS;

    if (idmanager_getIsDAGroot()==TRUE) {
        schedule_startDAGroot();
    }
}

/**
\brief Starting the DAGroot schedule propagation.
*/
void schedule_startDAGroot(void) {
    slotOffset_t    start_slotOffset;
    slotOffset_t    running_slotOffset;
    open_addr_t     temp_neighbor;

    start_slotOffset = SCHEDULE_MINIMAL_6TISCH_SLOTOFFSET;
    // set frame length, handle and number (default 1 by now)
    if (schedule_vars.frameLength == 0) {
        // slotframe length is not set, set it to default length
        schedule_setFrameLength(SLOTFRAME_LENGTH);
    } else {
        // slotframe length is set, nothing to do here
    }
    schedule_setFrameHandle(SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_HANDLE);
    schedule_setFrameNumber(SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_NUMBER);

    // shared TXRX anycast slot(s)
    memset(&temp_neighbor,0,sizeof(temp_neighbor));
    temp_neighbor.type             = ADDR_ANYCAST;
    running_slotOffset             = start_slotOffset;
//    for (running_slotOffset=start_slotOffset;running_slotOffset<start_slotOffset+SCHEDULE_MINIMAL_6TISCH_ACTIVE_CELLS;running_slotOffset++) {
    for (uint8_t i = 0 ; i < SCHEDULE_MINIMAL_6TISCH_ACTIVE_CELLS;i++) {
        schedule_addActiveSlot(
            running_slotOffset,                     // slot offset
            CELLTYPE_TXRX,                          // type of slot
            TRUE,                                   // shared?
            FALSE,                                  // auto cell?
            CELLRADIOSETTING_1  + i,                // cell radio setting
            SCHEDULE_MINIMAL_6TISCH_CHANNELOFFSET,  // channel offset
            &temp_neighbor,                                                  //Neighbor
            TRUE                                                             //Hard Cell? consider Minimal cell as hard cell (RFC 8480)
        );
        running_slotOffset             += superSlotLengthMap[i];
    }
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_schedule(void) {

   debugScheduleEntry_t temp;

   // increment the row just printed
   schedule_vars.debugPrintRow         = (schedule_vars.debugPrintRow+1)%schedule_vars.maxActiveSlots;

   // gather status data
   temp.row                            = schedule_vars.debugPrintRow;
   temp.slotOffset                     = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].slotOffset;
   temp.type                           = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].type;
   temp.shared                         = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].shared;
   temp.channelOffset                  = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].channelOffset;
   temp.cellRadioSetting               = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].cellRadioSetting;
   memcpy(
      &temp.neighbor,
      &schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].neighbor,
      sizeof(open_addr_t)
   );
   temp.numRx                          = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].numRx;
   temp.numTx                          = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].numTx;
   temp.numTxACK                       = \
      schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].numTxACK;
   memcpy(
      &temp.lastUsedAsn,
      &schedule_vars.scheduleBuf[schedule_vars.debugPrintRow].lastUsedAsn,
      sizeof(asn_t)
   );

   // send status data over serial port
   openserial_printStatus(
      STATUS_SCHEDULE,
      (uint8_t*)&temp,
      sizeof(debugScheduleEntry_t)
   );

   return TRUE;
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_backoff(void) {
   uint8_t temp[2];

   // gather status data
   temp[0] = schedule_vars.backoffExponenton;
   temp[1] = schedule_vars.backoff;

   // send status data over serial port
   openserial_printStatus(
      STATUS_BACKOFF,
      (uint8_t*)&temp,
      sizeof(temp)
   );

   return TRUE;
}

//=== from 6top (writing the schedule)

/**
\brief Set frame length.

\param newFrameLength The new frame length.
*/
void schedule_setFrameLength(frameLength_t newFrameLength) {

   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   schedule_vars.frameLength = newFrameLength;
   if (newFrameLength <= MAXACTIVESLOTS) {
      schedule_vars.maxActiveSlots = newFrameLength;
   }
   ENABLE_INTERRUPTS();
}

/**
\brief Set frame handle.

\param frameHandle The new frame handle.
*/
void schedule_setFrameHandle(uint8_t frameHandle) {

   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   schedule_vars.frameHandle = frameHandle;

   ENABLE_INTERRUPTS();
}

/**
\brief Set frame number.

\param frameNumber The new frame number.
*/
void schedule_setFrameNumber(uint8_t frameNumber) {

   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   schedule_vars.frameNumber = frameNumber;

   ENABLE_INTERRUPTS();
}

/**
\brief Get the information of a specific slot.

\param slotOffset
\param info
*/
void  schedule_getSlotInfo(
    slotOffset_t         slotOffset,
    slotinfo_element_t*  info
){

    scheduleEntry_t* slotContainer;

    // find an empty schedule entry container
    slotContainer = &schedule_vars.scheduleBuf[0];
    while (slotContainer<=&schedule_vars.scheduleBuf[schedule_vars.maxActiveSlots-1]) {
       //check that this entry for that neighbour and timeslot is not already scheduled.
       if (slotContainer->slotOffset==slotOffset){
               //it exists so this is an update.
               info->link_type                 = slotContainer->type;
               info->shared                    = slotContainer->shared;
               info->slotOffset                = slotOffset;
               info->cellRadioSetting         = slotContainer->cellRadioSetting;
               info->channelOffset             = slotContainer->channelOffset;
               info->isAutoCell                = slotContainer->isAutoCell;
               info->isHardCell                = slotContainer->isHardCell;
               memcpy(&(info->address), &(slotContainer->neighbor), sizeof(open_addr_t));
               return; //as this is an update. No need to re-insert as it is in the same position on the list.
        }
        slotContainer++;
    }
    // return cell type off
    info->link_type                 = CELLTYPE_OFF;
    info->shared                    = FALSE;
    info->channelOffset             = 0;        //set to zero if not set.
    info->cellRadioSetting         = 0;
    info->isAutoCell                = FALSE;
    info->isHardCell                = FALSE;
    memset(&(info->address), 0, sizeof(open_addr_t));
}

/**
\brief Add a new active slot into the schedule.

\param slotOffset       The slotoffset of the new slot
\param type             The type of the cell
\param shared           Whether this cell is shared (TRUE) or not (FALSE).
\param channelOffset    The channelOffset of the new slot
\param neighbor         The neighbor associated with this cell (all 0's if
   none)
*/
owerror_t schedule_addActiveSlot(
    slotOffset_t    slotOffset,
    cellType_t      type,
    bool            shared,
    bool            isAutoCell,
    cellRadioSetting_t cellRadioSetting,
    channelOffset_t channelOffset,
    open_addr_t*    neighbor,
    bool            isHardCell
) {
    uint8_t asn[5];
    scheduleEntry_t* slotContainer;
    scheduleEntry_t* startSlot;
    scheduleEntry_t* previousSlotWalker;
    scheduleEntry_t* nextSlotWalker;

    backupEntry_t*   backupEntry;

    uint8_t          i;
    bool             entry_found;
    bool             inBackupEntries;

    bool             needSwapEntries;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    // find an empty schedule entry container
    entry_found     = FALSE;
    inBackupEntries = FALSE;
    slotContainer = &schedule_vars.scheduleBuf[0];
    do {
        if (slotContainer->type != CELLTYPE_OFF) {
            if (slotContainer->slotOffset == slotOffset) {
                // found one entry with same slotoffset in schedule
                // check if there is space in second entries

                for (i=0;i<MAXBACKUPSLOTS;i++) {
                    if (slotContainer->backupEntries[i].type == CELLTYPE_OFF) {
                        inBackupEntries = TRUE;
                        backupEntry     = &(slotContainer->backupEntries[i]);
                        break;
                    }
                }
                if (inBackupEntries) {
                    entry_found = TRUE;
                    break;
                }
            }
            slotContainer++;
        } else {
            entry_found = TRUE;
            break;
        }
    } while (slotContainer <= &schedule_vars.scheduleBuf[schedule_vars.maxActiveSlots-1]);

    // abort it schedule overflow
    if (entry_found == FALSE) {
        ENABLE_INTERRUPTS();
        openserial_printError(
            COMPONENT_SCHEDULE,ERR_SCHEDULE_OVERFLOWN,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return E_FAIL;
    }


    // assign the next slot pointer if it's allocated in backup entries
    if (inBackupEntries) {

        // the highest priority cell should be in schedule
        // priority  high ----------------- low
        //          autoTx  -> autoRx -> negotiated

        // check that whether need to swap the entries
        needSwapEntries = FALSE;
        if (slotContainer->isAutoCell) {
            if (
                isAutoCell                              &&
                slotContainer->type == CELLTYPE_RX      &&
                type                == CELLTYPE_TX
            ){
                // swap the entry of schedule and backup schedule
                needSwapEntries = TRUE;
            }
        } else {
            if (isAutoCell){
                // swap the entry of schedule and backup schedule
                needSwapEntries = TRUE;
            }
        }

        if (needSwapEntries) {

            // backup current entries
            backupEntry->type              = slotContainer->type;
            backupEntry->shared            = slotContainer->shared;
            backupEntry->channelOffset     = slotContainer->channelOffset;
            backupEntry->cellRadioSetting  = slotContainer->cellRadioSetting;
            backupEntry->isAutoCell        = slotContainer->isAutoCell;
            backupEntry->isHardCell        = slotContainer->isHardCell;
            

            memcpy(&(backupEntry->neighbor), &(slotContainer->neighbor), sizeof(open_addr_t));

            backupEntry->numRx                  = slotContainer->numRx;
            backupEntry->numTx                  = slotContainer->numTx;
            backupEntry->numTxACK               = slotContainer->numTxACK;
            backupEntry->lastUsedAsn.byte4      = slotContainer->lastUsedAsn.byte4;
            backupEntry->lastUsedAsn.bytes0and1 = slotContainer->lastUsedAsn.bytes0and1;
            backupEntry->lastUsedAsn.bytes0and1 = slotContainer->lastUsedAsn.bytes0and1;
            backupEntry->next                   = slotContainer->next;

            // add cell to schedule
            slotContainer->type                 = type;
            slotContainer->shared               = shared;
            slotContainer->channelOffset        = channelOffset;
            slotContainer->isAutoCell           = isAutoCell;
            slotContainer->isHardCell           = isHardCell;
            slotContainer->cellRadioSetting     = cellRadioSetting;
            memcpy(&(slotContainer->neighbor),neighbor,sizeof(open_addr_t));

            // fill that schedule entry with current asn
            ieee154e_getAsn(&(asn[0]));
            slotContainer->lastUsedAsn.bytes0and1 = 256*asn[1]+asn[0];
            slotContainer->lastUsedAsn.bytes2and3 = 256*asn[3]+asn[2];
            slotContainer->lastUsedAsn.byte4      = asn[4];
        } else {
            // add cell to backup schedule

            backupEntry->type                       = type;
            backupEntry->shared                     = shared;
            backupEntry->channelOffset              = channelOffset;
            backupEntry->isAutoCell                 = isAutoCell;
            backupEntry->isHardCell                 = isHardCell;
            backupEntry->cellRadioSetting           = cellRadioSetting;
            memcpy(&backupEntry->neighbor,neighbor,sizeof(open_addr_t));

            // fill that schedule entry with current asn
            ieee154e_getAsn(&(asn[0]));
            backupEntry->lastUsedAsn.bytes0and1     = 256*asn[1]+asn[0];
            backupEntry->lastUsedAsn.bytes2and3     = 256*asn[3]+asn[2];
            backupEntry->lastUsedAsn.byte4          = asn[4];

            // use the same next point in schedule
            backupEntry->next = slotContainer->next;
        }
        ENABLE_INTERRUPTS();
        return E_SUCCESS;
    }

    // fill that schedule entry with parameters passed
    slotContainer->slotOffset                = slotOffset;
    slotContainer->type                      = type;
    slotContainer->shared                    = shared;
    slotContainer->channelOffset             = channelOffset;
    slotContainer->isAutoCell                = isAutoCell;
    slotContainer->isHardCell                = isHardCell;
    slotContainer->cellRadioSetting          = cellRadioSetting;
    slotContainer->superSlotLength           = superSlotLengthMap[cellRadioSetting];
    memcpy(&(slotContainer->neighbor),neighbor,sizeof(open_addr_t));

    // fill that schedule entry with current asn
    ieee154e_getAsn(&(asn[0]));
    slotContainer->lastUsedAsn.bytes0and1 = 256*asn[1]+asn[0];
    slotContainer->lastUsedAsn.bytes2and3 = 256*asn[3]+asn[2];
    slotContainer->lastUsedAsn.byte4      = asn[4];

    // insert in circular list
    if (schedule_vars.currentScheduleEntry==NULL) {
        // this is the first active slot added

        // the next slot of this slot is this slot
        slotContainer->next                   = slotContainer;

        // current slot points to this slot
        schedule_vars.currentScheduleEntry    = slotContainer;
    } else  {
        // this is NOT the first active slot added

        // find position in schedule
        
        //this is where the magic will happen
      startSlot                             = schedule_vars.currentScheduleEntry;  
      previousSlotWalker                    = schedule_vars.currentScheduleEntry;
        
        while (1) {
            nextSlotWalker                    = previousSlotWalker->next;
            if (
                (
                    (previousSlotWalker->slotOffset + previousSlotWalker->superSlotLength-1 <  slotContainer->slotOffset) &&
                    (slotContainer->slotOffset+slotContainer->superSlotLength-1 <  nextSlotWalker->slotOffset)
                )
                ||
                (
                    (previousSlotWalker->slotOffset +previousSlotWalker->superSlotLength-1 <  slotContainer->slotOffset) &&
                    (nextSlotWalker->slotOffset <= previousSlotWalker->slotOffset)
                )
                ||
                (
                    (slotContainer->slotOffset + slotContainer->superSlotLength-1 <  nextSlotWalker->slotOffset) &&
                    (nextSlotWalker->slotOffset <= previousSlotWalker->slotOffset)
                )
            ) {
                break;
            }
            if (previousSlotWalker->slotOffset == slotContainer->slotOffset) {
                // slot is already in schedule
                
                openserial_printError(
                   COMPONENT_SCHEDULE,ERR_SCHEDULE_ADDDUPLICATESLOT,
                   (errorparameter_t)slotContainer->slotOffset,
                   (errorparameter_t)0
                );
                // reset the entry
                slotContainer->slotOffset                = 0;
                slotContainer->type                      = CELLTYPE_OFF;
                slotContainer->shared                    = FALSE;
                slotContainer->channelOffset             = 0;
                slotContainer->cellRadioSetting          = 0;
                memset(&slotContainer->neighbor,0,sizeof(open_addr_t));
                ENABLE_INTERRUPTS();
                return E_FAIL;
            }
            previousSlotWalker                 = nextSlotWalker;
            
            // I have returned to the beginning of the schdule, no place found (i.e. partial conflict with another superSlot)
            if (startSlot->slotOffset == previousSlotWalker->slotOffset){
             openserial_printError( // create different error message for partial conflict
                   COMPONENT_SCHEDULE,ERR_SCHEDULE_ADDDUPLICATESLOT,
                   (errorparameter_t)slotContainer->slotOffset,
                   (errorparameter_t)0
                );
                // reset the entry
                slotContainer->slotOffset                = 0;
                slotContainer->type                      = CELLTYPE_OFF;
                slotContainer->shared                    = FALSE;
                slotContainer->channelOffset             = 0;
                slotContainer->cellRadioSetting          = 0;
                memset(&slotContainer->neighbor,0,sizeof(open_addr_t));
                ENABLE_INTERRUPTS();
                return E_FAIL;
            }
        }
        // insert between previousSlotWalker and nextSlotWalker
        
        previousSlotWalker->next              = slotContainer; 
        slotContainer->next                   = nextSlotWalker;
    }

    ENABLE_INTERRUPTS();
    return E_SUCCESS;
}

/**
\brief Remove an active slot from the schedule.

\param slotOffset       The slotoffset of the slot to remove.
\param type             The type of the slot to remove.
\param isShared         The slot is shared or not.
\param neighbor         The neighbor associated with this cell (all 0's if
   none)
*/
owerror_t schedule_removeActiveSlot(
    slotOffset_t slotOffset,
    cellType_t   type,
    bool         isShared,
    open_addr_t* neighbor
) {
    uint8_t          i;
    bool             entry_found;
    bool             isbackupEntry;
    backupEntry_t*   backupEntry;
    uint8_t          candidate_index;

    scheduleEntry_t* slotContainer;
    scheduleEntry_t* previousSlotWalker;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();


    // find the schedule entry
    entry_found        = FALSE;
    isbackupEntry      = FALSE;
    slotContainer = &schedule_vars.scheduleBuf[0];
    while (slotContainer<=&schedule_vars.scheduleBuf[schedule_vars.maxActiveSlots-1]) {
        if (slotContainer->slotOffset==slotOffset) {

            if (packetfunctions_sameAddress(neighbor,&(slotContainer->neighbor))) {
                entry_found = TRUE;
                break;
            } else {
                for (i=0;i<MAXBACKUPSLOTS;i++){
                    if (
                        packetfunctions_sameAddress(neighbor,&(slotContainer->backupEntries[i].neighbor))  &&
                        type     == slotContainer->backupEntries[i].type                                   &&
                        isShared == slotContainer->backupEntries[i].shared
                    ) {
                        isbackupEntry = TRUE;
                        backupEntry = &(slotContainer->backupEntries[i]);
                        break;
                    }
                }
                if (isbackupEntry) {
                    entry_found = TRUE;
                    break;
                }
            }
        }
        slotContainer++;
    }

    // abort it could not find
    if (entry_found == FALSE) {
        ENABLE_INTERRUPTS();
        openserial_printCritical(
            COMPONENT_SCHEDULE,ERR_FREEING_ERROR,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return E_FAIL;
    }

    if (isbackupEntry) {

        // reset the backup entry
        backupEntry->type                   = CELLTYPE_OFF;
        backupEntry->shared                 = FALSE;
        backupEntry->channelOffset          = 0;
        backupEntry->cellRadioSetting       = 0;
        backupEntry->neighbor.type          = ADDR_NONE;
        memset(&backupEntry->neighbor.addr_64b[0], 0x00, sizeof(backupEntry->neighbor.addr_64b));

        backupEntry->lastUsedAsn.bytes0and1 = 0;
        backupEntry->lastUsedAsn.bytes2and3 = 0;
        backupEntry->lastUsedAsn.byte4      = 0;
        backupEntry->next                   = NULL;

        ENABLE_INTERRUPTS();
        return E_SUCCESS;
    } else {
        // looking for a cell in backup entries
        candidate_index = MAXBACKUPSLOTS;
        for (i=0;i<MAXBACKUPSLOTS;i++){
            if (slotContainer->backupEntries[i].type != CELLTYPE_OFF) {
                candidate_index = i;
                if (
                    slotContainer->backupEntries[i].isAutoCell       &&
                    slotContainer->backupEntries[i].type == CELLTYPE_TX
                ){
                    break;
                }
            }
        }

        if (candidate_index < MAXBACKUPSLOTS){
            // move the backup entry to the schedule
            slotContainer->type                      = slotContainer->backupEntries[candidate_index].type;
            slotContainer->shared                    = slotContainer->backupEntries[candidate_index].shared;
            slotContainer->cellRadioSetting          = slotContainer->backupEntries[candidate_index].cellRadioSetting;
            slotContainer->channelOffset             = slotContainer->backupEntries[candidate_index].channelOffset;
            slotContainer->isAutoCell                = slotContainer->backupEntries[candidate_index].isAutoCell;
            slotContainer->isHardCell                = slotContainer->backupEntries[candidate_index].isHardCell;
            memcpy(&slotContainer->neighbor,&(slotContainer->backupEntries[candidate_index].neighbor),sizeof(open_addr_t));

            slotContainer->numTx                     =  slotContainer->backupEntries[candidate_index].numTx;
            slotContainer->numRx                     =  slotContainer->backupEntries[candidate_index].numRx;
            slotContainer->numTxACK                  =  slotContainer->backupEntries[candidate_index].numTxACK;
            slotContainer->lastUsedAsn.bytes0and1    =  slotContainer->backupEntries[candidate_index].lastUsedAsn.bytes0and1;
            slotContainer->lastUsedAsn.bytes2and3    =  slotContainer->backupEntries[candidate_index].lastUsedAsn.bytes2and3;
            slotContainer->lastUsedAsn.byte4         =  slotContainer->backupEntries[candidate_index].lastUsedAsn.byte4;

            // reset the backup entry
            schedule_resetBackupEntry(&(slotContainer->backupEntries[candidate_index]));

            ENABLE_INTERRUPTS();
            return E_SUCCESS;
        } else {
            // no backup cell found
        }
    }

    // remove from linked list
    if (slotContainer->next==slotContainer) {
        // this is the last active slot

        // the next slot of this slot is NULL
        slotContainer->next                   = NULL;

        // current slot points to this slot
        schedule_vars.currentScheduleEntry    = NULL;
    } else {
        // this is NOT the last active slot

        // find the previous in the schedule
        previousSlotWalker                    = schedule_vars.currentScheduleEntry;
        uint8_t cc = 0;
        while (1) {
            cc++;
            if (previousSlotWalker->next==slotContainer){
                break;
            }
            previousSlotWalker                 = previousSlotWalker->next;
            if (cc>210){//poipoi
         
                openserial_printInfo(
                COMPONENT_SCHEDULE,
                ERR_WRONG_ADDR_TYPE,
                (errorparameter_t)slotOffset,
                (errorparameter_t)888
                    );
                  }
              }

        // remove this element from the linked list, i.e. have the previous slot
        // "jump" to slotContainer's next
        previousSlotWalker->next              = slotContainer->next;

        // update current slot if points to slot I just removed
        if (schedule_vars.currentScheduleEntry==slotContainer) {
            /**
                attention: this should only happen at the end of slot.
                it's dangerous to remove current schedule entry in the middle
                of the slot. The item access of currentScheduleEntry could
                be from unexpected entry.

                In case the entry is removed at endSlot(), the currentSCheduleEntry
                should be the previous entry. This is because when the next active
                slot arrives, currentSCheduleEntry will be assigned as
                currentSCheduleEntry->next
            */
            schedule_vars.currentScheduleEntry = previousSlotWalker;
        }
   }

    // reset removed schedule entry
    schedule_resetEntry(slotContainer);

    ENABLE_INTERRUPTS();

    return E_SUCCESS;
}

bool schedule_isSlotOffsetAvailable(uint16_t slotOffset, uint8_t superSlotLength){

   scheduleEntry_t* scheduleWalker;
   int16_t precedingMargin;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
  // Check if the superSlot does not extend beyond the slotFrame
   if (
        slotOffset>=schedule_vars.frameLength || 
        slotOffset+superSlotLength >= schedule_vars.frameLength
       ){
      ENABLE_INTERRUPTS();
      return FALSE;
   }

   scheduleWalker = schedule_vars.currentScheduleEntry;
   do {
        // Check if the superSlot does not conflict with any other superSlot
        // int is needed in case precedingMargin is negative (i.e. previous frame)
        precedingMargin =  scheduleWalker->slotOffset - superSlotLength;
      if(
         precedingMargin < slotOffset && 
         slotOffset < scheduleWalker->slotOffset+ scheduleWalker->superSlotLength
         ){
          ENABLE_INTERRUPTS();
          return FALSE;
      }
      scheduleWalker = scheduleWalker->next;
   }while(scheduleWalker!=schedule_vars.currentScheduleEntry);

   ENABLE_INTERRUPTS();

   return TRUE;
}

void schedule_removeAllNegotiatedCellsToNeighbor(
    uint8_t        slotframeID,
    open_addr_t*   neighbor,
    cellRadioSetting_t neighborRadio
){
    uint8_t i;

    // remove all entries in schedule with previousHop address
    for(i=0;i<MAXACTIVESLOTS;i++){
        if (
            packetfunctions_sameAddress(&(schedule_vars.scheduleBuf[i].neighbor),neighbor) &&
              schedule_vars.scheduleBuf[i].cellRadioSetting == neighborRadio               &&
            (
                schedule_vars.scheduleBuf[i].type == CELLTYPE_TX ||
                schedule_vars.scheduleBuf[i].type == CELLTYPE_RX ||
                schedule_vars.scheduleBuf[i].isHardCell == FALSE
            )
        ){
            schedule_removeActiveSlot(
                schedule_vars.scheduleBuf[i].slotOffset,
                schedule_vars.scheduleBuf[i].type,
                schedule_vars.scheduleBuf[i].shared,
                neighbor
            );
        }
    }
}

// this sort of loses its meaning now
// unless you pass it a parameter of superSlotLength.
uint8_t schedule_getNumberOfFreeEntries(){
    uint8_t i;
    uint8_t counter;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    counter = 0;
    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(schedule_vars.scheduleBuf[i].type == CELLTYPE_OFF){
            counter++;
        }
     }

     ENABLE_INTERRUPTS();
    return counter;
}



//added by mm to fetch schedule and get all enteries by cmonitor
// this sort of loses its meaning now
// unless you pass it a parameter of superSlotLength.
schedule_vars_t schedule_getScheduleVars(){

    return  schedule_vars;
}


// New function to get the number of available superSlot entries
// author: Mina Rady
uint8_t schedule_getNumberOfSuperSlotFreeEntries( uint8_t requestedSlotLength){
   scheduleEntry_t* scheduleWalker;
   scheduleEntry_t* nextScheduleWalker;
   uint8_t counter=0;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   scheduleWalker = schedule_vars.currentScheduleEntry;
   nextScheduleWalker = scheduleWalker->next;
   do {

      if(scheduleWalker->slotOffset < nextScheduleWalker->slotOffset){
         counter+= (nextScheduleWalker->slotOffset - scheduleWalker->slotOffset 
                    - scheduleWalker->superSlotLength)/requestedSlotLength;
      }else{
          counter+= (SLOTFRAME_LENGTH- scheduleWalker->slotOffset 
                    - scheduleWalker->superSlotLength)/requestedSlotLength;
      }
      scheduleWalker = scheduleWalker->next;
      nextScheduleWalker = scheduleWalker->next;
   }while(scheduleWalker!=schedule_vars.currentScheduleEntry);

   ENABLE_INTERRUPTS();
   return counter;
}

uint8_t schedule_getNumberOfNegotiatedCells(open_addr_t* neighbor, cellRadioSetting_t cellRadioSetting, cellType_t cell_type){
    uint8_t i;
    uint8_t j;
    uint8_t counter;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    counter = 0;
    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].shared == FALSE       &&
            schedule_vars.scheduleBuf[i].type   == cell_type &&
            packetfunctions_sameAddress(&schedule_vars.scheduleBuf[i].neighbor, neighbor) == TRUE &&
            schedule_vars.scheduleBuf[i].cellRadioSetting == cellRadioSetting 
            //schedule_vars.scheduleBuf[i].isHardCell == FALSE  //Assuming that all the cells are negiciated but some are HARD and someothers are soft
        ){
            // for superSlots, the remaining sub-microSlots will be marked as off anyway
            counter++;
        } else {
            if (schedule_vars.scheduleBuf[i].isAutoCell){
                for(j=0;j<MAXBACKUPSLOTS;j++) {
                    if(
                        schedule_vars.scheduleBuf[i].backupEntries[j].type   == CELLTYPE_TX &&
                        packetfunctions_sameAddress(&(schedule_vars.scheduleBuf[i].backupEntries[j].neighbor), neighbor) == TRUE &&
                        schedule_vars.scheduleBuf[i].backupEntries[j].shared == FALSE //&&
                        //schedule_vars.scheduleBuf[i].backupEntries[j].cellRadioSetting == cellRadioSetting
                         // && schedule_vars.scheduleBuf[i].isHardCell == FALSE
                    ){  
                        counter++;
                        // at most one negotiated Tx cell to a neighbor in backup entries
                        break;
                    }
                }
            }
        }
    }

    ENABLE_INTERRUPTS();

    return counter;
}

bool schedule_isNumTxWrapped(open_addr_t* neighbor, cellRadioSetting_t cellRadioSetting){
    uint8_t i;
    bool    returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = FALSE;
    for(i=0;i<MAXACTIVESLOTS;i++){
        if(
            packetfunctions_sameAddress(&schedule_vars.scheduleBuf[i].neighbor, neighbor) == TRUE &&
            schedule_vars.scheduleBuf[i].cellRadioSetting == cellRadioSetting
        ){
            if (schedule_vars.scheduleBuf[i].numTx>0xFF/2){
                returnVal = TRUE;
            }
            ENABLE_INTERRUPTS();
            return returnVal;
        }
    }
    ENABLE_INTERRUPTS();
    return returnVal;

}

bool schedule_getCellsToBeRelocated(open_addr_t* neighbor, cellRadioSetting_t cellRadioSetting, cellInfo_ht* celllist){
    uint8_t     i;

    uint16_t    cellPDR;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    // found the cell with higest PDR
    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            packetfunctions_sameAddress(&schedule_vars.scheduleBuf[i].neighbor, neighbor) == TRUE &&
            schedule_vars.scheduleBuf[i].cellRadioSetting == cellRadioSetting &&
            schedule_vars.scheduleBuf[i].isHardCell == FALSE
        ){
            if (schedule_vars.scheduleBuf[i].numTx>MINIMAL_NUM_TX){
                cellPDR = 100*schedule_vars.scheduleBuf[i].numTxACK/schedule_vars.scheduleBuf[i].numTx;
                if (cellPDR < RELOCATE_PDRTHRES){
                    celllist->isUsed            = TRUE;
                    celllist->slotoffset        = schedule_vars.scheduleBuf[i].slotOffset;
                    celllist->channeloffset     = schedule_vars.scheduleBuf[i].channelOffset;
                    ENABLE_INTERRUPTS();
                    return TRUE;
                }
            }
        }
    }

    ENABLE_INTERRUPTS();

    return FALSE;
}

bool schedule_hasAutonomousTxRxCellUnicast(open_addr_t* neighbor){
    uint8_t i;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].type          == CELLTYPE_TXRX &&
            schedule_vars.scheduleBuf[i].shared                         &&
            schedule_vars.scheduleBuf[i].neighbor.type == ADDR_64B      &&
            packetfunctions_sameAddress(neighbor,&schedule_vars.scheduleBuf[i].neighbor)
        ){
            ENABLE_INTERRUPTS();
            return TRUE;
        }
    }

    ENABLE_INTERRUPTS();
    return FALSE;
}

bool schedule_getAutonomousTxRxCellUnicastNeighbor(open_addr_t* neighbor){
    uint8_t i;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].type          == CELLTYPE_TXRX &&
            schedule_vars.scheduleBuf[i].shared                         &&
            schedule_vars.scheduleBuf[i].neighbor.type == ADDR_64B      &&
            packetfunctions_sameAddress(neighbor,&schedule_vars.scheduleBuf[i].neighbor)
        ){
            memcpy(neighbor, &schedule_vars.scheduleBuf[i].neighbor, sizeof(open_addr_t));
            ENABLE_INTERRUPTS();
            return TRUE;
        }
    }

    ENABLE_INTERRUPTS();
    return FALSE;
}

bool schedule_hasAutoTxCellToNeighbor(open_addr_t* neighbor){
    uint8_t i;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].shared == TRUE &&
            schedule_vars.scheduleBuf[i].type   == CELLTYPE_TX &&
            schedule_vars.scheduleBuf[i].neighbor.type == ADDR_64B &&
            packetfunctions_sameAddress(neighbor,&schedule_vars.scheduleBuf[i].neighbor)
        ){
            ENABLE_INTERRUPTS();
            return TRUE;
        }
    }

    ENABLE_INTERRUPTS();
    return FALSE;
}

bool schedule_hasNegotiatedCellToNeighbor(open_addr_t* neighbor, cellType_t cell_type){
    uint8_t i;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].shared == FALSE &&
            schedule_vars.scheduleBuf[i].type   == cell_type &&
            schedule_vars.scheduleBuf[i].neighbor.type == ADDR_64B &&
            packetfunctions_sameAddress(neighbor,&schedule_vars.scheduleBuf[i].neighbor)
             
        ){
            ENABLE_INTERRUPTS();
            return TRUE;
        }
    }

    ENABLE_INTERRUPTS();
    return FALSE;
}

bool schedule_hasNegotiatedCellToNeighborKey(open_addr_t* neighbor, cellRadioSetting_t cellRadioSetting, cellType_t cell_type){
    uint8_t i;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].shared == FALSE &&
            schedule_vars.scheduleBuf[i].type   == cell_type &&
            schedule_vars.scheduleBuf[i].neighbor.type == ADDR_64B &&
            packetfunctions_sameAddress(neighbor,&schedule_vars.scheduleBuf[i].neighbor) &&
            schedule_vars.scheduleBuf[i].cellRadioSetting == cellRadioSetting
            
        ){
            ENABLE_INTERRUPTS();
            return TRUE;
        }
    }

    ENABLE_INTERRUPTS();
    return FALSE;
}
/**
\brief check whether there is negotiated tx cell to non-parent in schedule

\param parentNeighbor           The parent address.
\param parentNeighborRadio      The parent radio.
\param nonParentNeighbor        The neighbor address of the negotiated tx cell.
\param nonParentNeighborRadio   The neighbor radio of the negotiated tx cell.

*/

bool schedule_hasNegotiatedTxCellToNonParent(
    open_addr_t*        parentNeighbor,
    cellRadioSetting_t*   parentNeighborRadio,
    open_addr_t*        nonParentNeighbor,
    cellRadioSetting_t*   nonParentNeighborRadio
){
    uint8_t i;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].type          == CELLTYPE_TX   &&
            schedule_vars.scheduleBuf[i].shared        == FALSE         &&
            schedule_vars.scheduleBuf[i].neighbor.type == ADDR_64B      &&
            schedule_vars.scheduleBuf[i].isHardCell        == FALSE    &&  //We consider here that the cells negiciated cells added by a coap demand are HARD so we can't delete Negociated Hard cells
                                                                            //this is done to not delete the added cells to nonParent by MSF-housekeeping. We add through cinstrument cells according to RAW need and should not be deleted
            ( 
             packetfunctions_sameAddress(parentNeighbor,&schedule_vars.scheduleBuf[i].neighbor) == FALSE ||
             *parentNeighborRadio != schedule_vars.scheduleBuf[i].cellRadioSetting
            )
        ){
            memcpy(nonParentNeighbor,&schedule_vars.scheduleBuf[i].neighbor,sizeof(open_addr_t));
            *nonParentNeighborRadio = schedule_vars.scheduleBuf[i].cellRadioSetting;
            ENABLE_INTERRUPTS();
            return TRUE;
        }
    }

    ENABLE_INTERRUPTS();
    return FALSE;
}

bool schedule_getAutonomousTxRxCellAnycast(uint16_t* slotoffset){
    uint8_t i;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            schedule_vars.scheduleBuf[i].type          == CELLTYPE_TXRX &&
            schedule_vars.scheduleBuf[i].shared        == FALSE         &&
            schedule_vars.scheduleBuf[i].neighbor.type == ADDR_ANYCAST
        ){
            *slotoffset = schedule_vars.scheduleBuf[i].slotOffset;
            ENABLE_INTERRUPTS();
            return TRUE;
        }
    }

    ENABLE_INTERRUPTS();
    return FALSE;
}

//=== from IEEE802154E: reading the schedule and updating statistics

void schedule_syncSlotOffset(slotOffset_t targetSlotOffset) {

   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   while (schedule_vars.currentScheduleEntry->slotOffset!=targetSlotOffset) {
      schedule_advanceSlot();
   }

   ENABLE_INTERRUPTS();
}

/**
\brief advance to next active slot
*/
void schedule_advanceSlot(void) {

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    schedule_vars.currentScheduleEntry = schedule_vars.currentScheduleEntry->next;

    ENABLE_INTERRUPTS();
}

/**
\brief return slotOffset of next active slot
*/
slotOffset_t schedule_getNextActiveSlotOffset(void) {
    slotOffset_t res;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    res = ((scheduleEntry_t*)(schedule_vars.currentScheduleEntry->next))->slotOffset;

    ENABLE_INTERRUPTS();

    return res;
}

/**
\brief return current superSlot legnth
*/
uint8_t schedule_getCurrentSuperSlotLength(void) {
    uint8_t res;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    res = schedule_vars.currentScheduleEntry->superSlotLength;

    ENABLE_INTERRUPTS();

    return res;
}

/**
\brief Get the frame length.

\returns The frame length.
*/
frameLength_t schedule_getFrameLength(void) {
    frameLength_t returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.frameLength;

    ENABLE_INTERRUPTS();

    return returnVal;
}

/**

\brief Get the type of the current schedule entry.

\returns The type of the current schedule entry.
*/
cellType_t schedule_getType(void) {
    cellType_t returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.currentScheduleEntry->type;

    ENABLE_INTERRUPTS();

    return returnVal;
}

/**

\brief Get the isShared of the current schedule entry.

\returns The isShared of the current schedule entry.
*/
bool schedule_getShared(void) {
    bool returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.currentScheduleEntry->shared;

    ENABLE_INTERRUPTS();

    return returnVal;
}


/**

\brief Get the isAutoCell of the current schedule entry.

\returns The isAutoCell of the current schedule entry.
*/
bool schedule_getIsAutoCell(void) {
    bool returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.currentScheduleEntry->isAutoCell;

    ENABLE_INTERRUPTS();

    return returnVal;
}


/**

\brief Get the isHardCell of the current schedule entry.

\returns The isHardCell of the current schedule entry.
*/
bool schedule_getIsHardCell(void) {
    bool returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.currentScheduleEntry->isHardCell;

    ENABLE_INTERRUPTS();

    return returnVal;
}


/**
\brief Get the neighbor associated wit the current schedule entry.

\returns The neighbor associated wit the current schedule entry.
*/
void schedule_getNeighbor(open_addr_t* addrToWrite) {

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    memcpy(addrToWrite,&(schedule_vars.currentScheduleEntry->neighbor),sizeof(open_addr_t));

    ENABLE_INTERRUPTS();
}

/**
\brief Get the slot offset of the current schedule entry.

\returns The slot offset of the current schedule entry.
*/
slotOffset_t schedule_getSlottOffset(void) {
    channelOffset_t returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.currentScheduleEntry->slotOffset;

    ENABLE_INTERRUPTS();

    return returnVal;
}

/**

\brief Get the cellRadioSetting of the current schedule entry.

\returns The cellRadioSetting of the current schedule entry.
*/
cellRadioSetting_t schedule_getCellRadioSetting(void) {
    cellRadioSetting_t returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.currentScheduleEntry->cellRadioSetting;

    ENABLE_INTERRUPTS();

    return returnVal;
}

/**
\brief Get the channel offset of the current schedule entry.

\returns The channel offset of the current schedule entry.
*/
channelOffset_t schedule_getChannelOffset(void) {
    channelOffset_t returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    returnVal = schedule_vars.currentScheduleEntry->channelOffset;

    ENABLE_INTERRUPTS();

    return returnVal;
}

/**
\brief Check whether I can send on this slot.

This function is called at the beginning of every TX slot.
If the slot is *not* a shared slot, it always return TRUE.
If the slot is a shared slot, it decrements the backoff counter and returns
TRUE only if it hits 0.

Note that the backoff counter is global, not per slot.

\returns TRUE if it is OK to send on this slot, FALSE otherwise.
*/
bool schedule_getOkToSend(void) {
    bool returnVal;

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    if (schedule_vars.currentScheduleEntry->shared==FALSE) {
        // non-shared slot: backoff does not apply

        returnVal = TRUE;
    } else {
        // shared slot: check backoff before answering

        if (schedule_vars.currentScheduleEntry->neighbor.type == ADDR_ANYCAST){
            // this is a minimal cell
            if (schedule_vars.backoff>0) {
                schedule_vars.backoff--;
            }

            // only return TRUE if backoff hit 0
            if (schedule_vars.backoff==0) {
                returnVal = TRUE;
            } else {
                returnVal = FALSE;
            }
        } else {
            // this is a dedicated cell (auto Tx cell)
            neighbors_decreaseBackoff(&schedule_vars.currentScheduleEntry->neighbor);

            returnVal = neighbors_backoffHitZero(&schedule_vars.currentScheduleEntry->neighbor);
        }
    }

    ENABLE_INTERRUPTS();

    return returnVal;
}

/**
\brief Reset the backoff and backoffExponent.
*/
void schedule_resetBackoff(void) {

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    // reset backoffExponent
    schedule_vars.backoffExponenton   = MINBE-1;
    // reset backoff
    schedule_vars.backoff             = 0;

    ENABLE_INTERRUPTS();
}

/**
\brief Indicate the reception of a packet.
*/
void schedule_indicateRx(asn_t* asnTimestamp) {

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    // increment usage statistics
    schedule_vars.currentScheduleEntry->numRx++;

    // update last used timestamp
    memcpy(&(schedule_vars.currentScheduleEntry->lastUsedAsn), asnTimestamp, sizeof(asn_t));

    ENABLE_INTERRUPTS();
}

/**
\brief Indicate the transmission of a packet.
*/
void schedule_indicateTx(asn_t* asnTimestamp, bool succesfullTx) {

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    // increment usage statistics
    if (schedule_vars.currentScheduleEntry->numTx==0xFF) {
        schedule_vars.currentScheduleEntry->numTx/=2;
        schedule_vars.currentScheduleEntry->numTxACK/=2;
    }
    schedule_vars.currentScheduleEntry->numTx++;
    if (succesfullTx==TRUE) {
        schedule_vars.currentScheduleEntry->numTxACK++;
    }

    // update last used timestamp
    memcpy(&schedule_vars.currentScheduleEntry->lastUsedAsn, asnTimestamp, sizeof(asn_t));

    // update this backoff parameters for shared slots
    if (schedule_vars.currentScheduleEntry->shared==TRUE) {
        if (succesfullTx==TRUE) {
            if (schedule_vars.currentScheduleEntry->neighbor.type==ADDR_ANYCAST){
                // reset backoffExponent
                schedule_vars.backoffExponenton     = MINBE-1;
                // reset backoff
                schedule_vars.backoff               = 0;
            } else {
                neighbors_resetBackoff(&schedule_vars.currentScheduleEntry->neighbor);
            }
        } else {
            if (schedule_vars.currentScheduleEntry->neighbor.type==ADDR_ANYCAST){
                // increase the backoffExponent
                if (schedule_vars.backoffExponenton<MAXBE) {
                    schedule_vars.backoffExponenton++;
                }
                // set the backoff to a random value in [0..2^BE]
                schedule_vars.backoff       = openrandom_get16b()%(1<<schedule_vars.backoffExponenton);
            } else {
                neighbors_updateBackoff(&schedule_vars.currentScheduleEntry->neighbor);
            }
        }
    }

    ENABLE_INTERRUPTS();
}

bool schedule_getOneCellAfterOffset(uint8_t metadata,uint8_t offset,open_addr_t* neighbor, uint8_t cellOptions, uint16_t* slotoffset, uint16_t* channeloffset){
    bool returnVal;
    scheduleEntry_t* scheduleWalker;
    cellType_t type;
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    // translate cellOptions to cell type
    if ((cellOptions & SIXTOP_CELLREQUEST_LINKOPTIONS_MASK) == CELLOPTIONS_TX){
        type = CELLTYPE_TX;
    }
    if ((cellOptions & SIXTOP_CELLREQUEST_LINKOPTIONS_MASK) == CELLOPTIONS_RX){
        type = CELLTYPE_RX;
    }
    if ((cellOptions & SIXTOP_CELLREQUEST_LINKOPTIONS_MASK) == (CELLOPTIONS_TX | CELLOPTIONS_RX | CELLOPTIONS_SHARED)){
        type = CELLTYPE_TXRX;
    }

    returnVal      = FALSE;
    scheduleWalker = &schedule_vars.scheduleBuf[0]; // fisrt entry record slotoffset 0
    do {
        if(type == scheduleWalker->type && scheduleWalker->slotOffset >= offset){
            *slotoffset    = scheduleWalker->slotOffset;
            *channeloffset = scheduleWalker->channelOffset;
            returnVal      = TRUE;
            break;
        }
        scheduleWalker = scheduleWalker->next;
   }while(scheduleWalker!=&schedule_vars.scheduleBuf[0]);

    ENABLE_INTERRUPTS();

   return returnVal;
}

//=========================== private =========================================

/**
\pre This function assumes interrupts are already disabled.
*/
void schedule_resetEntry(scheduleEntry_t* e) {
    e->slotOffset             = 0;
    e->type                   = CELLTYPE_OFF;
    e->shared                 = FALSE;
    e->isAutoCell             = FALSE;
    e->isHardCell             = FALSE;
    e->channelOffset          = 0;
    e->cellRadioSetting       = 0;
    e->superSlotLength        = 0;

    e->neighbor.type          = ADDR_NONE;
    memset(&e->neighbor.addr_64b[0], 0x00, sizeof(e->neighbor.addr_64b));

    e->numRx                  = 0;
    e->numTx                  = 0;
    e->numTxACK               = 0;
    e->lastUsedAsn.bytes0and1 = 0;
    e->lastUsedAsn.bytes2and3 = 0;
    e->lastUsedAsn.byte4      = 0;
    e->next                   = NULL;
}

void schedule_resetBackupEntry(backupEntry_t* e) {
    e->type                   = CELLTYPE_OFF;
    e->shared                 = FALSE;
    e->isAutoCell             = FALSE;
    e->isHardCell             = FALSE;
    e->channelOffset          = 0;
    
    e->neighbor.type          = ADDR_NONE;
    memset(&e->neighbor.addr_64b[0], 0x00, sizeof(e->neighbor.addr_64b));

    e->numRx                  = 0;
    e->numTx                  = 0;
    e->numTxACK               = 0;
    e->lastUsedAsn.bytes0and1 = 0;
    e->lastUsedAsn.bytes2and3 = 0;
    e->lastUsedAsn.byte4      = 0;
    e->next                   = NULL;
}


/*Added by mm to remove HardCells created for track*/
//this can be used safely once it is called on sending a request to each node to delete all cells it hold,
//however, we can't delete the added cells on root side because we cant send a coap req to it.
//instead we need to use a sixtoprequest with CMD_DELETE, so we can let each pair of nodes negociate the cells to delete.

/*
void schedule_removeAllHardCellsToNeighbor(
    uint8_t        slotframeID,
    open_addr_t*   neighbor,
    cellRadioSetting_t neighborRadio
){
    uint8_t i;

    // remove all entries in schedule with previousHop address
    for(i=0;i<MAXACTIVESLOTS;i++){
        if (
            packetfunctions_sameAddress(&(schedule_vars.scheduleBuf[i].neighbor),neighbor) &&
              schedule_vars.scheduleBuf[i].cellRadioSetting == neighborRadio               &&
            (
                schedule_vars.scheduleBuf[i].type == CELLTYPE_TX ||
                schedule_vars.scheduleBuf[i].type == CELLTYPE_RX ||
                schedule_vars.scheduleBuf[i].isHardCell == TRUE
            )
        ){
            schedule_removeActiveSlot(
                schedule_vars.scheduleBuf[i].slotOffset,
                schedule_vars.scheduleBuf[i].type,
                schedule_vars.scheduleBuf[i].shared,
                neighbor
            );
        }
    }
}
*/
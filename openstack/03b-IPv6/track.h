#ifndef __TRACK_H
#define __TRACK_H
#include "schedule.h"
/**
\addtogroup IPv6
\{
\addtogroup TRACK
\{
*/
#define MAX_NUM_TRACKS 1 //Let's start with a max number of tracks set to 1 
                     

#define MAX_NUM_SUBTRACKS 2 //Let's start with a max number of sub_tracks set to 2 
                        // A complex track of two paths to allow packet replication
//=========================== define ==========================================

//=========================== typedef =========================================


typedef struct {
   uint8_t              subtrack_id;
   bool                 is_ingress;
   bool                 is_egress;
   uint8_t              bundle_length;
   open_addr_t          source_addr;
   slotinfo_element_t   cell_info;
   //open_addr_t          track_parent_addr;
   //cellRadioSetting_t   track_parent_cellRadioSetting;
} subtrack_t;

typedef struct {
   uint8_t    track_id; //Track ID
   uint8_t    num_subtracks; //number of sub-tracks (paths) forming the track
   subtrack_t    subtrack_list[MAX_NUM_SUBTRACKS+1]; //we add +1 to avoid falling on first value '0' it jumps test on creating track existence ID, start from 1
} track_t;


typedef struct {
   track_t    track_list[MAX_NUM_TRACKS+1]; //list of current Tracks
   uint8_t    nb_tracks; //number of current Tracks
   bool       busyInstallingTrack;
} track_vars_t;



//=========================== variables =======================================

//=========================== prototypes ======================================

void     track_init(void);
owerror_t  track_installOrUpdateTrack(OpenQueueEntry_t* msg);
owerror_t  track_deleteTrack(OpenQueueEntry_t* msg);
open_addr_t* track_getIngressAddr64(uint8_t trackID);
bool track_getTrackExistenceByID(uint8_t trackID);
cellRadioSetting_t track_getParentRadio(uint8_t trackID);
open_addr_t* track_getParentAddr64(uint8_t trackID);
bool track_getIsEgress(uint8_t trackID);
bool track_getIsIngress(uint8_t trackID);
uint8_t track_getNbSubTracks(uint8_t trackID);
uint8_t track_getNbTracks();
track_vars_t track_getTracks();

/**
\}
\}
*/

#endif

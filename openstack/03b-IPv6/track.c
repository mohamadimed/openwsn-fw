#include "opendefs.h"
#include "track.h"
#include "openrandom.h"
#include "board.h"
#include "idmanager.h"
#include "IEEE802154E.h"
#include "neighbors.h"
#include "msf.h"
#include "sixtop.h"


//=========================== variables =======================================

track_vars_t track_vars;

//=========================== prototypes ======================================

bool track_getTrackExistenceByID(uint8_t trackID);
bool track_getSubTrackExistenceByID(uint8_t TrackID, uint8_t subTrackID);
bool track_getIfIamIngress(uint8_t byte0, uint8_t byte1);
bool track_reserveTrackCells(open_addr_t neighbor, uint8_t neighborRadio,uint8_t trackID, uint8_t subTrackID, uint8_t bundle);
bool track_realocateTrackCells(open_addr_t neighbor, uint8_t neighborRadio,uint8_t trackID, uint8_t subTrackID, uint8_t bundle);
bool track_deleteTrackCells(uint8_t trackID, uint8_t subTrackID);
void track_setParentEui64from16(uint8_t trackID, uint8_t subTrackID, uint8_t byte0, uint8_t byte1);
open_addr_t track_getParentEui64from16(uint8_t trackID, uint8_t subTrackID, uint8_t byte0, uint8_t byte1);


//=========================== public ==========================================

/**
\brief Initialize this module.
*/
void track_init(void) {
   
   uint8_t i,j;
   
   // clear local variables
    memset(&track_vars,0,sizeof(track_vars_t));
   
   //init Track variable
   track_vars.nb_tracks = 0; 
   track_vars.busyInstallingTrack = 0;
   
   for(i=0;i<=MAX_NUM_TRACKS+1;i++){ //to not match with the first track having ID 0, so we dont skip first test of existing track/sub_track (MAX +1) to not use [0]

   track_vars.track_list[i].track_id = 0;
      
     for (j=0;j<=MAX_NUM_SUBTRACKS+1;j++)
      {
   track_vars.track_list[i].num_subtracks = 0;
   track_vars.track_list[i].subtrack_list[j].subtrack_id = 0;
   track_vars.track_list[i].subtrack_list[j].bundle_length = 0;
   track_vars.track_list[i].subtrack_list[j].is_egress = FALSE;
   track_vars.track_list[i].subtrack_list[j].is_ingress = FALSE;
   //current_track.subtrack_list[j].track_parent_addr = open_addr_t (NULL);
   //current_track.subtrack_list[j].track_parent_cellRadioSetting = NULL;
      }

                                    }
}


owerror_t  track_installOrUpdateTrack(OpenQueueEntry_t* msg){

      
   if (ieee154e_isSynch() == FALSE) {
         //abort if I am not associated to the network yet
        return 0;
    }

    if (track_vars.busyInstallingTrack==TRUE) {
        // don't continue if I'm still installing a track
        return 0;
    }
      

   
      //Feed track vars from the received packet
      
      uint8_t trackID, subTrackID;
      uint8_t bundle_length, parent_radio;
      uint8_t parent_addr_byte0, parent_addr_byte1;
      uint8_t ingress_addr_byte0, ingress_addr_byte1;
      open_addr_t parent;
      
      /*read packet payload*/

      trackID               = msg->payload[0];
      subTrackID            = msg->payload[1];
      ingress_addr_byte0    = msg->payload[2];
      ingress_addr_byte1    = msg->payload[3];
      bundle_length         = msg->payload[6];
      parent_addr_byte0     = msg->payload[7];
      parent_addr_byte1     = msg->payload[8];
      parent_radio          = msg->payload[9];


      //fill parent address with first 48 bytes
       parent=track_getParentEui64from16(trackID,subTrackID,parent_addr_byte0,parent_addr_byte1);



      //Get track ID to check if we already created this Track, it may be an update or a adding another subTruck

      if(!track_getTrackExistenceByID(trackID)){ //this is the first track we gonna add it and the subtrack also

         if((track_vars.nb_tracks >= MAX_NUM_TRACKS) || (trackID > MAX_NUM_TRACKS) || (subTrackID > MAX_NUM_SUBTRACKS)) // don't allow track ID > to max tracks to not overflow table and bug if luanched directly with track for eg3
            return 0;

         //we are not out of rang of MAX_NUM TRACKS
               if(track_reserveTrackCells(parent,parent_radio,trackID,subTrackID,bundle_length)) 
               
               {
                  //Track defined by trackId,subTrackID exists so proceed to suppression
               track_vars.nb_tracks++;
               track_vars.track_list[trackID].track_id = trackID;


               //get SubTrack info
               track_vars.track_list[trackID].num_subtracks++;
               track_vars.track_list[trackID].subtrack_list[subTrackID].subtrack_id = subTrackID;
               track_vars.track_list[trackID].subtrack_list[subTrackID].bundle_length = bundle_length;
               //Check and get if we are Ingress point
               if (idmanager_getIsDAGroot()) 
               track_vars.track_list[trackID].subtrack_list[subTrackID].is_egress = TRUE;
               //Check and get if we are Egress point 
               if(track_getIfIamIngress(ingress_addr_byte0,ingress_addr_byte1))
               track_vars.track_list[trackID].subtrack_list[subTrackID].is_ingress = TRUE;
                        
               track_setParentEui64from16(trackID,subTrackID,parent_addr_byte0,parent_addr_byte1);
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_cellRadioSetting = (cellRadioSetting_t) parent_radio;

               return 1; 
               
               }
               else {return 0;}
         }

       else { 
               //update Track if it already exists [NEED TO REALLOCATE HARD CELLS]   /*TBD*/
               
               if(!track_getSubTrackExistenceByID(trackID,subTrackID)){ // Here the track ID exists , but not the sub track, so add the new subtrack (its an ingress point may have several subtracks)
                  if(track_vars.track_list[trackID].num_subtracks >= MAX_NUM_SUBTRACKS || subTrackID > MAX_NUM_SUBTRACKS)
                     return 0;
               
                   //we are not out of rang of MAX_NUM TRACKS
                           
                        if(track_reserveTrackCells(parent,parent_radio,trackID,subTrackID,bundle_length)) 
                        {                      
                        //get SubTrack info
                        track_vars.track_list[trackID].num_subtracks++;
                        track_vars.track_list[trackID].subtrack_list[subTrackID].subtrack_id = subTrackID;
                        track_vars.track_list[trackID].subtrack_list[subTrackID].bundle_length = bundle_length;
                        //Check and get if we are Ingress point
                        if (idmanager_getIsDAGroot()) 
                        track_vars.track_list[trackID].subtrack_list[subTrackID].is_egress = TRUE;
                        //Check and get if we are Egress point 
                        if(track_getIfIamIngress(ingress_addr_byte0,ingress_addr_byte1))
                        track_vars.track_list[trackID].subtrack_list[subTrackID].is_ingress = TRUE;
                        
                        track_setParentEui64from16(trackID,subTrackID,parent_addr_byte0,parent_addr_byte1);
                        track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_cellRadioSetting = (cellRadioSetting_t) parent_radio;
                      
                        return 1;
                        }
                        else return 0;
               } else {
                        //update sub track if it already exists       //here we should have the track ID and sub track ID----> an update is needed
                                           
                         if(!track_realocateTrackCells(parent,parent_radio,trackID,subTrackID,bundle_length)) return 0;
                                 else {
                                 //get SubTrack info
                                 track_vars.track_list[trackID].subtrack_list[subTrackID].subtrack_id = subTrackID;
                                 track_vars.track_list[trackID].subtrack_list[subTrackID].bundle_length = bundle_length;
                                 //Check and get if we are Ingress point
                                 if (idmanager_getIsDAGroot()) 
                                 track_vars.track_list[trackID].subtrack_list[subTrackID].is_egress = TRUE;
                                 //Check and get if we are Egress point 
                                 if(track_getIfIamIngress(ingress_addr_byte0,ingress_addr_byte1))//Define condition for Egress later/*TBD*/
                                 track_vars.track_list[trackID].subtrack_list[subTrackID].is_ingress = TRUE;
                                 
                                 track_setParentEui64from16(trackID,subTrackID,parent_addr_byte0,parent_addr_byte1);
                                 track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_cellRadioSetting = (cellRadioSetting_t) parent_radio; 
                                 return 1;   }

                     
                          
                  }

       }

}
owerror_t  track_deleteTrack(OpenQueueEntry_t* msg)
{
if (ieee154e_isSynch() == FALSE) {
         //abort if I am not associated to the network yet
        return 0;
    }

    if (track_vars.busyInstallingTrack==TRUE) {
        // don't continue if I'm still installing a track
        return 0;
    }
      

   
      //Feed track vars from the received packet
      
      uint8_t trackID, subTrackID;

      trackID = msg->payload[0];
      subTrackID = msg->payload[1];

      //Check if we the Track/subTrack to be deleted has been already created
      if(!track_getTrackExistenceByID(trackID) || !track_getSubTrackExistenceByID(trackID,subTrackID))  //here it returns because if delete same track twice it will be zero and values will be zero so it will not find it, we should test if trak cnd sucb track in te same time to quit


         //Check if the subTrack exists (because it also define the Track)
         {

            //The subTrack doesn't exist return
            return 0;
         }

         else {

               //we already know the parent since we have the ID of track and subtrack, it will get it inside deleteTrack function

               //proceed to cell reservation
               if (track_deleteTrackCells(trackID,subTrackID)){
               

               //Track defined by trackId,subTrackID exists so proceed to suppression

               //get SubTrack info
               track_vars.track_list[trackID].num_subtracks--;
                             
               track_vars.track_list[trackID].subtrack_list[subTrackID].subtrack_id = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].bundle_length = 0;
               //Check and get if we are Ingress point
               if (idmanager_getIsDAGroot()) 
               track_vars.track_list[trackID].subtrack_list[subTrackID].is_egress = FALSE;
               //Check and get if we are Egress point 
               if(track_getIfIamIngress(msg->payload[2],msg->payload[3]))
               track_vars.track_list[trackID].subtrack_list[subTrackID].is_ingress = FALSE;
                        
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[0] = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[1] = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[2] = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[3] = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[4] = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[5] = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[6] = 0;
               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[7] = 0;

               track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_cellRadioSetting = (cellRadioSetting_t) 0;
               //let this line to final be cause if we set trickID to 0 we can't reach the above vars to reset tem
               if( track_vars.track_list[trackID].num_subtracks == 0) //here to say if we are ingress, don't set track_ID to 0 until we are in last lap od subtrack suppresion
                                                                      {track_vars.nb_tracks--;
                                                                      track_vars.track_list[trackID].track_id = 0;}
               
               return 1; 
               }
               else return 0;
         }

      }





//=========================== private =========================================

bool track_getTrackExistenceByID(uint8_t trackID)

{ bool track_exists;
  uint8_t i;
  
  track_exists = FALSE;
  
   for (i = 0; i <=MAX_NUM_TRACKS+1; i++)
   {
      if (track_vars.track_list[i].track_id == trackID)

      {track_exists = TRUE;
      break;}
   }
  return track_exists;
}


bool track_getSubTrackExistenceByID(uint8_t TrackID, uint8_t subTrackID)

{ bool subTrack_exists;
  uint8_t i;
  
  subTrack_exists = FALSE;

   for (i = 0; i <=MAX_NUM_SUBTRACKS+1; i++)
   {
      if (track_vars.track_list[TrackID].subtrack_list[i].subtrack_id == subTrackID)

      {subTrack_exists = TRUE;
      break;}
   }
  return subTrack_exists;
}

bool track_getIfIamIngress(uint8_t byte0, uint8_t byte1)

{ bool is_ingress;
  is_ingress = FALSE;

  //if (memcmp((void *) address_1->addr_128b, (void *) address_2->addr_128b, address_length) == 0)
  if(idmanager_getMyID(ADDR_64B)->addr_64b[6]== byte0 && idmanager_getMyID(ADDR_64B)->addr_64b[7]== byte1)
         is_ingress = TRUE;


  return is_ingress;
}

bool track_reserveTrackCells(open_addr_t neighbor, uint8_t neighborRadio,uint8_t trackID, uint8_t subTrackID, uint8_t bundle)

{  bool is_Reserved = FALSE;
   cellInfo_ht           celllist_add[CELLLIST_MAX_LEN];
   uint8_t               cellOptions;
   owerror_t             outcome;

      /*TBD LOOP using bundle*/

      if (msf_candidateAddCellList(celllist_add,bundle,neighborRadio)==FALSE)
            is_Reserved = FALSE;
                     
      else  {
            cellOptions = CELLOPTIONS_TX;
            cellOptions |= neighborRadio <<5;//0 <<5;//CELLRADIOSETTING_3 <<5;//neighborRadio <<5;

            
               
               // call sixtop
            outcome = sixtop_request(
            IANA_6TOP_CMD_ADD,                  // code
            &neighbor,                          // neighbor
            bundle,//numCells                          // number cells
            cellOptions,                     // cellOptions
            celllist_add,                       // celllist to add
            NULL,                               // celllist to delete (not used)
            msf_getsfid(),                      // sfid
            0,                                  // list command offset (not used)
            0,                                   // list command maximum celllist (not used)
            TRUE                                 // isHardCell? Yes, added manually to be used later so dont want to be deleted, consider it as HardCell
                                       );
            if(outcome == E_SUCCESS)
            is_Reserved = TRUE;
            }

   return is_Reserved;
}

bool track_deleteTrackCells(uint8_t trackID, uint8_t subTrackID)

{  bool is_Deleted = FALSE;
   cellInfo_ht           celllist_delete[CELLLIST_MAX_LEN];
   open_addr_t           neighbor;
   cellRadioSetting_t    neighborRadio;
   uint8_t               cellOptions;
   uint8_t               bundle;
   owerror_t             outcome;

   neighbor      = track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr;
   neighborRadio = track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_cellRadioSetting;
   bundle      = track_vars.track_list[trackID].subtrack_list[subTrackID].bundle_length;

   /*TBD bundle loop*/

      if (msf_candidateRemoveCellList(celllist_delete,&neighbor,neighborRadio,bundle, CELLTYPE_TX)==FALSE)
            is_Deleted = FALSE;
                     
      else  {
            cellOptions = CELLOPTIONS_TX;
            
               // call sixtop
            outcome = sixtop_request(
            IANA_6TOP_CMD_DELETE,                  // code
            &neighbor,                          // neighbor
            bundle, //numCells,                                  // number cells
            cellOptions,                     // cellOptions
            NULL,                       // celllist to add
            celllist_delete,                               // celllist to delete (not used)
            msf_getsfid(),                      // sfid
            0,                                  // list command offset (not used)
            0,                                   // list command maximum celllist (not used)
            TRUE                                 // isHardCell? Yes, added manually to be used later so dont want to be deleted, consider it as HardCell
                                       );
            if(outcome == E_SUCCESS)
            is_Deleted = TRUE;
            }

   return is_Deleted;
}



//#we must assume that the bundle should same when performing an update to ensure correct function, otherwise we must delete the track vars and create it all new
bool track_realocateTrackCells(open_addr_t neighbor, uint8_t neighborRadio,uint8_t trackID, uint8_t subTrackID, uint8_t bundle)

{  bool is_Reserved = FALSE;
   cellInfo_ht           celllist_add[CELLLIST_MAX_LEN];
   cellInfo_ht           celllist_delete[CELLLIST_MAX_LEN];
   uint8_t               cellOptions;
   owerror_t             outcome;

      memset(celllist_delete, 0, CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
      memset(celllist_add, 0, CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
      
      if (msf_candidateRemoveCellList(celllist_delete,&neighbor,neighborRadio,bundle, CELLTYPE_TX) == FALSE)
        
            return is_Reserved;
            
      if (msf_candidateAddCellList(celllist_add, bundle, neighborRadio) == FALSE){
            // failed to get cell list to add
           return is_Reserved;
        }

            
                     
      else  {
            cellOptions = CELLOPTIONS_TX;
            cellOptions |= neighborRadio <<5;//0 <<5;//CELLRADIOSETTING_3 <<5;//neighborRadio <<5;

            
               
               // call sixtop
            outcome = sixtop_request(
            IANA_6TOP_CMD_RELOCATE,                  // code
            &neighbor,                          // neighbor
            bundle,//numCells                          // number cells
            cellOptions,                     // cellOptions
            celllist_add,                       // celllist to add
            celllist_delete,                               // celllist to delete (not used)
            msf_getsfid(),                      // sfid
            0,                                  // list command offset (not used)
            0,                                   // list command maximum celllist (not used)
            TRUE                                 // isHardCell? Yes, added manually to be used later so dont want to be deleted, consider it as HardCell
                                       );
            if(outcome == E_SUCCESS)
            is_Reserved = TRUE;
            }

   return is_Reserved;
}


/*We use this function to generate the EUI-64 address of a given node based on its EUI-16, this is for shorten the sent packets size since now we lean on CoAP to install tracks*/
void track_setParentEui64from16(uint8_t trackID, uint8_t subTrackID,uint8_t byte0, uint8_t byte1)
{  track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.type = ADDR_64B;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[6] = byte0;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[7] = byte1;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[0] = 0x00;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[1] = 0x12;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[2] = 0x4b;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[3] = 0x00;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[4] = 0x14;
   track_vars.track_list[trackID].subtrack_list[subTrackID].track_parent_addr.addr_64b[5] = 0xb5;


}

open_addr_t track_getParentEui64from16(uint8_t trackID, uint8_t subTrackID, uint8_t byte0, uint8_t byte1)

{  open_addr_t neighbor;
   neighbor.type = ADDR_64B;
   neighbor.addr_64b[6] = byte0;
   neighbor.addr_64b[7] = byte1;
   neighbor.addr_64b[0] = 0x00;
   neighbor.addr_64b[1] = 0x12;
   neighbor.addr_64b[2] = 0x4b;
   neighbor.addr_64b[3] = 0x00;
   neighbor.addr_64b[4] = 0x14;
   neighbor.addr_64b[5] = 0xb5;

   return neighbor;

}


/*
    Payload format for Track creation. This packet should be created by 
     the PCE and sent to every concerned node in the mesh network


  1 Byte      1 Byte       2 Bytes    2 Bytes    1Byte    2 Bytes        1 Byte      1 Byte
+---------+------------+-----------+-----------+--------+-----------+------------+-----------+
|Track ID |Sub_Track ID|Ingress adr|Egress adr |Bundle n|Parent adr |Parent radio|add 1/del 0|
+---------+------------+-----------+-----------+--------+-----------+------------+-----------+

*/


OpenWSN firmware: stuff that runs on a mote

Part of UC Berkeley's OpenWSN project, http://www.openwsn.org/.

This  branch contains the most updated g6TiSCH/6DYN implementations. The firmware allows to cutomize your g6TiSCH/6DYN network. The input parmeters are in `/inc/opendefs.h` file.  The folloinw diagram below illustrates the configuration.

![g6tisch documentation](https://github.com/minarady1/openwsn-fw/blob/develop_FW-891/docs/g6tisch_documentation.png?raw=true)



``` 
//--------------------------- g6TiSCH definitions ------------------------------

// radio settings available for the MAC layer and supported by openmote-b
typedef enum 
{
    
    // different "modulations" (AT86RF215-specific)
    // At83rf215 settings start at index 0 because they are used directly in a sub-array in the atmel driver.
    RADIOSETTING_FSK_OPTION1_FEC,
    RADIOSETTING_OQPSK_RATE3,
    RADIOSETTING_OFDM_OPTION_1_MCS0,
    RADIOSETTING_OFDM_OPTION_1_MCS1,
    RADIOSETTING_OFDM_OPTION_1_MCS2,
    RADIOSETTING_OFDM_OPTION_1_MCS3,

    // default
    RADIOSETTING_24GHZ,
 
    // can be useful for switching receiver between OFDMx MCS modes.
    RADIOSETTING_NONE,
    
    RADIOSETTING_MAX 

} radioSetting_t;


// radio settings indexes for cell options.
// these settings will be mapped to any subset of the available settings 
// in the Open Radio interface

typedef enum{
    CELLRADIOSETTING_1,
    CELLRADIOSETTING_2 ,
    CELLRADIOSETTING_3 ,
    MAX_CELLRADIOSETTINGS 
} cellRadioSetting_t;

// mapping of MAC-level cellRadioSetting_t (in schedule.h) to Open Radio 
// radioSetting_t (in opendefs.h)
static const radioSetting_t cellRadioSettingMap[MAX_CELLRADIOSETTINGS]= {
  RADIOSETTING_24GHZ,
  RADIOSETTING_FSK_OPTION1_FEC,
  RADIOSETTING_OFDM_OPTION_1_MCS3, //using 3 atmel subGHz modulations to test with uGateway
};

// the radiosetting used for fallback (autonoumous cells)
#define CELLRADIOSETTING_FALLBACK       CELLRADIOSETTING_2

// the radiosetting used for initial syncronization
#define CELLRADIOSETTING_INIT           CELLRADIOSETTING_2


// max minutes for node to wait before joining (cjoin app)
// this is to simulate slow bootloading
#define SLOW_JOIN_INTERVAL             1 // mins, default 1 

// the standard block
#define MICROSLOTDURATION             328*1 // in tics = 10 ms

static const uint8_t superSlotLengthMap[MAX_CELLRADIOSETTINGS]= {
  2, // should be 2 for 24GHz radio
  4,//4, // should be 4 for FSK 
  1,//1, // should be 1 for OFDM 1 MCS3
};
// mapping of MAC-evel cellRadioSetting_t (in schedule.h) to DAGRank increases 
// ordered by range from shortest to longest
static const uint8_t radioMinHopRankIncreaseFactor [MAX_CELLRADIOSETTINGS]= {
    1,        //24GHZ  2
    5,       //FSK  5
    2        //OFDM 1
};

//poipoi: change this to be max (radioMinHopRankIncreaseFactor)
#define MAX_RANK_WEIGHT       5

static const uint8_t radioDefaultLinkCostMap [MAX_CELLRADIOSETTINGS]= {
    4,       //24GHZ  4
    4,       //FSK  1
    4       //OFDM 2
};

//--------------------------- end g6TiSCH definitions --------------------------
```

Build status
------------

|              builder                                                                                                                 |      build                   | outcome
| ------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------- | ------- 
| [Travis](https://travis-ci.org/openwsn-berkeley/openwsn-fw)                                                                          | compile                      | [![Build Status](https://travis-ci.org/openwsn-berkeley/openwsn-fw.png?branch=develop)](https://travis-ci.org/openwsn-berkeley/openwsn-fw)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=telosb,label=master,project=oos_openwsn,toolchain=mspgcc/)           | compile (TelosB)             | [![Build Status](http://builder.openwsn.org/job/Firmware/board=telosb,label=master,project=oos_openwsn,toolchain=mspgcc/badge/icon/)](http://builder.openwsn.org/job/Firmware/board=telosb,label=master,project=oos_openwsn,toolchain=mspgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=gina,label=master,project=oos_openwsn,toolchain=mspgcc/)             | compile (GINA)               | [![Build Status](http://builder.openwsn.org/job/Firmware/board=gina,label=master,project=oos_openwsn,toolchain=mspgcc/badge/icon/)](http://builder.openwsn.org/job/Firmware/board=gina,label=master,project=oos_macpong,toolchain=mspgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=wsn430v13b,label=master,project=oos_openwsn,toolchain=mspgcc/)       | compile (wsn430v13b)         | [![Build Status](http://builder.openwsn.org/job/Firmware/board=wsn430v13b,label=master,project=oos_openwsn,toolchain=mspgcc/badge/icon/)](http://builder.openwsn.org/job/Firmware/board=wsn430v13b,label=master,project=oos_macpong,toolchain=mspgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=wsn430v14,label=master,project=oos_openwsn,toolchain=mspgcc/)        | compile (wsn430v14)          | [![Build Status](http://builder.openwsn.org/job/Firmware/board=wsn430v14,label=master,project=oos_openwsn,toolchain=mspgcc/badge/icon/)](http://builder.openwsn.org/job/Firmware/board=wsn430v14,label=master,project=oos_macpong,toolchain=mspgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=Z1,label=master,project=oos_openwsn,toolchain=mspgcc/)               | compile (Z1)                 | [![Build Status](http://builder.openwsn.org/job/Firmware/board=z1,label=master,project=oos_openwsn,toolchain=mspgcc/badge/icon/)](http://builder.openwsn.org/job/Firmware/board=z1,label=master,project=oos_macpong,toolchain=mspgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=openmote-cc2538,label=master,project=oos_openwsn,toolchain=armgcc/)  | compile (OpenMote-CC2538)    | [![Build Status](http://builder.openwsn.org/job/Firmware/board=openmote-cc2538,label=master,project=oos_openwsn,toolchain=armgcc/badge/icon)](http://builder.openwsn.org/job/Firmware/board=openmote-cc2538,label=master,project=oos_openwsn,toolchain=armgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=OpenMoteSTM,label=master,project=oos_openwsn,toolchain=armgcc/)      | compile (OpenMoteSTM)        | [![Build Status](http://builder.openwsn.org/job/Firmware/board=openmotestm,label=master,project=oos_openwsn,toolchain=armgcc/badge/icon)](http://builder.openwsn.org/job/Firmware/board=openmotestm,label=master,project=oos_openwsn,toolchain=armgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=IoT-LAB_M3,label=master,project=oos_openwsn,toolchain=armgcc/)       | compile (IoT-LAB_M3)         | [![Build Status](http://builder.openwsn.org/job/Firmware/board=iot-lab_M3,label=master,project=oos_openwsn,toolchain=armgcc/badge/icon)](http://builder.openwsn.org/job/Firmware/board=iot-lab_M3,label=master,project=oos_openwsn,toolchain=armgcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Firmware/board=Python,label=master,project=oos_openwsn,toolchain=gcc/)              | compile (Python, simulation) | [![Build Status](http://builder.openwsn.org/job/Firmware/board=python,label=master,project=oos_openwsn,toolchain=gcc/badge/icon)](http://builder.openwsn.org/job/Firmware/board=python,label=master,project=oos_openwsn,toolchain=gcc/)
| [OpenWSN builder](http://builder.openwsn.org/job/Docs/)                                                                              | publish documentation        | [![Build Status](http://builder.openwsn.org/job/Docs/badge/icon)](http://builder.openwsn.org/job/Docs/)

Documentation
-------------

- overview: https://openwsn.atlassian.net/wiki/
- source code: http://openwsn-berkeley.github.io/firmware/

#ifndef EVGREGMAP_H
#define EVGREGMAP_H

#include "epicsTypes.h"

/**************************************************************************************************/
/*    Series 2xx Event Generator Modular Register Map                                             */
/*                                                                                                */
/*    Note: The "Uxx_" tag at the beginning of each of definitions below should not be included   */
/*          when the defined offset is passed to one of the I/O access macros. The macros will    */
/*          append the appropriate suffix to the offset name based on the number of bits to be    */
/*          read or written.    The purpose of this method is to produce a compiler error if you  */
/*          attempt to use a macro that does not match the size of the register.                  */
/**************************************************************************************************/

//=====================
// Status Registers
// Main status register is defined in mrmShared.h (U32_Status)
#define  U8_DBusRxValue         0x0000  // Distributed Data Bus Received Values
#define  U8_DBusTxValue         0x0001  // Distributed Data Bus Transmitted Values

//=====================
// General Control Register
// is defined in mrmShared.h (U32_Control)

//=====================
// Interrupt Control Registers
//
#define  U32_IrqFlag            0x0008  // Interrupt Flag Register
#define  U32_IrqEnable          0x000C  // Interrupt Enable Register

//=====================
// AC Trigger Control Registers
//
#define  U32_AcTrigControl      0x0010
#define  U8_AcTrigControl       0x0011  // AC Trigger Input Control Register
#define  U8_AcTrigDivider       0x0012  // AC Trigger Input Divider
#define  U8_AcTrigPhase         0x0013  // AC Trigger Input Phase Delay
#define  U8_AcTrigEvtMap        0x0017  // AC Trigger Input To Trigger Event Mapping

//=====================
// Data Buffer and Distributed Data Bus Control
//
#define  U32_DataTxCtrlEvg      0x0020  // Data Buffer Control Register
#define  U32_DataTxCtrlEvg_seg  0x001C  // Data Buffer Control Register for firmware version 207+ (segmented data buffer)
#define  U32_DBusSrc            0x0024  // Distributed Data Bus Mapping Register

//=====================
// FPGA Firmware Version
//
// defined in mrmShared.h

//=====================
// Input Signal State Registers
//
#define  U32_FPInput            0x0040  // Front Panel Input state register
#define  U32_UnivInput          0x0044  // Universal Input state register
#define  U32_TBInput            0x0048  // Transition Board Input state register

//=====================
// Event Clock Control
//
#define  U16_uSecDiv            0x004e  // Event Clock Freq Rounded to Nearest 1 MHz
#define  U8_ClockSource         0x0050  // Event Clock Control Register (also Source select (Internal, RF Input,...))
#define  U8_RfDiv               0x0051  // DBus phase toggle and RF Input Divider
#define  U16_ClockStatus        0x0052  // Event Clock Status

//=====================
// Event Analyzer Registers
//
#define  U32_EvAnControl        0x0060  // Event Analyser Control/Status Register
#define  U16_EvAnEvent          0x0066  // Event Code & Data Buffer Byte
#define  U32_EvAnTimeHigh       0x0068  // High-Order 32 Bits of Time Stamp Counter
#define  U32_EvAnTimeLow        0x006C  // Low-Order 32 Bits of Time Stamp Counter

//=====================
// Sequence RAM Control Registers
//
#define  U32_Seq1Control        0x0070  // Sequencer 1 Control Register
#define  U32_Seq2Control        0x0074  // Sequencer 2 Control Register
#define  U32_SeqControl_base    0x0070  // Sequencer Control Register Array Base
#define  U32_SeqControl(n)      (U32_SeqControl_base + (4*(n)))

#define  U8_SeqTrigSrc_base     0x0073
#define  U8_SeqTrigSrc(n)       (U8_SeqTrigSrc_base + (4*(n)))


//=====================
// Fractional Synthesizer Control Word
//
#define  U32_FracSynthWord      0x0080  // RF Reference Clock Pattern (Micrel SY87739L)

//=====================
// RF Recovery
//
#define  U32_RxInitPS           0x0088  // Initial Value For RF Recovery DCM Phase

//=====================
// Trigger Event Control Registers
//
#define  U32_TrigEventCtrl_base 0x0100  // Trigger Event Control Register Array Base
#define  U32_TrigEventCtrl(n)   (U32_TrigEventCtrl_base + (4*(n)))

#define  U8_TrigEventCode(n)    (U32_TrigEventCtrl(n) + 3)

#define  EVG_TRIG_EVT_ENA       0x00000100

//=====================
// Multiplexed Counter Control Register Arrays
//
#define  U32_MuxControl_base    0x0180  // Mux Counter Control Register Base Offset
#define  U32_MuxPrescaler_base  0x0184  // Mux Counter Prescaler Register Base Offset

#define  U32_MuxControl(n)      (U32_MuxControl_base + (8*(n)))

#define  U32_MuxPrescaler(n)    (U32_MuxPrescaler_base + (8*(n)))
#define  U8_MuxTrigMap(n)       (U32_MuxControl(n) + 3)

//=====================
// Front Panel Output Mapping Register Array
//
#define  U16_FrontOutMap_base      0x0400  // Front Output Port Mapping Register Offset
#define  U16_FrontOutMap(n)        (U16_FrontOutMap_base + (2*(n)))

//=====================
// Front Panel Universal Output Mapping Register Array
//
#define  U16_UnivOutMap_base    0x0440  // Front Univ Output Mapping Register
#define  U16_UnivOutMap(n)      (U16_UnivOutMap_base + (2*(n)))

//=====================
// Rear Universal Output Mapping Register Array
//
#define  U16_RearOutMap_base    0x0480  // Rear Univ Output Mapping Register
#define  U16_RearOutMap(n)      (U16_RearOutMap_base + (2*(n)))


//=====================
// Front Panel Input Mapping Registers
//
#define  U32_FrontInMap_base       0x0500  // Front Input Port Mapping Register
#define  U32_FrontInMap(n)         (U32_FrontInMap_base + (4*(n)))

//=====================
// Front Panel Input Phase Monitoring Registers
//
#define U32_FrontInPhaseMon_base 0x0520
#define U32_FrontInPhaseMon(n)   (U32_FrontInPhaseMon_base + (4*(n)))

//=====================
// Front Panel Universal Input Mapping Registers
//
#define  U32_UnivInMap_base     0x0540  // Front Univ Input Port Mapping Register
#define  U32_UnivInMap(n)       (U32_UnivInMap_base + (4*(n)))


//=====================
// Rear Universal Input Mapping Registers
//
#define  U32_RearInMap_base       0x0600  // Rear Univ Input Port Mapping Register
#define  U32_RearInMap(n)         (U32_RearInMap_base + (4*(n)))

//=====================
// Data Buffer Area
//
#define  U8_DataTxBaseEvg       0x0800  // Data Buffer Array Base Offset
#define  U8_DataTxBaseEvg_seg   0x2000  // Data Buffer Array Base Offset for firmware version 207+ (segmented data buffer)
#define  U8_DataBuffer(n)       (U8_DataTxBaseEvg + n)
#define  U8_DataBuffer_seg(n)   (U8_DataTxBaseEvg_seg + n)

//=====================
// SFP Transceiver EEPROM and diagnostics
//
#define  U32_SFP_base           0x1000
#define  U32_SFP_transceiver    (U32_SFP_base + 0x200)      // in EVG function register map
#define  U32_SFP(n)             (U32_SFP_base + (512*(n)))    // in FCT function register map


//=====================
// Sequence RAMs
//
#define  U32_SeqRamTS_base      0x8000  // Sequence Ram Timestamp Array Base Offset
#define  U32_SeqRamTS(n,m)      (U32_SeqRamTS_base + (0x4000*(n)) + (8*(m)))

#define  U8_SeqRamEvent_base    0x8007  // Sequence Ram Event Code Array Base Offset
#define  U8_SeqRamEvent(n,m)    (U8_SeqRamEvent_base + (0x4000*(n)) + (8*(m)))

#define  U8_SeqRamMask_base    0x8006  // Sequence Ram Event Code Array Base Offset
#define  U8_SeqRamMask(n,m)    (U8_SeqRamMask_base + (0x4000*(n)) + (8*(m)))

//=====================
// Size of Event Generator Register Space
//
#define  EVG_REGMAP_SIZE        0x10000  // Register map size is 64K



/**************************************************************************************************/
/*    AC Trigger Register Bit Assignmen                                                           */
/**************************************************************************************************/

#define  EVG_AC_TRIG_BYP            0x02
#define  EVG_AC_TRIG_SYNC_MASK      0x0D
#define  EVG_AC_TRIG_SYNC_EVTCLK    0x00
#define  EVG_AC_TRIG_SYNC_MXC7      0x01
#define  EVG_AC_TRIG_SYNC_FPIN1     0x05
#define  EVG_AC_TRIG_SYNC_FPIN2     0x09

/**************************************************************************************************/
/*    Interrupt Flag Register (0x0008) and Interrupt Enable Register (0x000c) Bit Assignments     */
/**************************************************************************************************/

#define  EVG_IRQ_ENABLE         0x80000000  // Master Interrupt Enable Bit
#define  EVG_IRQ_PCIIE          0x40000000
#define  EVG_IRQ_STOP_RAM_BASE  0x00001000  // Sequence RAM Stop Interrupt Bit
#define  EVG_IRQ_STOP_RAM(N)    (EVG_IRQ_STOP_RAM_BASE<<N)
#define  EVG_IRQ_START_RAM_BASE 0x00000100  // Sequence RAM Start Interrupt Bit
#define  EVG_IRQ_START_RAM(N)   (EVG_IRQ_START_RAM_BASE<<N)
#define  EVG_IRQ_EXT_INP        0x00000040  // External Input Interrupt Bit
#define  EVG_IRQ_DBUFF          0x00000020  // Data Buffer Interrupt Bit
#define  EVG_IRQ_FIFO           0x00000002  // Event FIFO Full Interrupt Bit
#define  EVG_IRQ_RXVIO          0x00000001  // Receiver Violation Bit

/**************************************************************************************************/
/*    Outgoing Event Link Clock Source Register (0x0050) Bit Assignments                          */
/**************************************************************************************************/

#define  EVG_CLK_SRC_SEL      0x07  // External/Internal reference clock select
#define  EVG_CLK_SRC_INTERNAL      0
#define  EVG_CLK_SRC_EXTERNAL      1
#define  EVG_CLK_SRC_PXIE100       2
#define  EVG_CLK_SRC_RECOVERED     4
#define  EVG_CLK_SRC_EXTDOWNRATE   5  // use external RF reference for downstream ports, internal reference for upstream port, Fan-Out mode, event rate down conversion
#define  EVG_CLK_SRC_PXIE10        6
#define  EVG_CLK_SRC_RECOVERHALVED 7  // recovered clock /2 decimate mode, event rate is halved
#define  EVG_CLK_PLLLOCK        0x80
#define  EVG_CLK_BW             0x70        /* PLL Bandwidth Select (see Silicon Labs Si5317 datasheet) */
#define  EVG_CLK_BW_shift       4
#define  EVG_CLK_PH_TOGG_mask   0x80
#define  EVG_CLK_PH_TOGG_shift     7

/**************************************************************************************************/
/*    Sequence RAM Control Register (0x0070, 0x0074) Bit Assignments                              */
/**************************************************************************************************/

#define  EVG_SEQ_RAM_RUNNING    0x02000000  // Sequence RAM is Running (read only)
#define  EVG_SEQ_RAM_ENABLED    0x01000000  // Sequence RAM is Enabled (read only)

#define  EVG_SEQ_RAM_SW_TRIG    0x00200000  // Sequence RAM Software Trigger Bit
#define  EVG_SEQ_RAM_RESET      0x00040000  // Sequence RAM Reset
#define  EVG_SEQ_RAM_DISABLE    0x00020000  // Sequence RAM Disable
#define  EVG_SEQ_RAM_ARM        0x00010000  // Sequence RAM Enable/Arm

#define  EVG_SEQ_RAM_SWMASK     0x0000F000  // Sequence RAM Software mask
#define  EVG_SEQ_RAM_SWMASK_shift   12
#define  EVG_SEQ_RAM_SWENABLE   0x00000F00  // Sequence RAM Software enable
#define  EVG_SEQ_RAM_SWENABLE_shift 8

#define  EVG_SEQ_RAM_REPEAT_MASK 0x00180000 // Sequence RAM Repeat Mode Mask
#define  EVG_SEQ_RAM_NORMAL     0x00000000  // Normal Mode: Repeat every trigger
#define  EVG_SEQ_RAM_SINGLE     0x00100000  // Single-Shot Mode: Disable on completion
#define  EVG_SEQ_RAM_RECYCLE    0x00080000  // Continuous Mode: Repeat on completion


/**************************************************************************************************/
/* Multiplexed Counter                                                                            */
/**************************************************************************************************/

#define  EVG_MUX_POLARITY       0x40000000
#define  EVG_MUX_STATUS         0x80000000

/**************************************************************************************************/
/* Control Register flags                                                                         */
/**************************************************************************************************/

#define  EVG_MASTER_ENA         0x80000000
#define  EVG_DIS_EVT_REC        0x40000000
#define  EVG_REV_PWD_DOWN       0x20000000
#define  EVG_MXC_RESET          0x01000000
#define  EVG_BCGEN              0x00800000  // Delay compensation beacon generator enable
#define  EVG_DCMST              0x00400000  // Delay compensation master enable

/**************************************************************************************************/
/* Input                                                                                          */
/**************************************************************************************************/

#define  EVG_EXT_INP_IRQ_ENA      0x01000000
#define  EVG_INP_SEQ_MASK         0xF0000000
#define  EVG_INP_SEQ_MASK_shift   28
#define  EVG_INP_SEQ_ENABLE       0x0E000000
#define  EVG_INP_SEQ_ENABLE_shift 24

/**************************************************************************************************/
/* Front Panel Input Phase Monitoring Register                                                    */
/**************************************************************************************************/
#define EVG_FPInPhMon_PHCLR_mask  0x80000000
#define EVG_FPInPhMon_PHCLR_shift 31
#define EVG_FPInPhMon_DBPH_mask   0x40000000
#define EVG_FPInPhMon_DBPH_shift  30
#define EVG_FPInPhMon_PHSEL_mask  0x03000000
#define EVG_FPInPhMon_PHSEL_shift 24
#define EVG_FPInPhMon_PHFE_mask   0x00000F00
#define EVG_FPInPhMon_PHFE_shift  8
#define EVG_FPInPhMon_PHRE_mask   0x0000000F
#define EVG_FPInPhMon_PHRE_shift  0

/**************************************************************************************************/
/* FCT Function Register map                                                                                          */
/**************************************************************************************************/
// note, that fanout SFPs are defined in the SFP section

#define U32_fct_status_base     0x000   // status register
#define U32_fct_control_base    0x004   // control register
#define U32_fct_upstreamDC      0x010   // upstream data compensation delay value
#define U32_fct_fifoDC          0x014   // receive FIFO data compensation delay value
#define U32_fct_internalDC      0x018   // FCT internal datapath data compensation delay value
#define U32_fct_topologyID      0x02C   // Timing node topology ID
#define U32_fct_portDC_base     0x040   // downstream link port loop delay value
#define U32_fct_portDC(n)       (U32_fct_portDC_base + (4*(n)))

#define EVG_FCT_maxPorts 8  // ports 1 - 8

/*
 * Status register flags
 */
#define EVG_FCT_STATUS_VIOLATION_mask   0x000000FF
#define EVG_FCT_STATUS_VIOLATION_shift  0
#define EVG_FCT_STATUS_STATUS_mask      0x00FF0000
#define EVG_FCT_STATUS_STATUS_shift     16



#ifndef  EVG_CONSTANTS
#define  EVG_CONSTANTS

/*const epicsUInt16 evgNumMxc = 8;
const epicsUInt16 evgNumEvtTrig = 8;
const epicsUInt16 evgNumDbusBit = 8;
const epicsUInt16 evgNumFrontOut = 6;
const epicsUInt16 evgNumUnivOut = 4;
const epicsUInt16 evgNumFrontInp = 2;
const epicsUInt16 evgNumUnivInp = 4;
const epicsUInt16 evgNumRearInp = 16;*/
const epicsUInt16 evgNumSeqRam = 2;
const epicsUInt16 evgEndOfSeqBuf = 5;
const epicsUInt16 evgNumSFPModules = 8;

#endif

#endif /* EVGREGMAP_H */

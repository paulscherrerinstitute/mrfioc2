#ifndef SFPINFO_H
#define SFPINFO_H

#define SFPMEM_SIZE 512

/*
 * SFP module EEPROM and diagnostics register offsets
 * from start of EEPROM.
 *
 * This is a sub-set of the registers documented.
 *
 * For firmware version #5
 * as documented in EVR-MRM-004.doc
 * Jukka Pietarinen
 * 07 Apr 2011
 *
 */

/*
Type of serial transceiver: Identifier Values
Value      Description of Physical Device
----------------------------------------------
00h        Unknown or unspecified
01h        GBIC
02h        Module/connector soldered to motherboard
03h        SFP transceiver
04-7Fh     Reserved
80-FFh     Vendor specific
*/

#define SFP_type_offset 0
#define SFP_type 3
#define SFP_typeid_MASK 0xff00ff00

#define SFP_linkrate 12     // Nominal bit rate, units of 100 MBits/s

#define SFP_linkLength_9uminkm      14  // Link length supported for 9/125 um fiber, units of km
#define SFP_linkLength_9umin100m    15  // Link length supported for 9/125 um fiber, units of 100 m
#define SFP_linkLength_50umin10m    16  // Link length supported for 50/125 um fiber, units of 10 m
#define SFP_linkLength_62umin10m    17  // Link length supported for 62.5/125 um fiber, units of 10 m
#define SFP_linkLength_copper       18  // Link length supported for copper, units of meters

#define SFP_bitRateMargin_upper     66  // Upper bit rate margin, units of %
#define SFP_bitRateMargin_lower     67  // Lower bit rate margin, units of %


/* Status/control register
 * bit 7:   TX_DISABLE State
 * bit 6-3: Reserved
 * bit 2:   TX_FAULT State
 * bit 0:   Data ready (Bar)
 */
#define SFP_status                  366

/* 16 byte ascii string identifiers */
#define SFP_vendor_name 20
#define SFP_part_num 40
#define SFP_serial 68

/* 4 byte string */
#define SFP_part_rev 56
#define SFP_man_date 84 /* YYMM, eg. 1004 == Apr 2010 */

/* 2 byte unsigned integer */
#define SFP_vccPower    354 // Real time VCC Power: supply voltage decoded as unsigned integer in increments of 100 uV

/* two byte 2s complement signed */
#define SFP_temp 352
#define SFP_tx_pwr 358
#define SFP_rx_pwr 360

#endif // SFPINFO_H

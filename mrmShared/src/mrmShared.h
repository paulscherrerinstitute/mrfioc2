#ifndef MRMSHARED_H
#define MRMSHARED_H

#include <string>
#include <epicsTypes.h>

//VME
struct configuration_vme{
    epicsInt32 slot;        // slot where the card is inserted
    epicsUInt32 address;    // VME address in A24 space
    epicsInt32 irqLevel;    // interupt level
    epicsInt32 irqVector;   // interrupt vector
    std::string position;   // position description for EVR
};


// PCI
struct configuration_pci{
    int bus;        // Bus number
    int device;     // Device number
    int function;   // Function number
};

enum busType{
    busType_vme = 0,
    busType_pci = 1
};

struct bus_configuration{
    struct configuration_vme vme;
    struct configuration_pci pci;
    enum busType busType;
};

// form factor corresponds to FPGA Firmware Version Register bit 26-24
enum formFactor {
  formFactor_unknown = -1,
  formFactor_CPCI=0, // 3U
  formFactor_PMC=1,
  formFactor_VME64=2,
  formFactor_CRIO=3,
  formFactor_CPCIFULL=4, // 6U
  formFactor_PXIe=6,
  formFactor_PCIe=7,
  formFactor_embedded=20,
  formFactor_last = formFactor_embedded
};

enum deviceSeries {
    series_unknown = 0,
    series_300 = 1,
    series_300DC = 2,
    series_230 = 3
};

typedef struct device_info {
    enum formFactor formFactor;
    struct bus_configuration bus;
    enum deviceSeries series;
} deviceInfoT;


/* PLL Bandwidth Select (see Silicon Labs Si5317 datasheet)
 *  000 – Si5317, BW setting HM (lowest loop bandwidth)
 *  001 – Si5317, BW setting HL
 *  010 – Si5317, BW setting MH
 *  011 – Si5317, BW setting MM
 *  100 – Si5317, BW setting ML (highest loop bandwidth)
 */
enum PLLBandwidth {
    PLLBandwidth_HM=0,
    PLLBandwidth_HL=1,
    PLLBandwidth_MH=2,
    PLLBandwidth_MM=3,
    PLLBandwidth_ML=4,
    PLLBandwidth_MAX=PLLBandwidth_ML
};


////////////////////////////////////////
/** Common registers for EVM and EVR **/
////////////////////////////////////////

//=====================
// General Control Register
//
#define U32_Control     0x004


//=====================
// General Status Register
//
#define U32_Status      0x000

//=====================
// FPGA firmware version register
//
#define U32_FWVersion   0x02C
#  define FWVersion_type_mask 0xF0000000
#  define FWVersion_type_shift 28
#  define FWVersion_form_mask 0x0F000000
#  define FWVersion_form_shift 24
#  define FWVersion_ver_mask  0x0000FFFF
#  define FWVersion_ver_shift  0
#  define FWVersion_verMinor_mask  0x00FF0000
#  define FWVersion_verMinor_shift  16


//=====================
// SPI register offsets
//
#define U32_SpiData    0x0A0
#define U32_SpiCtrl    0x0A4
  #define SpiCtrl_rrdy         (0x0040) // Receiver ready. If '1', data byte waiting in U32_SpiData.
  #define SpiCtrl_trdy         (0x0020) // Transmitter ready. If '1', U32_SpiData is ready to accept new transmit data byte.
  #define SpiCtrl_tmt          (0x0010) // Transmitter empty. If '1', data byte has been transmitted.
  #define SpiCtrl_oe           (0x0002) // Output enable for SPI pins. '1' enables SPI pins.
  #define SpiCtrl_slaveSelect  (0x0001) // Slave select output enable for SPI slave device. '1' means that the device is selected.

//=====================
// Data buffer register offsets and defines
//

// Tx control register offsets
#define DataTxCtrl_saddr_mask 0xFF000000    // Transfer start segment address (SADDR)
#define DataTxCtrl_saddr_shift 24
#define DataTxCtrl_done 0x100000    // Transmission complete (CPT)
#define DataTxCtrl_run  0x080000    // Transmission running (RUN)
#define DataTxCtrl_trig 0x040000    // Trigger transmission (TRIG)
#define DataTxCtrl_ena  0x020000    // Tx engine enable (ENA)
#define DataTxCtrl_mode 0x010000    // Data buffer and/or DBus mode selection (EN)
#define DataTxCtrl_len_mask 0x0007fc


#define DataBuffer_SegmentIRQ       0x8F80   // 4x32 bit
#define DataBufferFlags_checksum     0x8FA0   // 4x32 bit, each bit for one segment. 0 = Checksum OK
#define DataBufferFlags_overflow    0x8FC0   // 4x32 bit, each bit for one segment.
#define DataBufferFlags_rx          0x8FE0   // 4x32 bit
#define DataBuffer_RXSize(N)   (4*N+0x8800)  // 128x32 bit registers which contain segment received length


// Rx control register offsets
#  define DataRxCtrl_rx       0x8000    // Write 1 to set up for reception, read for run status (RX/ENA)
#  define DataRxCtrl_stop     0x4000    // Write 1 to stop, read for complete status (RDY/DIS)
#  define DataRxCtrl_rdy      DataRxCtrl_stop    // Data buffer Rx complete status (RDY)
#  define DataRxCtrl_sumerr   0x2000    // Checksum error (CS)
#  define DataRxCtrl_mode     0x1000    // Data buffer and/or DBus mode selection (EN)
#  define DataRxCtrl_len_mask 0x0fff

// misc
#define DataBuffer_segment_length 16    // Length of a single segment in a segmented data buffer
#define DataBuffer_len_max  DataTxCtrl_len_mask // Maximum supported length of the data buffer

#define MIN_FW_300_SERIES  0x207
#define START_FW_300_SERIES  0x200

////////////////////
/** Misc defines **/
////////////////////
// printf formatting for size_t differs on windows
#ifdef _WIN32
    #define FORMAT_SIZET_U "Iu"
    #define FORMAT_SIZET_X "Ix"
#else
    #define FORMAT_SIZET_U "zu"
    #define FORMAT_SIZET_X "zx"
#endif

#endif // MRMSHARED_H

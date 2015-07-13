/*
  pci_mrfev.h -- Definitions for Micro-Research Event Generator /
                 Event Receiver Linux 2.6 driver

  Author: Jukka Pietarinen (MRF)
  Date:   29.11.2006

*/

#define DEVICE_MINOR_NUMBERS      4
#define MAX_MRF_DEVICES           8
#define MAX_MRF_BARS              3

#define PCI_VENDOR_ID_LATTICE       0x1204
#define PCI_DEVICE_ID_LATTICE_ECP3  0xec30
#define PCI_VENDOR_ID_XILINX        0x10ee
#define PCI_DEVICE_ID_ZOMOJO_Z1     0x0007
#define PCI_DEVICE_ID_0505          0x0505
#define PCI_DEVICE_ID_KINTEX7       0x7011
#define PCI_DEVICE_ID_PLX_9056      0x9056

#define PCI_VENDOR_ID_MRF           0x1A3E
#define PCI_DEVICE_ID_MRF_PMCEVR200 0x10C8
#define PCI_DEVICE_ID_MRF_PMCEVR230 0x11E6
#define PCI_DEVICE_ID_MRF_PXIEVR220 0x10DC
#define PCI_DEVICE_ID_MRF_PXIEVG220 0x20DC
#define PCI_DEVICE_ID_MRF_PXIEVR230 0x10E6
#define PCI_DEVICE_ID_MRF_PXIEVG230 0x20E6
#define PCI_DEVICE_ID_MRF_CPCIEVR300 0x152C
#define PCI_DEVICE_ID_MRF_PCIEEVR300 0x172C
#define PCI_DEVICE_ID_MRF_CPCIEVG300 0x252C
#define PCI_DEVICE_ID_MRF_PXIEEVR300 0x112C
#define PCI_DEVICE_ID_MRF_PXIEEVG300 0x212C
#define PCI_DEVICE_ID_MRF_CPCIEVRTG 0x192C
#define PCI_DEVICE_ID_MRF_CPCIFCT   0x30E6
#define PCI_DEVICE_ID_MRF_MTCAEVR300 0x132C
#define PCI_DEVICE_ID_MRF_MTCAEVG300 0x232C

#define MODULE_VENDOR_ID_NOCONF     PCI_VENDOR_ID_PLX
#define MODULE_DEVICE_ID_NOCONF     PCI_DEVICE_ID_PLX_9030
#define MODULE_SUBVENDOR_ID_NOCONF  0
#define MODULE_SUBDEVICE_ID_NOCONF  0

#define DEVICE_MINORS 4
#define DEVICE_FIRST  0
#define DEVICE_EEPROM 0
#define DEVICE_FLASH  1
#define DEVICE_FPGA   2
#define DEVICE_EV     3
#define DEVICE_LAST   3

/* Define the maximum number of words in EEPROM */
#define EEPROM_MAX_WORDS            256

/* Define space needed for data */
#define EEPROM_DATA_ALLOC_SIZE      0x00002000
/*
#define XCF_DATA_ALLOC_SIZE         0x00100000
#define FPGA_DATA_ALLOC_SIZE        0x00100000
*/
#define XCF_DATA_ALLOC_SIZE         0x000400
#define FPGA_DATA_ALLOC_SIZE        0x001000
#define FLASH_DATA_ALLOC_SIZE       (256)
#define FLASH_SECTOR_SIZE           (0x00010000)
#define FLASH_SECTOR_PRIMARY_START  (0x00010000)
#define FLASH_SECTOR_PRIMARY_END    (0x00260000)

#define XCF_BLOCK_SIZE              1024
#define XCF_ERASE_TCKS              140000000

#define EV_IRQ_FLAG_OFFSET          (0x0008)
#define EV_IRQ_ENABLE_OFFSET        (0x000C)
#define EV_IRQ_PCI_DRIVER_ENA       (0x40000000)

#define EV_SPI_DATA_OFFSET          (0x00A0)
#define EV_SPI_CONTROL_OFFSET       (0x00A4)
#define EV_SPI_CONTROL_RRDY         (0x0040)
#define EV_SPI_CONTROL_TRDY         (0x0020)
#define EV_SPI_CONTROL_TMT          (0x0010)
#define EV_SPI_CONTROL_OE           (0x0002)
#define EV_SPI_CONTROL_SELECT       (0x0001)
#define SPI_RETRY_COUNT             (10000)
#define SPI_BE_COUNT                (0x10000000)

#define M25P_FAST_READ              (0x0B)
#define M25P_RDID                   (0x9F)
#define M25P_WREN                   (0x06)
#define M25P_WRDI                   (0x04)
#define M25P_RDSR                   (0x05)
#define M25P_SE                     (0xD8)
#define M25P_BE                     (0xC7)
#define M25P_PP                     (0x02)
#define M25P_STATUS_WIP             (0x01)

#define MRF_DEVTYPE_V2P_9030        (0x00000001)
#define MRF_DEVTYPE_V5_9056         (0x00000002)
#define MRF_DEVTYPE_V5_PCIE         (0x00000004)
#define MRF_DEVTYPE_ECP3_PCI        (0x00000008)
#define MRF_DEVTYPE_ECP3_PCIE       (0x00000010)
#define MRF_DEVTYPE_K7_PCIE         (0x00000020)


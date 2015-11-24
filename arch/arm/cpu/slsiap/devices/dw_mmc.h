#ifndef __DW_MMC_HW_H__
#define __DW_MMC_HW_H__

/* add register */
#define DWMCI_CLKCTRL			0x114

/* CLK DELAY SHIFT Register*/
#define DW_MMC_DRIVE_DELAY(n)       ((n & 0xFF) << 0)   // write
#define DW_MMC_DRIVE_PHASE(n)       ((n & 0x03) <<16)   // write
#define DW_MMC_SAMPLE_DELAY(n)      ((n & 0xFF) << 8)   // read
#define DW_MMC_SAMPLE_PHASE(n)      ((n & 0x03) <<24)   // read/* CLK DELAY SHIFT Register*/

#endif /* __DW_MMC_HW_H__ */
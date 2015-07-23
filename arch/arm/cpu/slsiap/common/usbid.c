#include <common.h>
#include <nx_chip.h>
#include <nx_ecid.h>
#include "usbid.h"

#if (0)
#define	NX_DEBUG_MSG(args...) 	printf(args)
#else
#define	NX_DEBUG_MSG(args...) 	do{}while(0)
#endif

void CalUSBID(U16 *VID, U16 *PID, U32 ECID)
{
	if (ECID == 0) {   // ECID is not burned
		*VID = USBD_VID;
        *PID = USBD_PID;
#if defined(CONFIG_MACH_S5P6818)
		if (*PID == USBD_PID)
			*PID = USBD_5430_PID;
		NX_DEBUG_MSG("\nECID Null!!\nVID %x, PID %x\n", *VID, *PID);
#endif
    } else {
        *VID = (ECID>>16)&0xFFFF;
        *PID = (ECID>> 0)&0xFFFF;
#if defined(CONFIG_MACH_S5P6818)
		if (*PID == USBD_PID)
			*PID = USBD_5430_PID;
		NX_DEBUG_MSG("VID %x, PID %x\n", *VID, *PID);
#endif
	}
}

void GetUSBID(U16 *VID, U16 *PID)
{
	struct NX_ECID_RegisterSet * const pECIDReg = (struct NX_ECID_RegisterSet *)PHY_BASEADDR_ECID_MODULE;
	
	U32 ID = pECIDReg->ECID[3];
	
	CalUSBID(VID, PID, ID);
}


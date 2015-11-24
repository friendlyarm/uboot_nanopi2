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
	struct NX_ECID_RegisterSet * const pECIDReg = (struct NX_ECID_RegisterSet *)PHY_BASEADDR_ECID_MODULE;

	U32 i; 
	char NAME[49];
	char *cmp_name = "NEXELL-NXP4330-R0-LF3000";

    for( i = 0 ; i < 48 ; i ++ ) {   
        NAME[i] = (char)pECIDReg->CHIPNAME[i];
    }   
    for( i = 0; i < 48; i++) {   
        if( (NAME[i] == '-') && (NAME[i+1] == '-') ) {   
            NAME[i] = 0;  
            NAME[i+1] = 0;
        }   
    }   
		
    if (strcmp(NAME, cmp_name) == 0) {
		NX_DEBUG_MSG("\nNXP4330!!\n%s\n", NAME);
		*VID = USBD_VID;
        *PID = USBD_PID;
	} else {
		if (ECID == 0) {   // ECID is not burned
			*VID = USBD_VID;
	        *PID = USBD_PID;
#if defined(CONFIG_MACH_S5P6818)
			if (*PID == USBD_PID)
				*PID = USBD_5430_PID;
#endif
			NX_DEBUG_MSG("\nECID Null!!\nVID %x, PID %x\n", *VID, *PID);
	    } else {
	        *VID = (ECID>>16)&0xFFFF;
	        *PID = (ECID>> 0)&0xFFFF;
#if defined(CONFIG_MACH_S5P6818)
			if (*PID == USBD_PID)
				*PID = USBD_5430_PID;
#endif
			NX_DEBUG_MSG("VID %x, PID %x\n", *VID, *PID);
		}
    } 
}

void GetUSBID(U16 *VID, U16 *PID)
{
	struct NX_ECID_RegisterSet * const pECIDReg = (struct NX_ECID_RegisterSet *)PHY_BASEADDR_ECID_MODULE;
	
	U32 ID = pECIDReg->ECID[3];
	
	CalUSBID(VID, PID, ID);
}


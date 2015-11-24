#include <common.h>
#include <watchdog.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm/errno.h>
#include <platform.h>

#ifndef CONFIG_WATCHDOG_TIMEOUT
#define CONFIG_WATCHDOG_TIMEOUT	10
#endif

void hw_watchdog_reset(void)
{
	// reload count value
	if (NX_WDT_GetEnable(0)) {
		NX_WDT_ClearInterruptPending(0, NX_WDT_GetInterruptNumber(0));
		NX_WDT_SetCurrentCount(0, NX_WDT_GetReloadCount(0));
	}
}

void hw_watchdog_restart(void)
{
	// enable hw watchdog
	if (!NX_WDT_GetEnable(0)) {
		NX_WDT_SetCurrentCount(0, CONFIG_WATCHDOG_TIMEOUT * 6103/* about 1 sec */);
		NX_WDT_SetResetEnable(0, CTRUE);
		NX_WDT_SetEnable(0, CTRUE);
	} 
}

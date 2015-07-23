#ifndef __ASM_ARM_ARCH_CLK_H_
#define __ASM_ARM_ARCH_CLK_H_

#include <linux/spinlock.h>
#include <clkdev.h>

#define	__init
struct device {
	char *init_name;
};

#define	dev_name(d)		(d->init_name)
#define	EXPORT_SYMBOL(_f)

#endif

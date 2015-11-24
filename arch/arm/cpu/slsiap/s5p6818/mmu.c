
#include <common.h>
#include <asm/io.h>
#include <platform.h>
#include <mach-api.h>

#include <command.h>
#include <asm/sections.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <asm/armv7.h>

#include <platform.h>
#include <mach-types.h>
#include <asm/arch/mach-api.h>

DECLARE_GLOBAL_DATA_PTR;
extern ulong _bss_end_ofs;		/* BSS end relative to _start */

#define SMP_CACHE_COHERENCY
#define SMP_SCU_ENABE

/*------------------------------------------------------------------------------
 * Stack area that to be used after mmu set
 */
#define ACCESS_ALIGN		4
#define PAGE_DOMAIN			0	// use domain 0, domain 0 is setted client mode in util.s

#define BSS_END_OPS				_bss_end_ofs
#define PAGE_TABLE_START		((CONFIG_SYS_TEXT_BASE + BSS_END_OPS) & 0xffff0000) + 0x10000
#define PAGE_TABLE_SIZE			0x0000c000

enum {
	FLD_FAULT 	= 0,
	FLD_COARSE 	= 1,
	FLD_SECTION = 2,
	FLD_FINE	= 3
};

enum {
	AP_FAULT 	= 0,
	AP_CLIENT 	= 1,
	AP_RESERVED = 2,
	AP_MANAGER 	= 3
};

enum {
	SECTION_BUFFERABLE 	= 2,
	SECTION_CACHEABLE  	= 3,
	SECTION_SBO 		= 4,
	SECTION_DOMAIN 		= 5,
	SECTION_AP 			= 10,
	SECTION_SHARED		= 16,
};

/*------------------------------------------------------------------------------
 * Virtual Map Address Table
 */
static const u32 ptable[] = {
 	#include "page_map.h"
};

static void make_page_table(u32 *ptable)
{
	int	index;
	unsigned int temp, virt, phys, num_of_MB;
	unsigned int mode, i, addr, data;
	unsigned int *table;

	table = (u32*)ptable;

	/* Clear page table zone (16K). */
	memset((char*)PAGE_TABLE_START, 0x00, 4096*4);

	/*
	 * ROM Base: Non-Cacheable, NO-Bufferable
	 */
	index = 0;
	mode  = (0<<SECTION_CACHEABLE) | (0<<SECTION_BUFFERABLE) | (FLD_SECTION<<0);	// No Cachable & No Bufferable
#ifdef SMP_CACHE_COHERENCY
	mode = 0xC16;
#else
	mode |= (1<<SECTION_SHARED);
	mode  = mode | AP_CLIENT<<SECTION_AP;					/* set kernel R/W permission */
#endif

    virt = 0, temp = 0, phys = 0;
    num_of_MB = 1;

   	addr = PAGE_TABLE_START + ((virt>>20)*ACCESS_ALIGN);
   	for (i=0; i < num_of_MB; i++) {
   		data = phys | mode;
       *(volatile unsigned int *)(addr)= data;
       	phys += 1<<20;
    	addr += ACCESS_ALIGN;
    }

	/*
	 * Cacheable, Bufferable, R/W
	 */
	index = 0;
	mode  = (1<<SECTION_CACHEABLE) | (1<<SECTION_BUFFERABLE) | (FLD_SECTION<<0);	// Cachable & Bufferable
#ifdef SMP_CACHE_COHERENCY
	mode = 0x15C06;
	mode = 0x15C06 | (1 << SECTION_CACHEABLE);	/* Cacheable */
#else
	mode |= (1<<SECTION_SHARED);
	mode  = mode | AP_CLIENT<<SECTION_AP;					// set kernel R/W permission
#endif

	while (1) {
		temp = *(table+index++) & 0xfff<<20;
		virt = *(table+index++) & 0xfff<<20;
		phys = *(table+index++) & 0xfff<<20;
		num_of_MB = *(table+index++);

		if (num_of_MB == 0)
			break;

		if (0 != virt) {
			addr = PAGE_TABLE_START + ((virt>>20)*ACCESS_ALIGN);
			for (i=0; i < num_of_MB; i++) {
				data = phys | mode;
				*(volatile unsigned int *)(addr)= data;
				phys += 1<<20;
				addr += ACCESS_ALIGN;
			}
		}
	}

	/*
	 * No Cacheable, No Bufferable, R/W
	 */
	index = 0;
	mode = (0<<SECTION_CACHEABLE) | (0<<SECTION_BUFFERABLE) | (FLD_SECTION<<0);	// No Cachable & No Bufferable
#ifdef SMP_CACHE_COHERENCY
	mode = 0xC16;
#else
	mode = mode | AP_CLIENT<<SECTION_AP;					// set kernel R/W permission
#endif

	while (1) {
		virt = *(table+index++) & 0xfff<<20;
		temp = *(table+index++) & 0xfff<<20;
		phys = *(table+index++) & 0xfff<<20;
		num_of_MB = *(table+index++);

		if (num_of_MB==0)
			break;

		if (0 != virt) {
			addr = PAGE_TABLE_START + ((virt>>20)*ACCESS_ALIGN);
			for (i=0; i < num_of_MB; i++) {
				data = phys | mode;
				*(volatile unsigned int *)(addr)= data;
				phys += 1<<20;
				addr += ACCESS_ALIGN;
			}
		}
	}
	return;
}

extern void arm_init_before_mmu(void);
extern void enable_mmu(unsigned);

void mmu_on(void)
{
	void *vector_base = (void *)0xFFFF0000;

	dcache_disable();
	arm_init_before_mmu();				/* Flush DCACHE */

	/* copy vector table */
	memcpy(vector_base, (void const *)CONFIG_SYS_TEXT_BASE, 64);

	/* set CCI-400 */
	#define	CCI_REG	0xe0090000
	writel(0x8, (CCI_REG + 0x0000));	// CCI
	writel(0x0, (CCI_REG + 0x1000));	// S0 : coresight
 	writel(0x0, (CCI_REG + 0x2000));	// S1 : bottom bus
	writel(0x0, (CCI_REG + 0x3000));	// S2 : top bus
	writel((0x3<<30) | 0x3, (CCI_REG + 0x4000));	// S3: cpu cluster 1
	writel((0x3<<30) | 0x3, (CCI_REG + 0x5000));	// S4: cpu cluster 0

	mmu_page_table_flush(PAGE_TABLE_START, PAGE_TABLE_SIZE);
	make_page_table((u32*)ptable);		/* 	Make MMU PAGE TABLE	*/

	enable_mmu(PAGE_TABLE_START);
}

#if defined (CONFIG_SMP)
void mmu_secondary_on(void)
{
	dcache_disable();
	arm_init_before_mmu();				/* Flush DCACHE */

	mmu_page_table_flush(PAGE_TABLE_START, PAGE_TABLE_SIZE);
	enable_mmu(PAGE_TABLE_START);
}
#endif

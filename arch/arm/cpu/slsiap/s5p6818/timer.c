/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <platform.h>
#include <mach-api.h>

/* degug print */
#if	(0)
#define pr_debug(msg...)		printf(msg)
#else
#define pr_debug(msg...)		do {} while (0)
#endif

#if (CFG_TIMER_SYS_TICK_CH > 3)
#error Not support timer channel "0~3"
#endif

/* global variables to save timer count */
static unsigned long timestamp;
static unsigned long lastdec;
/* set .data section, before u-boot is relocated */
static int	 timerinit __attribute__ ((section(".data"))) = 0;
static DEFINE_SPINLOCK(time_lock);

/* macro to hw timer tick config */
static long	TIMER_FREQ  = 1000000;
static long	TIMER_HZ    = 1000000 / CONFIG_SYS_HZ;
static long	TIMER_COUNT = 0xFFFFFFFF;

#define	REG_TCFG0				(0x00)
#define	REG_TCFG1				(0x04)
#define	REG_TCON				(0x08)
#define	REG_TCNTB0				(0x0C)
#define	REG_TCMPB0				(0x10)
#define	REG_TCNT0				(0x14)
#define	REG_CSTAT				(0x44)

#define	TCON_BIT_AUTO			(1<<3)
#define	TCON_BIT_INVT			(1<<2)
#define	TCON_BIT_UP				(1<<1)
#define	TCON_BIT_RUN			(1<<0)
#define TCFG0_BIT_CH(ch)		(ch == 0 || ch == 1 ? 0 : 8)
#define TCFG1_BIT_CH(ch)		(ch * 4)
#define TCON_BIT_CH(ch)			(ch ? ch * 4  + 4 : 0)
#define TINT_CH(ch)				(ch)
#define TINT_CSTAT_BIT_CH(ch)	(ch + 5)
#define	TINT_CSTAT_MASK	(0x1F)
#define TIMER_TCNT_OFFS	(0xC)

/*
 * Timer HW
 */
static inline void timer_clock(void __iomem *base, int ch, int mux, int scl)
{
	u32 val= readl(base + REG_TCFG0) & ~(0xFF   << TCFG0_BIT_CH(ch));

	writel(val | ((scl-1)<< TCFG0_BIT_CH(ch)), base + REG_TCFG0);
	val = readl(base + REG_TCFG1) & ~(0xF << TCFG1_BIT_CH(ch));
	writel(val | (mux << TCFG1_BIT_CH(ch)), base + REG_TCFG1);
}

static inline void timer_count(void __iomem *base, int ch, unsigned int cnt)
{
	writel((cnt-1), base + REG_TCNTB0 + (TIMER_TCNT_OFFS * ch));
	writel((cnt-1), base + REG_TCMPB0 + (TIMER_TCNT_OFFS * ch));
}

static inline void timer_start(void __iomem *base, int ch)
{
	int on = 0;
	u32 val = readl(base + REG_CSTAT) & ~(TINT_CSTAT_MASK<<5 | 0x1 << ch);

	writel(val| (0x1 << TINT_CSTAT_BIT_CH(ch) | on << ch), base + REG_CSTAT);
	val = readl(base + REG_TCON) & ~(0xE << TCON_BIT_CH(ch));
	writel(val | (TCON_BIT_UP << TCON_BIT_CH(ch)), base + REG_TCON);

	val &= ~(TCON_BIT_UP << TCON_BIT_CH(ch));
	val |= ((TCON_BIT_AUTO | TCON_BIT_RUN)  << TCON_BIT_CH(ch));
	writel(val, base + REG_TCON);
	dmb();
}

static inline void timer_stop(void __iomem *base, int ch)
{
	int on = 0;
	u32 val = readl(base + REG_CSTAT) & ~(TINT_CSTAT_MASK<<5 | 0x1 << ch);

	writel(val | (0x1 << TINT_CSTAT_BIT_CH(ch) | on << ch), base + REG_CSTAT);
	val = readl(base + REG_TCON) & ~(TCON_BIT_RUN << TCON_BIT_CH(ch));
	writel(val, base + REG_TCON);
}

static inline unsigned long timer_read(void __iomem *base, int ch)
{
	return (TIMER_COUNT - readl(base + REG_TCNT0 + (TIMER_TCNT_OFFS * ch)));
}

int timer_init(void)
{
	struct clk *clk = NULL;
	char name[16] = "pclk";
	int ch = CFG_TIMER_SYS_TICK_CH;
	unsigned long rate, tclk = 0;
	unsigned long mout, thz, cmp = -1UL;
	int tcnt, tscl = 0, tmux = 0;
	int mux = 0, scl = 0;
	int version = nxp_cpu_version();
	void __iomem *base = (void __iomem *)IO_ADDRESS(PHY_BASEADDR_TIMER);

	if (timerinit)
		return 0;

	/* get with PCLK */
	clk  = clk_get(NULL, name);
   	rate = clk_get_rate(clk);
   	for (mux = 0; 5 > mux; mux++) {
   		mout = rate/(1<<mux), scl = mout/TIMER_FREQ, thz = mout/scl;
   		if (!(mout%TIMER_FREQ) && 256 > scl) {
   			tclk = thz, tmux = mux, tscl = scl;
   			break;
   		}
		if (scl > 256)
			continue;
		if (abs(thz-TIMER_FREQ) >= cmp)
			continue;
		tclk = thz, tmux = mux, tscl = scl;
		cmp = abs(thz-TIMER_FREQ);
   	}
	tcnt = tclk;	/* Timer Count := 1 Mhz counting */

	/* get with CLKGEN */
	if (version) {
		sprintf(name, "%s.%d", DEV_NAME_TIMER, ch);
		clk  = clk_get(NULL, name);
		rate = clk_round_rate(clk, TIMER_FREQ);
		if (abs(tclk - TIMER_FREQ) >= abs(rate - TIMER_FREQ)) {
			clk_set_rate(clk, rate);
			tmux = 5, tscl = 1, tcnt = TIMER_FREQ;
		}
	}

   	TIMER_FREQ = tcnt;	/* Timer Count := 1 Mhz counting */
   	TIMER_HZ = TIMER_FREQ / CONFIG_SYS_HZ;
	tcnt = TIMER_COUNT == 0xFFFFFFFF ? TIMER_COUNT + 1 : tcnt;

	/* reset control: Low active ___|---   */
	NX_RSTCON_SetRST(RESETINDEX_OF_TIMER_MODULE_PRESETn, RSTCON_ASSERT);
	NX_RSTCON_SetRST(RESETINDEX_OF_TIMER_MODULE_PRESETn, RSTCON_NEGATE);

	timer_stop(base, ch);
	timer_clock(base, ch, tmux, tscl);
	timer_count(base, ch, tcnt);
	timer_start(base, ch);

	reset_timer_masked();
	timerinit = 1;

	return 0;
}

void reset_timer(void)
{
	reset_timer_masked();
}

unsigned long get_timer(unsigned long base)
{
	unsigned long time = get_timer_masked();
	unsigned long hz = TIMER_HZ;
	return (time/hz - base);
}

void set_timer(unsigned long t)
{
	timestamp = (unsigned long)t;
}

void reset_timer_masked(void)
{
	void __iomem *base = (void __iomem *)IO_ADDRESS(PHY_BASEADDR_TIMER);
	int ch = CFG_TIMER_SYS_TICK_CH;

	spin_lock(&time_lock);
	/* reset time */
	lastdec = timer_read(base, ch);  	/* capure current decrementer value time */
	timestamp = 0;	       		/* start "advancing" time stamp from 0 */
	spin_unlock(&time_lock);
}

unsigned long get_timer_masked(void)
{
	void __iomem *base = (void __iomem *)IO_ADDRESS(PHY_BASEADDR_TIMER);
	int ch = CFG_TIMER_SYS_TICK_CH;

	unsigned long now = timer_read(base, ch);		/* current tick value */

	if (now >= lastdec) {			/* normal mode (non roll) */
		timestamp += now - lastdec; /* move stamp fordward with absoulte diff ticks */
	} else {						/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += now + TIMER_COUNT - lastdec;
	}
	/* save last */
	lastdec = now;

	pr_debug("now=%lu, last=%lu, timestamp=%lu\n", now, lastdec, timestamp);
	return (unsigned long)timestamp;
}

void __udelay(unsigned long usec)
{
	unsigned long tmo, tmp;
	pr_debug("+udelay=%ld\n", usec);

	if (!timerinit)
		timer_init();

	if (usec >= 1000) {			// if "big" number, spread normalization to seconds //
		tmo  = usec / 1000;		// start to normalize for usec to ticks per sec //
		tmo *= TIMER_FREQ;		// find number of "ticks" to wait to achieve target //
		tmo /= 1000;			// finish normalize. //
	} else {						// else small number, don't kill it prior to HZ multiply //
		tmo = usec * TIMER_FREQ;
		tmo /= (1000*1000);
	}

	tmp = get_timer_masked ();			// get current timestamp //
	pr_debug("A. tmo=%ld, tmp=%ld\n", tmo, tmp);

	if ( tmp > (tmo + tmp + 1) )	// if setting this fordward will roll time stamp //
		reset_timer_masked();		// reset "advancing" timestamp to 0, set lastdec value //
	else
		tmo += tmp;					// else, set advancing stamp wake up time //

	pr_debug("B. tmo=%ld, tmp=%ld\n", tmo, tmp);

	while (tmo > get_timer_masked ())// loop till event //
	{
		//*NOP*/;
	}

	pr_debug("-udelay=%ld\n", usec);
	return;
}

void udelay_masked(unsigned long usec)
{
	unsigned long tmo, endtime;
	signed long diff;

	if (usec >= 1000) {		/* if "big" number, spread normalization to seconds */
		tmo = usec / 1000;	/* start to normalize for usec to ticks per sec */
		tmo *= TIMER_FREQ;		/* find number of "ticks" to wait to achieve target */
		tmo /= 1000;		/* finish normalize. */
	} else {			/* else small number, don't kill it prior to HZ multiply */
		tmo = usec * TIMER_FREQ;
		tmo /= (1000*1000);
	}

	endtime = get_timer_masked() + tmo;

	do {
		unsigned long now = get_timer_masked();
		diff = endtime - now;
	} while (diff >= 0);
}

unsigned long long get_ticks(void)
{
	return get_timer_masked();
}

unsigned long get_tbclk(void)
{
	unsigned long  tbclk = TIMER_FREQ;
	return tbclk;
}

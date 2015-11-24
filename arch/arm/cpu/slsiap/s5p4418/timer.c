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
#define DBGOUT(msg...)		do { printf("timer: " msg); } while (0)
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#if (CFG_TIMER_SYS_TICK_CH > 3)
#error Not support timer channel "0~3"
#endif

/* global variables to save timer count */
static ulong timestamp;
static ulong lastdec;
/* set .data section, before u-boot is relocated */
static int	 timerinit __attribute__ ((section(".data"))) = 0;
static DEFINE_SPINLOCK(time_lock);

/* macro to hw timer tick config */
static long	TIMER_FREQ  = 1000000;
static long	TIMER_HZ    = 1000000 / CONFIG_SYS_HZ;
static long	TIMER_COUNT = -1UL;

#define	TIMER_CFG0		(0x00)
#define	TIMER_CFG1		(0x04)
#define	TIMER_TCON		(0x08)
#define	TIMER_CNTB		(0x0C)
#define	TIMER_CMPB		(0x10)
#define	TIMER_CNTO		(0x14)
#define	TIMER_STAT		(0x44)

#define	TCON_AUTO		(1<<3)
#define	TCON_INVT		(1<<2)
#define	TCON_UP			(1<<1)
#define	TCON_RUN		(1<<0)
#define CFG0_CH(ch)		(ch == 0 || ch == 1 ? 0 : 8)
#define CFG1_CH(ch)		(ch * 4)
#define TCON_CH(ch)		(ch ? ch * 4  + 4 : 0)
#define TINT_CH(ch)		(ch)
#define TINT_CS_CH(ch)	(ch + 5)
#define	TINT_CS_MASK	(0x1F)
#define TIMER_CH_OFFS	(0xC)

/*
 * Timer HW
 */
#define	TIMER_BASE		IO_ADDRESS(PHY_BASEADDR_TIMER)
#define	TIMER_READ(ch)	(TIMER_COUNT - readl(TIMER_BASE + TIMER_CNTO + (TIMER_CH_OFFS * ch)))

static inline void timer_clock(int ch, int mux, int scl)
{
	volatile U32 val;

	val  = readl(TIMER_BASE + TIMER_CFG0);
	val &= ~(0xFF   << CFG0_CH(ch));
	val |=  ((scl-1)<< CFG0_CH(ch));
	writel(val, TIMER_BASE + TIMER_CFG0);

	val  = readl(TIMER_BASE + TIMER_CFG1);
	val &= ~(0xF << CFG1_CH(ch));
	val |=  (mux << CFG1_CH(ch));
	writel(val, TIMER_BASE + TIMER_CFG1);
}

static inline void timer_count(int ch, unsigned int cnt)
{
	writel((cnt-1), TIMER_BASE + TIMER_CNTB + (TIMER_CH_OFFS * ch));
	writel((cnt-1), TIMER_BASE + TIMER_CMPB + (TIMER_CH_OFFS * ch));
}

static inline void timer_irq_clear(int ch)
{
	volatile U32 val;
	val  = readl(TIMER_BASE + TIMER_STAT);
	val &= ~(TINT_CS_MASK<<5);
	val |= (0x1 << TINT_CS_CH(ch));
	writel(val, TIMER_BASE + TIMER_STAT);
}

/*------------------------------------------------------------------------------
 * u-boot timer interface
 */
void timer_start(int ch)
{
	volatile U32 val;
	int on = 0;

	val  = readl(TIMER_BASE + TIMER_STAT);
	val &= ~(TINT_CS_MASK<<5 | 0x1 << TINT_CH(ch));
	val |=  (0x1 << TINT_CS_CH(ch) | on << TINT_CH(ch));
	writel(val, TIMER_BASE + TIMER_STAT);

	val = readl(TIMER_BASE + TIMER_TCON);
	val &= ~(0xE << TCON_CH(ch));
	val |=  (TCON_UP << TCON_CH(ch));
	writel(val, TIMER_BASE + TIMER_TCON);

	val &= ~(TCON_UP << TCON_CH(ch));
	val |=  ((TCON_AUTO | TCON_RUN)  << TCON_CH(ch));
	writel(val, TIMER_BASE + TIMER_TCON);
}

void timer_stop(int ch)
{
	volatile U32 val;
	int on = 0;

	val  = readl(TIMER_BASE + TIMER_STAT);
	val &= ~(TINT_CS_MASK<<5 | 0x1 << TINT_CH(ch));
	val |=  (0x1 << TINT_CS_CH(ch) | on << TINT_CH(ch));
	writel(val, TIMER_BASE + TIMER_STAT);

	val  = readl(TIMER_BASE + TIMER_TCON);
	val &= ~(TCON_RUN << TCON_CH(ch));
	writel(val, TIMER_BASE + TIMER_TCON);
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
	tcnt = TIMER_COUNT == -1UL ? TIMER_COUNT+1 : tcnt;

	/* reset control: Low active ___|---   */
	NX_RSTCON_SetnRST(RESETINDEX_OF_TIMER_MODULE_PRESETn, RSTCON_nDISABLE);
	NX_RSTCON_SetnRST(RESETINDEX_OF_TIMER_MODULE_PRESETn, RSTCON_nENABLE);

	timer_stop (ch);
	timer_clock(ch, tmux, tscl);
	timer_count(ch, tcnt);
	timer_start(ch);

	reset_timer_masked();
	timerinit = 1;

	return 0;
}

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer(ulong base)
{
	ulong time = get_timer_masked();
	ulong hz = TIMER_HZ;
	return (time/hz - base);
}

void set_timer(ulong t)
{
	timestamp = (ulong)t;
}

void reset_timer_masked(void)
{
	int ch = CFG_TIMER_SYS_TICK_CH;

	spin_lock(&time_lock);
	/* reset time */
	lastdec = TIMER_READ(ch);  	/* capure current decrementer value time */
	timestamp = 0;	       		/* start "advancing" time stamp from 0 */
	spin_unlock(&time_lock);
}

ulong get_timer_masked(void)
{
	int ch = CFG_TIMER_SYS_TICK_CH;
	ulong now = TIMER_READ(ch);		/* current tick value */

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

	DBGOUT("now=%ld, last=%ld, timestamp=%ld\n", now, lastdec, timestamp);
	return (ulong)timestamp;
}

void __udelay(unsigned long usec)
{
	ulong tmo, tmp;
	DBGOUT("+udelay=%ld\n", usec);

	if (! timerinit)
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
	DBGOUT("A. tmo=%ld, tmp=%ld\n", tmo, tmp);

	if ( tmp > (tmo + tmp + 1) )	// if setting this fordward will roll time stamp //
		reset_timer_masked();		// reset "advancing" timestamp to 0, set lastdec value //
	else
		tmo += tmp;					// else, set advancing stamp wake up time //

	DBGOUT("B. tmo=%ld, tmp=%ld\n", tmo, tmp);

	while (tmo > get_timer_masked ())// loop till event //
	{
		//*NOP*/;
	}

	DBGOUT("-udelay=%ld\n", usec);
	return;
}

void udelay_masked(unsigned long usec)
{
	ulong tmo, endtime;
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
		ulong now = get_timer_masked();
		diff = endtime - now;
	} while (diff >= 0);
}

unsigned long long get_ticks(void)
{
	return get_timer_masked();
}

ulong get_tbclk(void)
{
	ulong  tbclk = TIMER_FREQ;
	return tbclk;
}

/*
 * STMP SYDMA Register Definitions
 *
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * This file is created by xml file. Don't Edit it.
 */

#ifndef __ARCH_ARM___SYDMA_H
#define __ARCH_ARM___SYDMA_H  1

#define REGS_SYDMA_BASE (STMP3XXX_REGS_BASE + 0x26000)
#define REGS_SYDMA_PHYS (0x80026000)
#define REGS_SYDMA_SIZE 0x00002000

#define HW_SYDMA_CTRL	(0x00000000)
#define HW_SYDMA_CTRL_SET	(0x00000004)
#define HW_SYDMA_CTRL_CLR	(0x00000008)
#define HW_SYDMA_CTRL_TOG	(0x0000000c)
#define HW_SYDMA_CTRL_ADDR  \
		(REGS_SYDMA_BASE + HW_SYDMA_CTRL)
#define HW_SYDMA_CTRL_SET_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_CTRL_SET)
#define HW_SYDMA_CTRL_CLR_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_CTRL_CLR)
#define HW_SYDMA_CTRL_TOG_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_CTRL_TOG)

#define BM_SYDMA_CTRL_SFTRST	0x80000000
#define BV_SYDMA_CTRL_SFTRST__RUN   0x0
#define BV_SYDMA_CTRL_SFTRST__RESET 0x1
#define BM_SYDMA_CTRL_CLKGATE	0x40000000
#define BV_SYDMA_CTRL_CLKGATE__RUN     0x0
#define BV_SYDMA_CTRL_CLKGATE__NO_CLKS 0x1
#define BP_SYDMA_CTRL_RSVD1	10
#define BM_SYDMA_CTRL_RSVD1	0x3FFFFC00
#define BF_SYDMA_CTRL_RSVD1(v)  \
		(((v) << 10) & BM_SYDMA_CTRL_RSVD1)
#define BM_SYDMA_CTRL_COMPLETE_IRQ_EN	0x00000200
#define BV_SYDMA_CTRL_COMPLETE_IRQ_EN__DISABLED 0x0
#define BV_SYDMA_CTRL_COMPLETE_IRQ_EN__ENABLED  0x1
#define BP_SYDMA_CTRL_RSVD0	3
#define BM_SYDMA_CTRL_RSVD0	0x000001F8
#define BF_SYDMA_CTRL_RSVD0(v)  \
		(((v) << 3) & BM_SYDMA_CTRL_RSVD0)
#define BM_SYDMA_CTRL_ERROR_IRQ	0x00000004
#define BM_SYDMA_CTRL_COMPLETE_IRQ	0x00000002
#define BM_SYDMA_CTRL_RUN	0x00000001
#define BV_SYDMA_CTRL_RUN__HALT 0x0
#define BV_SYDMA_CTRL_RUN__RUN  0x1

#define HW_SYDMA_RADDR	(0x00000010)
#define HW_SYDMA_RADDR_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_RADDR)

#define BP_SYDMA_RADDR_RSRC_ADDR	0
#define BM_SYDMA_RADDR_RSRC_ADDR	0xFFFFFFFF
#define BF_SYDMA_RADDR_RSRC_ADDR(v)	(v)

#define HW_SYDMA_WADDR	(0x00000020)
#define HW_SYDMA_WADDR_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_WADDR)

#define BP_SYDMA_WADDR_WSRC_ADDR	0
#define BM_SYDMA_WADDR_WSRC_ADDR	0xFFFFFFFF
#define BF_SYDMA_WADDR_WSRC_ADDR(v)	(v)

#define HW_SYDMA_XFER_COUNT	(0x00000030)
#define HW_SYDMA_XFER_COUNT_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_XFER_COUNT)

#define BP_SYDMA_XFER_COUNT_SIZE	0
#define BM_SYDMA_XFER_COUNT_SIZE	0xFFFFFFFF
#define BF_SYDMA_XFER_COUNT_SIZE(v)	(v)

#define HW_SYDMA_BURST	(0x00000040)
#define HW_SYDMA_BURST_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_BURST)

#define BP_SYDMA_BURST_RSVD0	4
#define BM_SYDMA_BURST_RSVD0	0xFFFFFFF0
#define BF_SYDMA_BURST_RSVD0(v) \
		(((v) << 4) & BM_SYDMA_BURST_RSVD0)
#define BP_SYDMA_BURST_WLEN	2
#define BM_SYDMA_BURST_WLEN	0x0000000C
#define BF_SYDMA_BURST_WLEN(v)  \
		(((v) << 2) & BM_SYDMA_BURST_WLEN)
#define BV_SYDMA_BURST_WLEN__1 0x0
#define BV_SYDMA_BURST_WLEN__2 0x1
#define BV_SYDMA_BURST_WLEN__4 0x2
#define BV_SYDMA_BURST_WLEN__8 0x3
#define BP_SYDMA_BURST_RLEN	0
#define BM_SYDMA_BURST_RLEN	0x00000003
#define BF_SYDMA_BURST_RLEN(v)  \
		(((v) << 0) & BM_SYDMA_BURST_RLEN)
#define BV_SYDMA_BURST_RLEN__1 0x0
#define BV_SYDMA_BURST_RLEN__2 0x1
#define BV_SYDMA_BURST_RLEN__4 0x2
#define BV_SYDMA_BURST_RLEN__8 0x3

#define HW_SYDMA_DACK	(0x00000050)
#define HW_SYDMA_DACK_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_DACK)

#define BP_SYDMA_DACK_RSVD0	8
#define BM_SYDMA_DACK_RSVD0	0xFFFFFF00
#define BF_SYDMA_DACK_RSVD0(v) \
		(((v) << 8) & BM_SYDMA_DACK_RSVD0)
#define BP_SYDMA_DACK_WDELAY	4
#define BM_SYDMA_DACK_WDELAY	0x000000F0
#define BF_SYDMA_DACK_WDELAY(v)  \
		(((v) << 4) & BM_SYDMA_DACK_WDELAY)
#define BP_SYDMA_DACK_RDELAY	0
#define BM_SYDMA_DACK_RDELAY	0x0000000F
#define BF_SYDMA_DACK_RDELAY(v)  \
		(((v) << 0) & BM_SYDMA_DACK_RDELAY)

#define HW_SYDMA_DEBUG0	(0x00000100)
#define HW_SYDMA_DEBUG0_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_DEBUG0)

#define BP_SYDMA_DEBUG0_DATA	0
#define BM_SYDMA_DEBUG0_DATA	0xFFFFFFFF
#define BF_SYDMA_DEBUG0_DATA(v)	(v)

#define HW_SYDMA_DEBUG1	(0x00000110)
#define HW_SYDMA_DEBUG1_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_DEBUG1)

#define BP_SYDMA_DEBUG1_DATA	0
#define BM_SYDMA_DEBUG1_DATA	0xFFFFFFFF
#define BF_SYDMA_DEBUG1_DATA(v)	(v)

#define HW_SYDMA_DEBUG2	(0x00000120)
#define HW_SYDMA_DEBUG2_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_DEBUG2)

#define BP_SYDMA_DEBUG2_DATA	0
#define BM_SYDMA_DEBUG2_DATA	0xFFFFFFFF
#define BF_SYDMA_DEBUG2_DATA(v)	(v)

#define HW_SYDMA_VERSION	(0x00000130)
#define HW_SYDMA_VERSION_ADDR \
		(REGS_SYDMA_BASE + HW_SYDMA_VERSION)

#define BP_SYDMA_VERSION_MAJOR	24
#define BM_SYDMA_VERSION_MAJOR	0xFF000000
#define BF_SYDMA_VERSION_MAJOR(v) \
		(((v) << 24) & BM_SYDMA_VERSION_MAJOR)
#define BP_SYDMA_VERSION_MINOR	16
#define BM_SYDMA_VERSION_MINOR	0x00FF0000
#define BF_SYDMA_VERSION_MINOR(v)  \
		(((v) << 16) & BM_SYDMA_VERSION_MINOR)
#define BP_SYDMA_VERSION_STEP	0
#define BM_SYDMA_VERSION_STEP	0x0000FFFF
#define BF_SYDMA_VERSION_STEP(v)  \
		(((v) << 0) & BM_SYDMA_VERSION_STEP)
#endif /* __ARCH_ARM___SYDMA_H */
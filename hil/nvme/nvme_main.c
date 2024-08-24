//////////////////////////////////////////////////////////////////////////////////
// nvme_main.c for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//				  Kibin Park <kbpark@enc.hanyang.ac.kr>
//
// This file is part of Cosmos+ OpenSSD.
//
// Cosmos+ OpenSSD is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// Cosmos+ OpenSSD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cosmos+ OpenSSD; see the file COPYING.
// If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Company: ENC Lab. <http://enc.hanyang.ac.kr>
// Engineer: Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//			 Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//			 Kibin Park <kbpark@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: NVMe Main
// File Name: nvme_main.c
//
// Version: v1.2.0
//
// Description:
//   - initializes FTL and NAND
//   - handles NVMe controller
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.2.0
//   - header file for buffer is changed from "ia_lru_buffer.h" to "lru_buffer.h"
//   - Low level scheduler execution is allowed when there is no i/o command
//
// * v1.1.0
//   - DMA status initialization is added
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include "debug.h"
#include "io_access.h"

#include "nvme.h"
#include "host_lld.h"
#include "nvme_main.h"
#include "nvme_admin_cmd.h"
#include "nvme_io_cmd.h"

#ifdef GREEDY_FTL
	#include "memory_map.h"
#endif

#ifdef WIN32
	#include "bsp_windows.hl"
#endif

#include "../../test/test_main.h"

#include "nvme_main.h"

#include "cosmos_plus_system.h"
#include "hil.h"
#include "../fil/fil.h"

volatile NVME_CONTEXT g_nvmeTask;

extern void test_main();

int cmt_miss1 = 0;
int cmt_miss2 = 0;
int cmt_hit = 0;
int gc_cnt = 0;
int meta_cnt = 0;

void nvme_main()
{
	xil_printf("!!! Wait until FTL reset complete !!! \r\n");

#ifdef GREEDY_FTL
	InitFTL();
#else
	HIL_Initialize();
	HIL_Format();
#endif

	xil_printf("\r\nFTL reset complete!!! \r\n");
	xil_printf("Turn on the host PC \r\n");

#if defined(WIN32) || (NVME_UNIT_TEST == 1)
	test_main();

	return;	// stop at here
#endif

	while(1)
	{
		nvme_run();
	}
}

static int bInitProfileCount = 0;
static int bPrintProfileCount = 0;

void nvme_run()
{
	unsigned int exeLlr;			// dy, execute low level request

	exeLlr = 1;

	if (g_nvmeTask.status == NVME_TASK_WAIT_CC_EN)
	{
		unsigned int ccEn;
		ccEn = check_nvme_cc_en();
		if (ccEn == 1)
		{
			set_nvme_admin_queue(1, 1, 1);
			set_nvme_csts_rdy(1);
			g_nvmeTask.status = NVME_TASK_RUNNING;
			xil_printf("\r\nNVMe ready!!!\r\n");
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_RUNNING)
	{
		NVME_COMMAND nvmeCmd;
		unsigned int cmdValid;

		cmdValid = get_nvme_cmd(&nvmeCmd.qID, &nvmeCmd.cmdSlotTag, &nvmeCmd.cmdSeqNum, nvmeCmd.cmdDword);

		if (cmdValid == 1)
		{
			if (nvmeCmd.qID == 0)
			{
				handle_nvme_admin_cmd(&nvmeCmd);
			}
			else
			{
				handle_nvme_io_cmd(&nvmeCmd);
#ifdef GREEDY_FTL
				ReqTransSliceToLowLevel();
#endif
				exeLlr = 0;
			}
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_SHUTDOWN)
	{
		xil_printf("cmt_hit = %d, cmt_miss = %d, %d\r\n", cmt_hit, cmt_miss1, cmt_miss2);
		xil_printf("data gc = %d, meta gc = %d\r\n", gc_cnt, meta_cnt);
		NVME_STATUS_REG nvmeReg;
		nvmeReg.dword = IO_READ32(NVME_STATUS_REG_ADDR);
		if (nvmeReg.ccShn != 0)
		{
			unsigned int qID;
			set_nvme_csts_shst(1);

			for (qID = 0; qID < 8; qID++)
			{
				set_io_cq(qID, 0, 0, 0, 0, 0, 0);
				set_io_sq(qID, 0, 0, 0, 0, 0);
			}

			set_nvme_admin_queue(0, 0, 0);
			g_nvmeTask.cacheEn = 0;
			set_nvme_csts_shst(2);
			g_nvmeTask.status = NVME_TASK_WAIT_RESET;

#ifdef  GREEDY_FTL
			//flush grown bad block info
			UpdateBadBlockTableForGrownBadBlock((unsigned int)RESERVED_DATA_BUFFER_BASE_ADDR);
#endif
			xil_printf("\r\nNVMe shutdown!!!\r\n");
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_WAIT_RESET)
	{
		unsigned int ccEn;
		ccEn = check_nvme_cc_en();
		if (ccEn == 0)
		{
			g_nvmeTask.cacheEn = 0;
			set_nvme_csts_shst(0);
			set_nvme_csts_rdy(0);
			g_nvmeTask.status = NVME_TASK_IDLE;
			xil_printf("\r\nNVMe disable!!!\r\n");
		}
	}
	else if (g_nvmeTask.status == NVME_TASK_RESET)
	{
		unsigned int qID;
		for (qID = 0; qID < 8; qID++)
		{
			set_io_cq(qID, 0, 0, 0, 0, 0, 0);
			set_io_sq(qID, 0, 0, 0, 0, 0);
		}
		g_nvmeTask.cacheEn = 0;
		set_nvme_admin_queue(0, 0, 0);
		set_nvme_csts_shst(0);
		set_nvme_csts_rdy(0);
		g_nvmeTask.status = NVME_TASK_IDLE;

		xil_printf("\r\nNVMe reset!!!\r\n");
	}

#ifdef GREEDY_FTL
	if (exeLlr && ((nvmeDmaReqQ.headReq != REQ_SLOT_TAG_NONE) || notCompletedNandReqCnt || blockedReqCnt))
	{
		CheckDoneNvmeDmaReq();		// check HDMA done and then move to freeReqQ
		SchedulingNandReq();
	}

	#ifdef WIN32
	SYSTEM_Run(0);		// for HDMA
	#endif

	if (bInitProfileCount == 1)
	{
		STAT_Initialize();
		bInitProfileCount = 0;
	}

	if (bPrintProfileCount == 1)
	{
		STAT_Print();
		bPrintProfileCount = 0;
	}

#else

	HIL_Run();		// dy, run host side FW & HW

#endif
}



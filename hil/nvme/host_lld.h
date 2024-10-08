//////////////////////////////////////////////////////////////////////////////////
// host_lld.h for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
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
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: NVMe Low Level Driver
// File Name: host_lld.h
//
// Version: v1.1.0
//
// Description:
//   - defines parameters and data structures of the NVMe low level driver
//   - declares functions of the NVMe low level driver
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.1.0
//   - new DMA status type is added (HOST_DMA_ASSIST_STATUS)
//	 - DMA partial done check functions are added
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#ifndef __HOST_LLD_H_
#define __HOST_LLD_H_

#include "cosmos_plus_global_config.h"

#if defined(WIN32) || (NVME_UNIT_TEST == 1)
	extern char*	XPAR_NVMEHOSTCONTROLLER_0_BASEADDR_EMUL;
	#define HOST_IP_ADDR						(XPAR_NVMEHOSTCONTROLLER_0_BASEADDR_EMUL)
#else
	#define HOST_IP_ADDR						(XPAR_NVMEHOSTCONTROLLER_0_BASEADDR)
#endif

#define DEV_IRQ_MASK_REG_ADDR				(HOST_IP_ADDR + 0x4)			//dy, interrupt mask, layout DEV_IRQ_REG
#define DEV_IRQ_CLEAR_REG_ADDR				(HOST_IP_ADDR + 0x8)
#define DEV_IRQ_STATUS_REG_ADDR				(HOST_IP_ADDR + 0xC)			// dy, DEV_IRQ_REG

#define PCIE_STATUS_REG_ADDR				(HOST_IP_ADDR + 0x100)
#define PCIE_FUNC_REG_ADDR					(HOST_IP_ADDR + 0x104)

#define NVME_STATUS_REG_ADDR				(HOST_IP_ADDR + 0x200)			// dy, NVME_STATUS_REG
#define HOST_DMA_FIFO_CNT_REG_ADDR			(HOST_IP_ADDR + 0x204)
#define NVME_ADMIN_QUEUE_SET_REG_ADDR		(HOST_IP_ADDR + 0x21C)
#define NVME_IO_SQ_SET_REG_ADDR				(HOST_IP_ADDR + 0x220)
#define NVME_IO_CQ_SET_REG_ADDR				(HOST_IP_ADDR + 0x260)

#define NVME_CMD_FIFO_REG_ADDR				(HOST_IP_ADDR + 0x300)
#define NVME_CPL_FIFO_REG_ADDR				(HOST_IP_ADDR + 0x304)
#define HOST_DMA_CMD_FIFO_REG_ADDR			(HOST_IP_ADDR + 0x310)

#define NVME_CMD_SRAM_ADDR					(HOST_IP_ADDR + 0x2000)

#define HOST_DMA_DIRECT_TYPE				(1)
#define HOST_DMA_AUTO_TYPE					(0)

#define HOST_DMA_TX_DIRECTION				(1)
#define HOST_DMA_RX_DIRECTION				(0)

#define ONLY_CPL_TYPE						(0)
#define AUTO_CPL_TYPE						(1)
#define CMD_SLOT_RELEASE_TYPE				(2)

#pragma pack(push, 1)

typedef struct _DEV_IRQ_REG
{
	union {
		unsigned int dword;
		struct {
			unsigned int pcieLink			:1;
			unsigned int busMaster			:1;
			unsigned int pcieIrq			:1;
			unsigned int pcieMsi			:1;
			unsigned int pcieMsix			:1;
			unsigned int nvmeCcEn			:1;
			unsigned int nvmeCcShn			:1;
			unsigned int mAxiWriteErr		:1;
			unsigned int mAxiReadErr		:1;
			unsigned int pcieMreqErr		:1;
			unsigned int pcieCpldErr		:1;
			unsigned int pcieCpldLenErr		:1;
			unsigned int reserved0			:20;
		};
	};
} DEV_IRQ_REG;

typedef struct _PCIE_STATUS_REG
{
	union {
		unsigned int dword;
		struct {
			unsigned int ltssm				:6;
			unsigned int reserved0			:2;
			unsigned int pcieLinkUp			:1;
			unsigned int reserved1			:23;
		};
	};
} PCIE_STATUS_REG;

typedef struct _PCIE_FUNC_REG
{
	union {
		unsigned int dword;
		struct {
			unsigned int busMaster			:1;
			unsigned int msiEnable			:1;
			unsigned int msixEnable			:1;
			unsigned int irqDisable			:1;
			unsigned int msiVecNum			:3;
			unsigned int reserved0			:25;
		};
	};
} PCIE_FUNC_REG;

//offset: 0x00000200, size: 4
typedef struct _NVME_STATUS_REG
{
	union {
		unsigned int dword;
		struct {
			unsigned int ccEn				:1;
			unsigned int ccShn				:2;
			unsigned int reserved0			:1;
			unsigned int cstsRdy			:1;
			unsigned int cstsShst			:2;
			unsigned int reserved1			:25;
		};
	};
} NVME_STATUS_REG;

//offset: 0x00000300, size: 4
typedef struct _NVME_CMD_FIFO_REG
{
	union {
		unsigned int dword;
		struct {
			unsigned int qID				:4;		//dy, Command QID, 0:admin, 1: IO
			unsigned int reserved0			:4;
			unsigned int cmdSlotTag			:7;		//dy, index of command slot, base NVME_CMD_SRAM_ADDR, 64byte each
			unsigned int reserved2			:1;
			unsigned int cmdSeqNum			:8;
			unsigned int reserved3			:7;
			unsigned int cmdValid			:1;
		};
	};
} NVME_CMD_FIFO_REG;

//offset: 0x00000304, size: 8
typedef struct _NVME_CPL_FIFO_REG
{
	union {
		unsigned int dword[3];
		struct {
			struct 
			{
				unsigned int cid				:16;
				unsigned int sqId				:4;
				unsigned int reserved0			:12;
			};

			unsigned int specific;

			unsigned short cmdSlotTag			:7;
			unsigned short reserved1			:7;
			unsigned short cplType				:2;

			union {
				unsigned short statusFieldWord;
				struct 
				{
					unsigned short reserved0	:1;
					unsigned short SC			:8;
					unsigned short SCT			:3;
					unsigned short reserved1	:2;
					unsigned short MORE			:1;
					unsigned short DNR			:1;
				}statusField;
			};
		};
	};
} NVME_CPL_FIFO_REG;

//offset: 0x0000021C, size: 4
typedef struct _NVME_ADMIN_QUEUE_SET_REG
{
	union {
		unsigned int dword;
		struct {
			unsigned int cqValid			:1;
			unsigned int sqValid			:1;
			unsigned int cqIrqEn			:1;
			unsigned int reserved0			:29;
		};
	};
} NVME_ADMIN_QUEUE_SET_REG;

//offset: 0x00000220, size: 8
typedef struct _NVME_IO_SQ_SET_REG
{
	union {
		unsigned int dword[2];
		struct {
			unsigned int pcieBaseAddrL;
			unsigned int pcieBaseAddrH		:4;
			unsigned int reserved0			:11;
			unsigned int valid				:1;
			unsigned int cqVector			:4;
			unsigned int reserved1			:4;
			unsigned int sqSize				:8;
		};
	};
} NVME_IO_SQ_SET_REG;


//offset: 0x00000260, size: 8
typedef struct _NVME_IO_CQ_SET_REG
{
	union {
		unsigned int dword[2];
		struct {
			unsigned int pcieBaseAddrL;
			unsigned int pcieBaseAddrH		:4;
			unsigned int reserved0			:11;
			unsigned int valid				:1;
			unsigned int irqVector			:3;
			unsigned int irqEn				:1;
			unsigned int reserved1			:4;
			unsigned int cqSize				:8;
		};
	};
} NVME_IO_CQ_SET_REG;

//offset: 0x00000204, size: 4
typedef struct _HOST_DMA_FIFO_CNT_REG
{
	union {
		unsigned int dword;
		struct 
		{
			unsigned char directDmaRx;
			unsigned char directDmaTx;
			unsigned char autoDmaRx;			// dy, DMA index for Rx, DMA done/request index, MAX 255, increment only
			unsigned char autoDmaTx;			// dy, DMA index for Tx, DMA done/request index, MAX 255, increment only
		};
	};
} HOST_DMA_FIFO_CNT_REG;

//offset: 0x0000030C, size: 16
typedef struct _HOST_DMA_CMD_FIFO_REG
{
	union {
		unsigned int dword[4];
		struct 
		{
			unsigned int devAddr;						// dy, Memory Address
			unsigned int pcieAddrH;						// dy, 설정하지 않음.
			unsigned int pcieAddrL;						// dy, 설정하지 않음.
			struct 
			{
				unsigned int dmaLen				:13;
				unsigned int autoCompletion		:1;		// dy, NVME_COMMAND_AUTO_COMPLETION_ON or OFF
				unsigned int cmd4KBOffset		:9;		// dy, 4KB offset in a request
				unsigned int cmdSlotTag			:7;
				unsigned int dmaDirection		:1;		// dy, HOST_DMA_TX_DIRECTION, HOST_DMA_RX_DIRECTION
				unsigned int dmaType			:1;		// dy, HOST_DMA_AUTO_TYPE or HOST_DMA_DIRECT_TYPE
			};
		};
	};
} HOST_DMA_CMD_FIFO_REG;

//offset: 0x00002000, size: 64 * 128
typedef struct _NVME_CMD_SRAM
{
	unsigned int dword[128][16];
} _NVME_CMD_SRAM;

#pragma pack(pop)


typedef struct _HOST_DMA_STATUS
{
	HOST_DMA_FIFO_CNT_REG fifoHead;		// dy, DMA IP SFR의 정보를 가져오기 위한 것. HW update
	HOST_DMA_FIFO_CNT_REG fifoTail;		// dy, Insert한 Issue한 DMA의 정보를 저장, tail에 insert하면서 저장하고 head와 비교하여 완료 되었는지 확인, FW update
	unsigned int directDmaTxCnt;
	unsigned int directDmaRxCnt;
	unsigned int autoDmaTxCnt;			// dy, Total auto DMA Tx Count, (cumulative)
	unsigned int autoDmaRxCnt;			// dy, Total auto DMA Rx Count, (cumulative)
} HOST_DMA_STATUS;


typedef struct _HOST_DMA_ASSIST_STATUS
{
	unsigned int autoDmaTxOverFlowCnt;
	unsigned int autoDmaRxOverFlowCnt;
} HOST_DMA_ASSIST_STATUS;

#ifdef __cplusplus
extern "C" {
#endif

	void dev_irq_init();

	void dev_irq_handler();

	unsigned int check_nvme_cc_en();

	void set_nvme_csts_rdy(unsigned int rdy);

	void set_nvme_csts_shst(unsigned int shst);

	void set_nvme_admin_queue(unsigned int sqValid, unsigned int cqValid, unsigned int cqIrqEn);

	unsigned int get_nvme_cmd(unsigned short *qID, unsigned short *cmdSlotTag, unsigned int *cmdSeqNum, unsigned int *cmdDword);

	void set_auto_nvme_cpl(unsigned int cmdSlotTag, unsigned int specific, unsigned int statusFieldWord);

	void set_nvme_slot_release(unsigned int cmdSlotTag);

	void set_nvme_cpl(unsigned int sqId, unsigned int cid, unsigned int specific, unsigned int statusFieldWord);

	void set_io_sq(unsigned int ioSqIdx, unsigned int valid, unsigned int cqVector, unsigned int qSzie, unsigned int pcieBaseAddrL, unsigned int pcieBaseAddrH);

	void set_io_cq(unsigned int ioCqIdx, unsigned int valid, unsigned int irqEn, unsigned int irqVector, unsigned int qSzie, unsigned int pcieBaseAddrL, unsigned int pcieBaseAddrH);

	void set_direct_tx_dma(unsigned int devAddr, unsigned int pcieAddrH, unsigned int pcieAddrL, unsigned int len);

	void set_direct_rx_dma(unsigned int devAddr, unsigned int pcieAddrH, unsigned int pcieAddrL, unsigned int len);

	void set_auto_tx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

	void set_auto_rx_dma(unsigned int cmdSlotTag, unsigned int cmd4KBOffset, unsigned int devAddr, unsigned int autoCompletion);

	void check_direct_tx_dma_done();

	void check_direct_rx_dma_done();

	void check_auto_tx_dma_done();

	void check_auto_rx_dma_done();

	unsigned int check_auto_tx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);

	unsigned int check_auto_rx_dma_partial_done(unsigned int tailIndex, unsigned int tailAssistIndex);

	extern HOST_DMA_STATUS g_hostDmaStatus;
	extern HOST_DMA_ASSIST_STATUS g_hostDmaAssistStatus;


#if defined(WIN32) || (NVME_UNIT_TEST == 1)
	void set_tx_dma_done(int bAll);
	void set_rx_dma_done(int bAll);
#endif

#ifdef __cplusplus
}
#endif

#endif	//__HOST_LLD_H_

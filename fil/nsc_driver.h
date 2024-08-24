//////////////////////////////////////////////////////////////////////////////////
// fmc_driver.h for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Kibin Park <kbpark@enc.hanyang.ac.kr>
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
// Engineer: Kibin Park <kbpark@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: NAND Storage Controller Driver
// File Name: nsc_driver.h
//
// Version: v1.2.0
//
// Description:
//   - define parameters, data structure and functions of NAND storage controller driver
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.2.0
//   - Completion flag checker is added
//   - Way ready checker is added
//   - Request status report checker is added
//
// * v1.1.0
//   - V2FReadPageTransferAsync needs additional input (rowAddress)
//   - Opcode of some commands is modified
//   - LLSCommand_ReadRawPage is deleted
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#ifndef FMC_DRIVER_H_
#define FMC_DRIVER_H_

#include "cosmos_plus_system.h"

#define V2FCommand_NOP 0
#define V2FCommand_Reset 1
#define V2FCommand_SetFeatures 6
#define V2FCommand_GetFeatures 46
#define V2FCommand_ReadPageTrigger 13
#define V2FCommand_ReadPageTransfer 18
#define V2FCommand_ProgramPage 28
#define V2FCommand_BlockErase 37
#define V2FCommand_StatusCheck 41
#define V2FCommand_ReadPageTransferRaw 55


#define V2FCrcValid(errorInformation) !!((errorInformation) & (0x10000000))
#define V2FSpareChunkValid(errorInformation) !!((errorInformation) & (0x01000000))
#define V2FPageChunkValid(errorInformation) ((errorInformation) == 0xffffffff)
#define V2FWorstChunkErrorCount(errorInformation) (((errorInformation) & 0x00ff0000) >> 16)

#define V2FEnterToggleMode(dev, way, payLoadAddr) V2FSetFeaturesSync(dev, way, 0x00000006, 0x00000008, 0x00000020, payLoadAddr)

#define V2FWayReady(readyBusy, wayNo) (((readyBusy) >> (wayNo)) & 1)		// dy, way ready bitmap, 1b: ready, 0b: busy
#define V2FTransferComplete(completeFlag) ((completeFlag) & 1)
#define V2FRequestReportDone(statusReport) ((statusReport) & 1)
#define V2FEliminateReportDoneFlag(statusReport) ((statusReport) >> 1)
#define V2FRequestComplete(statusReport) (((statusReport) & 0x60) == 0x60)
#define V2FRequestFail(statusReport) ((statusReport) & 3)

#ifdef WIN32
#define V2FRequestReportSetDone(pStatusReport)		((*pStatusReport) |= 1)
#define V2FRequestReportSetComplete(pStatusReport)	((*pStatusReport) |= (0x60 << 1))		// dy, V2FEliminateReportDoneFlag() 고려하여 1bit left shift 해줌
#define V2FClearErrorInfo(pErrorInformation)		((*pErrorInformation) = 0)
#define V2FCrcSetValid(pErrorInformation)			((*pErrorInformation) |= 0x10000000)
#define V2FSpareChunkSetValid(pErrorInformation)	((*pErrorInformation) |= 0x01000000)
#define V2FPageChunkSetValid(pErrorInformation)		((*pErrorInformation) = 0xFFFFFFFF)
#endif

typedef struct
{
	unsigned int cmdSelect;				// dy, NAND command, V2FCommand_ReadPageTrigger 참조.
	unsigned int rowAddress;			// dy, IO adress
	unsigned int userData;
	unsigned int dataAddress;
	unsigned int spareAddress;
	unsigned int errorCountAddress;		// dy, read error information, ERROR_INFO_FAIL, ERROR_INFO_PASS, ERROR_INFO_WARNING
	unsigned int completionAddress;
	unsigned int waySelection;			// dy, channel 내에서의 way
	unsigned int channelBusy;			// dy, 1: busy, 0: idle, HW가 설정하는 값이므로 Simulator에서는 항상 0인 상태이다.
	unsigned int readyBusy;				// dy, channel의 way 별 ready bitmap, 1b: ready, 0b: busy
} V2FMCRegisters;

unsigned int V2FIsControllerBusy(V2FMCRegisters* dev);
void V2FResetSync(V2FMCRegisters* dev, int way);
void V2FSetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int feature0x02, unsigned int feature0x10, unsigned int feature0x01, unsigned int payLoadAddr);
void V2FGetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int* feature0x01, unsigned int* feature0x02, unsigned int* feature0x10, unsigned int* feature0x30);
void V2FReadPageTriggerAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress);
void V2FReadPageTransferAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, void* spareDataBuffer, unsigned int* errorInformation, unsigned int* completion, unsigned int rowAddress);
void V2FReadPageTransferRawAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, unsigned int* completion);
void V2FProgramPageAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress, void* pageDataBuffer, void* spareDataBuffer);
void V2FEraseBlockAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress);
void V2FStatusCheckAsync(V2FMCRegisters* dev, int way, unsigned int* statusReport);
unsigned int V2FStatusCheckSync(V2FMCRegisters* dev, int way);
unsigned int V2FReadyBusyAsync(V2FMCRegisters* dev);

#endif /* FMC_DRIVER_H_ */

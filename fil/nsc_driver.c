//////////////////////////////////////////////////////////////////////////////////
// fmc_driver.c for Cosmos+ OpenSSD
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
// File Name: nsc_driver.c
//
// Version: v1.1.0
//
// Description:
//   - low level driver for NAND storage controller
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.1.0
//   - V2FReadPageTransferAsync needs additional input (rowAddress)
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include "cosmos_plus_system.h"
#include "nsc_driver.h"

#ifdef WIN32
void V2FControllerSetComplete(V2FMCRegisters* dev)
{
	if (dev->completionAddress)
	{
		V2FRequestReportSetDone((volatile unsigned int *)dev->completionAddress);
		V2FRequestReportSetComplete((volatile unsigned int *)dev->completionAddress);
	}
}

void V2FControllerSetNoError(V2FMCRegisters* dev)
{
	V2FClearErrorInfo((volatile unsigned int *)dev->errorCountAddress);
	V2FCrcSetValid((volatile unsigned int *)dev->errorCountAddress);
	V2FSpareChunkSetValid((volatile unsigned int *)dev->errorCountAddress);

	V2FClearErrorInfo(&((volatile unsigned int *)dev->errorCountAddress)[1]);
	V2FPageChunkSetValid(&((volatile unsigned int *)dev->errorCountAddress)[1]);
}

#endif

unsigned int 
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FIsControllerBusy(V2FMCRegisters* dev)
{
	volatile unsigned int channelBusy = *((volatile unsigned int*)&(dev->channelBusy));

	return channelBusy;
}

void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FResetSync(V2FMCRegisters* dev, int way)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_Reset;
	while (V2FIsControllerBusy(dev));
}

void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FSetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int feature0x02, unsigned int feature0x10, unsigned int feature0x01, unsigned int payLoadAddr)
{
	unsigned int* payload = (unsigned int*)payLoadAddr;
	payload[0] = feature0x02;
	payload[1] = feature0x10;
	payload[2] = feature0x01;
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->userData)) = (unsigned int)payload;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_SetFeatures;
	while (V2FIsControllerBusy(dev));
}

void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FGetFeaturesSync(V2FMCRegisters* dev, int way, unsigned int* feature0x01, unsigned int* feature0x02, unsigned int* feature0x10, unsigned int* feature0x30)
{
	volatile unsigned int buffer[4] = {0};
	volatile unsigned int completion = 0;
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->userData)) = (unsigned int)buffer;
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)&completion;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_GetFeatures;
	while (V2FIsControllerBusy(dev));
	while (!(completion & 1));
	*feature0x01 = buffer[0];
	*feature0x02 = buffer[1];
	*feature0x10 = buffer[2];
	*feature0x30 = buffer[3];
}

void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FReadPageTriggerAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ReadPageTrigger;
}

void
#ifndef WIN32
__attribute__((optimize("O0"))) 
#endif
V2FReadPageTransferAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, void* spareDataBuffer, unsigned int* errorInformation, unsigned int* completion, unsigned int rowAddress)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->dataAddress)) = (unsigned int)pageDataBuffer;
	*((volatile unsigned int*)&(dev->spareAddress)) = (unsigned int)spareDataBuffer;
	*((volatile unsigned int*)&(dev->errorCountAddress)) = (unsigned int)errorInformation;
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)completion;
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*completion = 0;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ReadPageTransfer;

#ifdef WIN32
	V2FControllerSetComplete(dev);
	V2FControllerSetNoError(dev);
#endif
}

void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FReadPageTransferRawAsync(V2FMCRegisters* dev, int way, void* pageDataBuffer, unsigned int* completion)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->dataAddress)) = (unsigned int)pageDataBuffer;
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)completion;
	*completion = 0;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ReadPageTransferRaw;

#ifdef WIN32
	V2FControllerSetComplete(dev);
#endif
}


void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FProgramPageAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress, void* pageDataBuffer, void* spareDataBuffer)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*((volatile unsigned int*)&(dev->dataAddress)) = (unsigned int)pageDataBuffer;
	*((volatile unsigned int*)&(dev->spareAddress)) = (unsigned int)spareDataBuffer;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_ProgramPage;

#ifdef WIN32
	V2FControllerSetComplete(dev);
#endif
}

void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FEraseBlockAsync(V2FMCRegisters* dev, int way, unsigned int rowAddress)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->rowAddress)) = rowAddress;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_BlockErase;
}

void
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FStatusCheckAsync(V2FMCRegisters* dev, int way, unsigned int* statusReport)
{
	*((volatile unsigned int*)&(dev->waySelection)) = way;
	*((volatile unsigned int*)&(dev->completionAddress)) = (unsigned int)statusReport;
	*statusReport = 0;
	*((volatile unsigned int*)&(dev->cmdSelect)) = V2FCommand_StatusCheck;

#ifdef WIN32
	V2FControllerSetComplete(dev);
#endif
}

unsigned int
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FStatusCheckSync(V2FMCRegisters* dev, int way)
{
	volatile unsigned int status;
	V2FStatusCheckAsync(dev, way, (unsigned int*)&status);
	while (!(status & 1));
	return (status >> 1);
}

unsigned int
#ifndef WIN32
__attribute__((optimize("O0")))
#endif
V2FReadyBusyAsync(V2FMCRegisters* dev)
{
	volatile unsigned int readyBusy = dev->readyBusy;

	return readyBusy;
}


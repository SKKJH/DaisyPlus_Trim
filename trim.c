#include "trim.h"
#include "data_buffer.h"
#include "request_allocation.h"
#include "address_translation.h"

void GetTrimData()
{
	trim_flag = 0;
	unsigned int tempAddr = trimDevAddr;
//	xil_printf("Get Trim Data\n");
//	xil_printf("num of ranges : %d\r\n",nr);
	for (int i=0; i<nr; i++)
	{
		unsigned int *ptr = (unsigned int*)tempAddr;
		//xil_printf("devAddr : %d\r\n", ptr);

		if (((*(ptr)<0)||(*(ptr)>0xfffffff0))||((*(ptr+1)<0)||(*(ptr+1)>0xfffffff0))||((*(ptr+2)< 0)||(*(ptr+2)>0xfffffff0))||((*(ptr+3)<0)||(*(ptr+3)>0xfffffff0)))
		{
//			xil_printf("ContextAttributess : %d\r\n",(*(ptr)));
//			xil_printf("lengthInLogicalBlocks : %d\r\n",(*(ptr+1)));
//			xil_printf("startingLBA[0] : %d\r\n",(*(ptr+2)));
//			xil_printf("startingLBA[1] : %d\r\n",(*(ptr+3)));

			dmRangePtr->dmRange[i].ContextAttributes.value = 0;
			dmRangePtr->dmRange[i].lengthInLogicalBlocks = 1;
			dmRangePtr->dmRange[i].startingLBA[0] = 0;
			dmRangePtr->dmRange[i].startingLBA[1] = 0;
		}
		else
		{
			dmRangePtr->dmRange[i].ContextAttributes.value = *(ptr);
			dmRangePtr->dmRange[i].lengthInLogicalBlocks = *(ptr+1);
			dmRangePtr->dmRange[i].startingLBA[0] = *(ptr + 2);
			dmRangePtr->dmRange[i].startingLBA[1] = *(ptr + 3);
		}
//		xil_printf("start lba : %d, length : %d\r\n",dmRangePtr->dmRange[i].startingLBA[0], dmRangePtr->dmRange[i].lengthInLogicalBlocks);
		tempAddr += sizeof(unsigned int)*4;
	}
}

void DoTrim()
{
	int nlb, startLba, tempLsa, nvmeBlockOffset, BLK0, BLK1, BLK2, BLK3, tempLength, tempsLba;

	for (int i=0; i<nr; i++)
	{
		nlb = dmRangePtr->dmRange[i].lengthInLogicalBlocks;
		startLba = dmRangePtr->dmRange[i].startingLBA[0];
		//xil_printf("nlb : %d, start lba : %d\r\n", nlb, startLba);
		nvmeBlockOffset = (startLba % NVME_BLOCKS_PER_SLICE);	
		tempLsa = startLba / NVME_BLOCKS_PER_SLICE;
		tempLength = nlb;
		tempsLba = startLba;
		BLK0 = 0;
		BLK1 = 0;
		BLK2 = 0;
		BLK3 = 0;	
		if (tempLength == 1)
		{
			if (nvmeBlockOffset == 0)
			{
				BLK0 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 1;
			}
			else if (nvmeBlockOffset == 1)
			{
				BLK1 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 1;
			}
			else if (nvmeBlockOffset == 2)
			{
				BLK2 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 1;
			}
			else
			{
				BLK3 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 1;
			}
		}
		else if (tempLength == 2)
		{
			if (nvmeBlockOffset == 0)
			{
				BLK0 = 1;
				BLK1 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 2;
			}
			else if (nvmeBlockOffset == 1)
			{
				BLK1 = 1;
				BLK2 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 2;
			}
			else if (nvmeBlockOffset == 2)
			{
				BLK2 = 1;
				BLK3 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 2;
			}
			else
			{
				BLK3 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempsLba += 1;
				tempLength -= 1;
			}
		}
		else if (tempLength == 3)
		{
			if (nvmeBlockOffset == 0)
			{
				BLK0 = 1;
				BLK1 = 1;
				BLK2 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 3;
			}
			else if (nvmeBlockOffset == 1)
			{
				BLK1 = 1;
				BLK2 = 1;
				BLK3 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempLength -= 3;
			}
			else if (nvmeBlockOffset == 2)
			{
				BLK2 = 1;
				BLK3 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempsLba += 2;
				tempLength -= 2;
			}
			else
			{
				BLK3 = 1;
				TRIM(tempLsa, BLK0, BLK1, BLK2, BLK3);
				tempsLba += 1;
				tempLength -= 1;
			}
		}
		while (tempLength >= 4)
		{
			tempLsa = tempsLba / NVME_BLOCKS_PER_SLICE;
			TRIM(tempLsa, 1, 1, 1, 1);
			tempsLba += 4;
			tempLength -= 4;
		}
		tempLsa = tempsLba / NVME_BLOCKS_PER_SLICE;
		if (tempLength == 3)
		{
			TRIM(tempLsa, 1, 1, 1, 0);
		}
		else if (tempLength == 2)
		{
			TRIM(tempLsa, 1, 1, 0, 0);
		}
		else if (tempLength == 1)
		{
			TRIM(tempLsa, 1, 0, 0, 0);
		}
	}
}


void TRIM(unsigned int LPN, unsigned int BLK0, unsigned int BLK1, unsigned int BLK2, unsigned int BLK3) {
    //xil_printf("LPN : %d, BIT : %d%d%d%d\n",LPN, BLK0, BLK1, BLK2, BLK3);

	XTime_GetTime(&bufStart);
    unsigned int bufEntry = CheckDataBufHitByLSA(LPN);
    if (bufEntry != DATA_BUF_FAIL) {
    	//xil_printf("Buffer Hit with LPN: %d\r\n!!",LPN);
        if (BLK0 == 1) {
            dataBufMapPtr->dataBuf[bufEntry].blk0 = 0;
        }
        if (BLK1 == 1) {
            dataBufMapPtr->dataBuf[bufEntry].blk1 = 0;
        }
        if (BLK2 == 1) {
            dataBufMapPtr->dataBuf[bufEntry].blk2 = 0;
        }
        if (BLK3 == 1) {
            dataBufMapPtr->dataBuf[bufEntry].blk3 = 0;
        }
        if ((dataBufMapPtr->dataBuf[bufEntry].blk0 == 0) && 
            (dataBufMapPtr->dataBuf[bufEntry].blk1 == 0) && 
            (dataBufMapPtr->dataBuf[bufEntry].blk2 == 0) && 
            (dataBufMapPtr->dataBuf[bufEntry].blk3 == 0)) 
        {
        	//xil_printf("This LSA will be Cleaned LSA: %d!!\r\n", dataBufMapPtr->dataBuf[bufEntry].logicalSliceAddr);
            unsigned int prevBufEntry, nextBufEntry;
            prevBufEntry = dataBufMapPtr->dataBuf[bufEntry].prevEntry;
            nextBufEntry = dataBufMapPtr->dataBuf[bufEntry].nextEntry;

            if (prevBufEntry != DATA_BUF_NONE && nextBufEntry != DATA_BUF_NONE) {
                dataBufMapPtr->dataBuf[prevBufEntry].nextEntry = nextBufEntry;
                dataBufMapPtr->dataBuf[nextBufEntry].prevEntry = prevBufEntry;
                nextBufEntry = DATA_BUF_NONE;
                prevBufEntry = dataBufLruList.tailEntry;
                dataBufMapPtr->dataBuf[dataBufLruList.tailEntry].nextEntry = bufEntry;
                dataBufLruList.tailEntry = bufEntry;
            } else if (prevBufEntry != DATA_BUF_NONE && nextBufEntry == DATA_BUF_NONE) {
                dataBufLruList.tailEntry = bufEntry;
            } else if (prevBufEntry == DATA_BUF_NONE && nextBufEntry != DATA_BUF_NONE) {
                dataBufMapPtr->dataBuf[nextBufEntry].prevEntry = DATA_BUF_NONE;
                dataBufLruList.headEntry = nextBufEntry;
                prevBufEntry = dataBufLruList.tailEntry;
                dataBufMapPtr->dataBuf[dataBufLruList.tailEntry].nextEntry = bufEntry;
                dataBufLruList.tailEntry = bufEntry;
            } else {
                prevBufEntry = DATA_BUF_NONE;
                nextBufEntry = DATA_BUF_NONE;
                dataBufLruList.headEntry = bufEntry;
                dataBufLruList.tailEntry = bufEntry;
            }
            SelectiveGetFromDataBufHashList(bufEntry);
            dataBufMapPtr->dataBuf[bufEntry].blockingReqTail = REQ_SLOT_TAG_NONE;
            dataBufMapPtr->dataBuf[bufEntry].dirty = DATA_BUF_CLEAN;
            dataBufMapPtr->dataBuf[bufEntry].reserved0 = 0;
        }
    }
	XTime_GetTime(&bufEnd);
	bufTime += bufEnd - bufStart;

	XTime_GetTime(&tableStart);
    unsigned int virtualSliceAddr = logicalSliceMapPtr->logicalSlice[LPN].virtualSliceAddr;
    if (virtualSliceAddr != VSA_NONE) {
        if (BLK0 == 1) {
            logicalSliceMapPtr->logicalSlice[LPN].blk0 = 0;
        }
        if (BLK1 == 1) {
            logicalSliceMapPtr->logicalSlice[LPN].blk1 = 0;
        }
        if (BLK2 == 1) {
            logicalSliceMapPtr->logicalSlice[LPN].blk2 = 0;
        }
        if (BLK3 == 1) {
            logicalSliceMapPtr->logicalSlice[LPN].blk3 = 0;
        }
        if ((logicalSliceMapPtr->logicalSlice[LPN].blk0 == 0) &&
            (logicalSliceMapPtr->logicalSlice[LPN].blk1 == 0) &&
            (logicalSliceMapPtr->logicalSlice[LPN].blk2 == 0) &&
            (logicalSliceMapPtr->logicalSlice[LPN].blk3 == 0)) 
        {
            InvalidateOldVsa(LPN);
        }
    }
	XTime_GetTime(&tableEnd);
	tableTime += tableEnd - tableStart;
}


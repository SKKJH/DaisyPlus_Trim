/*******************************************************
*
* Copyright (C) 2018-2019 
* Embedded Software Laboratory(ESLab), SUNG KYUN KWAN UNIVERSITY
* 
* This file is part of ESLab's Flash memory firmware
* 
* This source can not be copied and/or distributed without the express
* permission of ESLab
*
* Author: DongYoung Seo (dongyoung.seo@gmail.com)
* ESLab: http://nyx.skku.ac.kr
*
*******************************************************/

#ifndef __DFTL_CONFIG_H__
#define __DFTL_CONFIG_H__

#define ACTIVE_BLOCK_BUFFERING_COUNT		(USER_CHANNELS * USER_WAYS * 3)				// 2 is enough, must be a small value for nCurProgramBuffering
#define ACTIVE_BLOCK_BUFFERING_LPN_COUNT	(ACTIVE_BLOCK_BUFFERING_COUNT * LPN_PER_PHYSICAL_PAGE)

#define ACTIVE_BLOCK_COUNT_PER_STREAM		(2)			// current / old, MAX 256

#define SUPPORT_META_DEMAND_LOADING			(1)			// 0: Full Page Mapping
														// 1: DFTL (with meta demand loading and meta GC)

#ifdef FPM_FTL
	#undef SUPPORT_META_DEMAND_LOADING
	#define SUPPORT_META_DEMAND_LOADING		(0)
#endif

#define META_VPAGE_SIZE						LOGICAL_PAGE_SIZE		// Same of mapping unit, to share VNAND / ActiveBlock moudle
#define META_OP_RATIO						(1)						// Meta overprovisioning ratio, 1 means 100%
																	// it need enough OP to reduce meta GC
#define META_VBLOCK_COUNT_MIN				(4)						// Minimum meta VBlock
//#define META_CACHE_SIZE						(16 * MB)				// Byte unit
#define META_CACHE_SIZE						(4 * MB)				// Byte unit

#define META_GC_THRESHOLD					(1)				// meta data gc tirgger free blocks

#ifdef WIN32
	// debugging
	#define SUPPORT_GC_DEBUG		(1)
	#define SUPPORT_BLOCK_DEBUG		(1)
#endif

#endif		// end of #ifndef __DFTL_INIT_H__

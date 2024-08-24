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

#ifndef __HIL_CONFIG_H__
#define __HIL_CONFIG_H__

#include "cosmos_plus_global_config.h"

#define HOST_REQUEST_COUNT		(256)
#define HOST_REQUEST_BUF_COUNT	(HOST_REQUEST_COUNT * 8)

//#define STATIC_USER_DENSITY_MB	(256)			// set user density by force, set 0: disable

#define HIL_DEBUG		(0)

#if (HIL_DEBUG == 0)
	#define HIL_DEBUG_PRINTF(...)		((void)0)
#else
	#define HIL_DEBUG_PRINTF		PRINTF
#endif

#endif	// end of #ifndef __HIL_H__

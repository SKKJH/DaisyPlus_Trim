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

#ifndef __FIL_CONFIG_H__
#define __FIL_CONFIG_H__

#include "cosmos_plus_system.h"

#define FTL_REQUEST_COUNT		(USER_CHANNELS * USER_WAYS * 2)		// #CH * #wAY * double Buffering

#define FIL_READ_RETRY			(5)			// re-try count when UECC

// DEBUG CONFIG
#define NAND_IO_PRINT			(0)

#endif	// end of #ifdef __FIL_CONFIG_H__





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

#ifndef __DFTL_INTERNAL_H__
#define __DFTL_INTERNAL_H__

// only headers for dftl internal operation

#include <new>

// target header
#include "cosmos_types.h"
#include "cosmos_plus_system.h"

#include "statistics.h"

// hil headers
#include "hil_config.h"

// fil headers
#include "fil.h"

// common headers
#include "osal.h"
#include "util.h"
#include "debug.h"
#include "list.h"

// ftl headers
#include "ftl.h"

#include "dftl_config.h"
#include "dftl_types.h"
#include "dftl_hdma.h"
#include "dftl_vnand.h"
#include "dftl_meta.h"
#include "dftl_block.h"
#include "dftl_bufferpool.h"
#include "dftl_activeblock.h"
#include "dftl_garbagecollector.h"
#include "dftl_request_meta.h"
#include "dftl_request_gc.h"
#include "dftl_request_hil.h"
#include "dftl_request.h"
#include "dftl_global.h"

#endif		// end of #ifndef __DFTL_INTERNAL_H__

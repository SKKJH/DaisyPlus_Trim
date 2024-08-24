#ifndef __COSMOS_TYPES_H__
#define __COSMOS_TYPES_H__

#include "nvme.h"

#define KB				(1024)
#define MB				(1024 * KB)
#define GB				(1024 * MB)

typedef void				VOID;
typedef long long int		INT64;
typedef unsigned long long	UINT64;
typedef int					INT32;
typedef unsigned int		UINT32;
typedef short				INT16;
typedef unsigned short		UINT16;
typedef signed char			INT8;
typedef unsigned char		UINT8;
typedef INT8				BOOL;

#define INVALID_INDEX		0xFFFFFFFF

typedef enum
{
	TRACE_READ = 0,
	TRACE_WRITE,
	TRACE_EOF,
	TRACE_UNKNOWN,
} TRACE_TYPE;

typedef enum
{
	NVME_CMD_OPCODE_FLUASH = IO_NVM_FLUSH,
	NVME_CMD_OPCODE_READ = IO_NVM_READ,
	NVME_CMD_OPCODE_WRITE = IO_NVM_WRITE,
} NVME_CMD_OPCODE;

// request id from HIL to FTL
typedef struct
{
	unsigned int	nRequestIndex		: 16;		// HOST REQUEST INDEXS
	unsigned int	nRequestBufIndex	: 16;		// HOST BUFFER INDEX
} HIL_REQUEST_ID;

typedef enum
{
#ifdef STREAM_FTL
	FTL_REQUEST_ID_TYPE_HIL,					// request info type for HIL request
	FTL_REQUEST_ID_TYPE_GC,						// Request info type for Garbage Collection

	FTL_REQUEST_ID_TYPE_HIL_READ,
	FTL_REQUEST_ID_TYPE_STREAM_MERGE_READ,
	FTL_REQUEST_ID_TYPE_BLOCK_GC_READ,

	FTL_REQUEST_ID_TYPE_PROGRAM,				// Main ID Type for Program
	FTL_REQUEST_SUB_ID_TYPE_STREAM_MERGE_WRITE,	// Sub IDType for PROGRAM
	FTL_REQUEST_SUB_ID_TYPE_HIL_WRITE,			// Sub IDType for PROGRAM
	FTL_REQUEST_SUB_ID_TYPE_BLOCK_GC_WRITE,		// Sub IDType for PROGRAM

	FTL_REQUEST_ID_TYPE_DEBUG,

#elif defined(DFTL)
	FTL_REQUEST_ID_TYPE_HIL_READ,
	FTL_REQUEST_ID_TYPE_WRITE,		// Comon HIL & GC & Meta Write

	FTL_REQUEST_ID_TYPE_GC_READ,
	FTL_REQUEST_ID_TYPE_META_READ,

	FTL_REQUEST_ID_TYPE_DEBUG,
#endif

	FTL_REQUEST_ID_TYPE_COUNT,				// Must be end statement
} FTL_REQUEST_ID_TYPE;

#define FTL_REQUEST_ID_TYPE_BITS				(4)
#define FTL_REQUEST_ID_REQUEST_INDEX_BITS		(16)
#define ACTIVE_BLOCK_COUNT_BITS					8				// number of bit for active block count
#define FTL_MOVE_INFO_INDEX_BITS				3

typedef struct
{
	unsigned int	nType : FTL_REQUEST_ID_TYPE_BITS;						// FTL_REQUEST_ID_TYPE, common part
} FTL_REQUEST_ID_COMMON;

typedef struct
{
	unsigned int	nType : FTL_REQUEST_ID_TYPE_BITS;						// FTL_REQUEST_ID_TYPE, common part
	unsigned int	nRequestIndex : FTL_REQUEST_ID_REQUEST_INDEX_BITS;		// Index of HIL to FTL Request
} FTL_REQUEST_ID_HIL;

#define ACTIVE_BLOCK_BUFFERING_INDEX_BITS	8
typedef struct
{
	unsigned int	nType : FTL_REQUEST_ID_TYPE_BITS;						// FTL_REQUEST_ID_TYPE, common part
	unsigned int	nActiveBlockIndex : ACTIVE_BLOCK_COUNT_BITS;			// Index of HIL to FTL Request
	unsigned int	nBufferingIndex : ACTIVE_BLOCK_BUFFERING_INDEX_BITS;	// index of active block buffering 
	unsigned int	nIOType : 3;											// IOTYPE to distinguish Host/GC
} FTL_REQUEST_ID_PROGRAM;

typedef struct
{
	unsigned int	nType : FTL_REQUEST_ID_TYPE_BITS;						// FTL_REQUEST_ID_TYPE, common part
	unsigned int	nRequestIndex: FTL_REQUEST_ID_REQUEST_INDEX_BITS;		// Index of GC Request
	unsigned int	nMoveInfoIndex : FTL_MOVE_INFO_INDEX_BITS;				// Index of GCMgr.astMoveInfo
} FTL_REQUEST_ID_GC;

typedef struct
{
	unsigned int	nType : FTL_REQUEST_ID_TYPE_BITS;						// FTL_REQUEST_ID_TYPE, common part
	unsigned int	nRequestIndex: FTL_REQUEST_ID_REQUEST_INDEX_BITS;		// Index of Meta Request
} FTL_REQUEST_ID_META;

typedef struct
{
	union
	{
		FTL_REQUEST_ID_COMMON		stCommon;

		FTL_REQUEST_ID_HIL			stHIL;				// HIL Read / Write
		FTL_REQUEST_ID_PROGRAM		stProgram;			// FTL Program for host and GC
		FTL_REQUEST_ID_GC			stGC;
		FTL_REQUEST_ID_META			stMeta;
	};
} FTL_REQUEST_ID;

typedef struct
{
	unsigned int	nActiveBlockIndex : 16;			// Index of active block
	unsigned int	nProgramBufferingIndex : 16;	// buffering index
} PROGRAM_REQUEST_ID;

typedef enum
{
	IOCTL_INIT_PROFILE_COUNT,				// Initializa profile count
	IOCTL_PRINT_PROFILE_COUNT,
} IOCTL_TYPE;


#endif		// #ifndef __COSMOS_TYPES_H__

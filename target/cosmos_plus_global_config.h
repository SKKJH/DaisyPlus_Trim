#ifndef __COSMOS_PLUS_GLOBAL_CONFIG_H__
#define __COSMOS_PLUS_GLOBAL_CONFIG_H__

// this is top level header for global configuration
// This configurations for overall layers
// Never include any header files

// each layer configurations are refer to below files
// HIL configurations: hil_config.h
// FTL configurations: ftl_config.h
// FIL configurations: fil_config.h
// SYSTEM(cosmos+) configuratinos: cosmos_plus_system.h
// memory map: cosmos_plus_memory_map.h

#ifdef WIN32
	#define SUPPORT_DATA_VERIFICATION	// to check data consistency, the first 4byte of NAND spare area contains LPN
										// cosmos+, between FTL <-> NAND (does not work), this option trigger read UECC, LPN area of spare area must be used for ECC
										// Win32, between HIL <-> NAND

#endif

#define DATA_VERIFICATION_INVALID_LPN		0xFFFFFFFF

// Test configurations
#ifdef WIN32
	#define NVME_UNIT_TEST			(1)		// Run test with out host PC, test start functino is test_main()
	#define UNIT_TEST_PERFORMANCE	(0)		// UNIT TEST for performance
	#define UNIT_TEST_NAND_PERF		(0)		// NAND Performance
	#define UNIT_TEST_FIL_PERF		(0)		// NAND Performance
	#define UNIT_TEST_FTL_PERF		(0)		// NAND Performance
#else
	#define NVME_UNIT_TEST			(0)		// Run test with out host PC, test start functino is test_main()
	#define UNIT_TEST_PERFORMANCE	(0)		// UNIT TEST for performance
	#define UNIT_TEST_NAND_PERF		(0)		// NAND Performance
	#define UNIT_TEST_FIL_PERF		(0)		// NAND Performance
	#define UNIT_TEST_FTL_PERF		(0)		// NAND Performance
#endif

#endif	// end of __COSMOS_PLUS_GLOBAL_CONFIG_H__

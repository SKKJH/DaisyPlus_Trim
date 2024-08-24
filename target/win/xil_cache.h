#ifndef __XIL_CACHE_H__
#define __XIL_CACHE_H__

#ifndef WIN32
#error	"check your build configuration"		// windows only
#endif

#define Xil_DCacheEnable()
#define Xil_DCacheDisable()
#define Xil_DCacheInvalidateRange(Addr, Len)
#define Xil_DCacheFlushRange(Addr, Len)
#define Xil_ICacheEnable()
#define Xil_ICacheDisable()
#define Xil_ICacheInvalidateRange(Addr, Len)

#endif
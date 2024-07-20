#include "xtime_l.h"
unsigned int trim_flag, nr, trimDevAddr;
XTime cmdStart, cmdEnd, cmdTime;
XTime dataStart, dataEnd, dataTime;
XTime bufStart, bufEnd, bufTime;
XTime tableStart, tableEnd, tableTime;
void GetTrimData();
void DoTrim();
void TRIM(unsigned int LPN, unsigned int BLK0, unsigned int BLK1, unsigned int BLK2, unsigned int BLK3);

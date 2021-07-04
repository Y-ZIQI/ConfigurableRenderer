#ifndef __DEFINES
#define __DEFINES

#define M_E                 2.71828182845904523536  // e
#define M_LOG2E             1.44269504088896340736  // log2(e)
#define M_LOG10E            0.434294481903251827651 // log10(e)
#define M_LN2               0.693147180559945309417 // ln(2)
#define M_LN10              2.30258509299404568402  // ln(10)
#define M_PI                3.14159265358979323846  // pi
#define M_PI_2              1.57079632679489661923  // pi/2
#define M_PI_4              0.785398163397448309616 // pi/4
#define M_1_PI              0.318309886183790671538 // 1/pi
#define M_2_PI              0.636619772367581343076 // 2/pi
#define M_2_SQRTPI          1.12837916709551257390  // 2/sqrt(pi)
#define M_SQRT2             1.41421356237309504880  // sqrt(2)
#define M_SQRT1_2           0.707106781186547524401 // 1/sqrt(2)
#define M_2PI               6.28318530717958647693  // 2pi
#define M_4PI               12.5663706143591729539  // 4pi
#define M_4_PI              1.27323954473516268615  // 4/pi
#define M_1_2PI             0.159154943091895335769 // 1/2pi
#define M_1_4PI             0.079577471545947667884 // 1/4pi

#define ATOMIC_COUNT

#ifdef ATOMIC_COUNT
layout (binding = 0, offset = 0) uniform atomic_uint counter[8];
uint tCount = 0;
//#define ATOMIC_COUNT_INCREMENT atomicCounterIncrement(counter[0]);
#define ATOMIC_COUNT_INCREMENT tCount++;
#define ATOMIC_COUNTER_I_INCREMENT(i) atomicCounterIncrement(counter[i]);
#define ATOMIC_COUNT_CALCULATE atomic_calculate();
void atomic_calculate(){
    if((tCount & 0x00000001) != 0) atomicCounterIncrement(counter[0]);
    if((tCount & 0x00000002) != 0) atomicCounterIncrement(counter[1]);
    if((tCount & 0x00000004) != 0) atomicCounterIncrement(counter[2]);
    if((tCount & 0x00000008) != 0) atomicCounterIncrement(counter[3]);
    if((tCount & 0x00000010) != 0) atomicCounterIncrement(counter[4]);
    if((tCount & 0x00000020) != 0) atomicCounterIncrement(counter[5]);
    if((tCount & 0x00000040) != 0) atomicCounterIncrement(counter[6]);
    if((tCount & 0x00000080) != 0) atomicCounterIncrement(counter[7]);
}
#else
#define ATOMIC_COUNT_INCREMENT
#define ATOMIC_COUNTER_I_INCREMENT(i)
#define ATOMIC_COUNT_CALCULATE
#endif

#endif
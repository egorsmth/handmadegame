#ifndef INTRINSICS_H

struct bit_scan_result
{
    bool Found;
    uint32 Index;
};

inline bit_scan_result FindFirstSetBit(uint32 Value)
{
    bit_scan_result Result = {};
#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for (int32 i = 0; i < 32; i++)
    {
        if (Value & (1 << i))
        {
            Result.Found = true;
            Result.Index = i;
            break;
        }
    }
#endif
    return Result;
}

#define INTRINSICS_H
#endif
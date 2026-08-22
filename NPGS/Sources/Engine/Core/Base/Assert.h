#pragma once

// Assert
// ------
#ifdef _DEBUG
#define NPGS_ENABLE_ASSERT
#endif // _DEBUG

#ifdef NPGS_ENABLE_ASSERT
#include <cstdlib>
#include <iostream>
#ifdef _WIN32
#    include <Windows.h>
#endif

#define NpgsAssert(Expr, ...)                                                                                 \
if (!(Expr))                                                                                                  \
{                                                                                                             \
    std::cerr << "Assertion failed: " << #Expr << " in " << __FILE__ << " at line " << __LINE__ << std::endl; \
    std::cerr << "Message: " << __VA_ARGS__ << std::endl;                                                     \
    DebugBreakOrAbort();                                                                                      \
}

#define NpgsStaticAssert(Expr, ...) static_assert(Expr, __VA_ARGS__)

#ifdef _WIN32
inline void DebugBreakOrAbort()
{
    DebugBreak();
}
#else
inline void DebugBreakOrAbort()
{
    std::abort();
}
#endif // _WIN32

#else

#define NpgsAssert(Expr, ...)       static_cast<void>(0)
#define NpgsStaticAssert(Expr, ...) static_cast<void>(0)

#endif // NPGS_ENABLE_ASSERT

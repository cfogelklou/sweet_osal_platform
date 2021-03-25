/**
 * COPYRIGHT    (c)	Applicaudia 2020
 * @file        helper_macros.h
 * @brief       Useful macros for PAKM
 */


#ifndef UTILS_SRC_HELPERMACROS_H_
#define UTILS_SRC_HELPERMACROS_H_

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
// Use to exclude default values when a header file is included from C source code.
#ifndef __cplusplus
#define PRMVAL_DEFAULT(x)
#else
#define PRMVAL_DEFAULT(x) = (x)
#endif // __cplusplus

#if defined(__cplusplus) && !defined(CC26XX)
#include <type_traits>

#ifndef MAX
template <typename T1, typename T2>
inline auto MAX(const T1 &a, const T2 &b)
-> typename std::common_type<const T1&, const T2&>::type {
  return (((a) > (b)) ? (a) : (b));
}
#endif // MAX

#ifndef MIN
template <typename T1, typename T2>
inline auto MIN(const T1 &a, const T2 &b)
-> typename std::common_type<const T1&, const T2&>::type {
  return (((a) < (b)) ? (a) : (b));
}
#endif // MIN

#ifndef ABS
template <typename T>
inline T ABS(const T &x) {
  return (((x) >= 0) ? (x) : -(x));
}
#endif // ABS

#ifndef COMPILE_TIME_ASSERT
#define COMPILE_TIME_ASSERT(cond) static_assert(cond, "Compile time assert")
#endif // COMPILE_TIME_ASSERT

#ifndef ARRSZ
template <typename T, std::size_t N>
constexpr std::size_t ARRSZ(T const (&)[N]) noexcept {
  return N;
}
#endif // ARRSZ

#ifndef ARRSZN
template <typename T, int N>
constexpr int ARRSZN(T const (&)[N]) noexcept {
  return N;
}
#endif

#else // !__cplusplus

#ifndef MAX
#define MAX(x,y) (((x)>(y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) (((x)<(y)) ? (x) : (y))
#endif

#ifndef ABS
#define ABS(x) (((x) >= 0) ? (x) : -(x))
#endif

#ifndef ARRSZ
#define ARRSZ(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

#ifndef ARRSZN
#define ARRSZN(arr) ((int)(ARRSZ(arr)))
#endif

#ifndef COMPILE_TIME_ASSERT
// Will not compile if cond is false.
#define COMPILE_TIME_ASSERT(cond) do{switch(((cond)?1:0)){case 0:break;case (cond):break;}}while(0)
#endif

#endif // !__cplusplus

#ifndef MAKE_MASK
#define MAKE_MASK(bits) ((1u << (bits)) - 1)
#endif

#ifndef SET_BITS
#define SET_BITS(x, y) (x |= (y))
#endif

#ifndef CLEAR_BITS
#define CLEAR_BITS(x, y) (x &= ~(y))
#endif

#endif /* UTILS_SRC_HELPERMACROS_H_ */

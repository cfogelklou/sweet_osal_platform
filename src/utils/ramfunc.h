/**
 * COPYRIGHT    (c) Applicaudia 2020
 * @file        ramfunc.h
 * @brief       Copy function to data section of RAM using the section .ramfunctions
 */

#ifndef RAMFUNC_H
#define RAMFUNC_H

#ifdef __SPC5__

/**
 * RAMFUNC, will copy function to data section of RAM using the section .ramfunctions
 *
 * Usage:
 * Declaration in h/hpp:
 *   int RAMFUNC myfunction(int foo);
 * Definition in c/cpp:
 *   int RAMFUNC myfunction(int foo) { return foo + 1; }
 *
 */
#define RAMFUNC __attribute__ ((noinline, longcall, section (".ramfunctions")))

#else

#define RAMFUNC

#endif // __SPC5__

#endif // RAMFUNC_H

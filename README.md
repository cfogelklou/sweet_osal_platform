# sweet_osal_platform (SOP)

[![CI or PR](https://github.com/cfogelklou/sweet_osal_platform/actions/workflows/ci_pr.yml/badge.svg?branch=master)](https://github.com/cfogelklou/sweet_osal_platform/actions/workflows/ci_pr.yml)

An easily portable abstraction layer for Windows, Linux, Free-RTOS, Android, and iOS platforms containing OS abstractions and utilities that have been proven useful over my career.

Many of these source files originated in embedded projects 20 years ago, so are lightweight but also not "modern" and might be closer to C than C++ (utils/byteq for example) but correctness trumps modernity. For example, I recently replaced the "mutex" in the osal with a std::recursivemutex, as C++11 is now mostly implemented even in the embedded world.

The goal with open sourcing these is to ensure their use continues in all future projects while simultaneously refactoring them, modernizing them, and building upon them. (as opposed to the typical process of picking, choosing, and rewriting as the needs arise)

Please refer to [Github Actions](https://github.com/cfogelklou/sweet_osal_platform/actions/workflows/ci_pr.yml) for a cookbook to build the OSAL.

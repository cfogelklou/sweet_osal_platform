/*
 * cs_task_locker.hpp
 *
 *  Created on: May 16, 2020
 *      Author: Christopher
 */

#ifndef PLATFORM_LIB_CS_TASK_LOCKER_HPP_
#define PLATFORM_LIB_CS_TASK_LOCKER_HPP_

#ifdef __cplusplus
// //////////////////////////////////////////////////////////////////////////////////
//  Never forget to unlock the OSAL cs.
class CSTaskLocker {
public:
  CSTaskLocker();

  ~CSTaskLocker();

private:
};


#endif


#endif /* PLATFORM_LIB_CS_TASK_LOCKER_HPP_ */

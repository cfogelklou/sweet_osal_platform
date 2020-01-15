#ifndef CSLOCKER_HPP_
#define CSLOCKER_HPP_

#ifdef __cplusplus
// //////////////////////////////////////////////////////////////////////////////////
//  Never forget to unlock the OSAL Critical Section.
class CSLocker {
public:
  CSLocker();

  ~CSLocker();

private:
};

#endif

#endif

/**
 * COPYRIGHT    (c) Applicaudia 2020
 * @file        abstract.hpp
 * @brief       Utility macro for marking a method abstract
 */

#ifndef ABSTRACT_HPP
#define ABSTRACT_HPP

/** Usage:
In header file:

class MyClass {
public:
  virtual uint8_t MyFunc(bool &arg) ABSTRACT;
};

Implementation:

ABSTRACT_FUNCTION(
  uint8_t MyClass::MyFunc(bool &arg) {
    // dummy implementation
    (void)arg;
    LOG_WARNING(("This should never happen!"));
    return 0;
  }
)


 */

#ifdef __SPC5__
#define ABSTRACT
#define ABSTRACT_FUNCTION(x) x
#else
#define ABSTRACT = 0
#define ABSTRACT_FUNCTION(x)
#endif


#endif // ABSTRACT_HPP

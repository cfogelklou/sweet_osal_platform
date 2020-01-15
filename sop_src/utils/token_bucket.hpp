/**
 * COPYRIGHT    (c) Applicaudia 2020
 * @file        token_bucket.hpp
 */

#ifndef TOKEN_BUCKET_HPP
#define TOKEN_BUCKET_HPP

#include "platform/osal.h"
#include "helper_macros.h"

/**
 * Integer only variation of the TokenBucket to do rate limit or congestion.
 * See https://en.wikipedia.org/wiki/Token_bucket for more information
 * Works by filling a "bucket" when time increases and emptying a bit each time a message should pass.
 * Note that if the bucket is full and there is a burst, more messages than mRate can pass for a short while.
 */
class TokenBucket {
private:
  int const mRate;
  int const mTimeSpan;
  int mBucketFill;
  uint32_t mLastTime;
public:

  /**
   * Create a TokenBucket that on average passes approx rate during one time span.
   * @param rate : The number of passes
   * @param timeSpan : time span in milliseconds.
   */
  TokenBucket(const int rate, const int timeSpan)
      : mRate(rate)
      , mTimeSpan(timeSpan)
      , mBucketFill(mRate * mTimeSpan)
      , mLastTime(OSALGetMS()) {
  }

  /**
   * Check if a message should pass or not.
   * @return true if a message should pass, false if it should be queued or dropped
   */
  bool Pass() {
    auto current = OSALGetMS();
    auto timePassed = current - mLastTime;
    mLastTime = current;

    // Fill up bucket
    mBucketFill += timePassed * mRate;
    mBucketFill = MIN(mBucketFill, mRate * mTimeSpan);

    // If bucket has been filled enough, let a message pass.
    if (mBucketFill >= mTimeSpan) {
      mBucketFill -= mTimeSpan;
      return true;
    } else {
      return false;
    }
  }
};

#endif // TOKEN_BUCKET_HPP

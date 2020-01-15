#ifdef RUN_GTEST

#include <stdint.h>
#include "osal/mempools.h"
#include "osal/osal.h"
#include "utils/platform_log.h"
#include "utils/dl_list.h"
#include "utils/sl_list.h"
#include "gtest/gtest.h"
#include "tests/gtest_test_wrapper.hpp"
#include <stdlib.h>

LOG_MODNAME("test_lists.cpp")


class TestLists : public GtestMempoolsWrapper {
protected:
  TestLists() {}
  virtual ~TestLists() {}
};


#if defined(__SPC5__)
#define OSALRandom(p,l) _OSALRandom(p,l)
void _OSALRandom( uint8_t *ptmp, int len ){
  for (int i = 0; i < len; i++){
    ptmp[i] = rand() & 0xff;
  }
}
#endif

typedef struct {
  SLLNode listNode;
  uint16_t value;
  unsigned int creationOrder;
} mySllNode_t;

typedef struct {
  DLLNode listNode;
  uint16_t value;
  unsigned int creationOrder;
} myNode_t;

#if defined(WIN32) || defined(__linux__) || defined(__APPLE__)
#define NUM_ELEMENTS 10000
#else
#define NUM_ELEMENTS 10
#endif

static int bgosal_TestDllNodeCompareCb(void *pParam, const DLLNode * const pNode0,
                                       const DLLNode * const pNode1) {
  myNode_t *node0 = (myNode_t *)pNode0;
  myNode_t *node1 = (myNode_t *)pNode1;
  (void)pParam;
  return (node0->value - node1->value);
}

/* ****************************************************************************
   Description: See the prototype at the top of the file.
******************************************************************************/
TEST_F(TestLists, dll1) {
  unsigned int i = 0;
  DLL list0, list1, sortedList;
  DLL_Init(&list0);
  DLL_Init(&list1);
  DLL_Init(&sortedList);

  LOG_Log("Creating nodes\r\n");
  // Create nodes
  for (i = 0; i < NUM_ELEMENTS; i++) {
    myNode_t *p = (myNode_t *)MemPoolsHeapMalloc(sizeof(myNode_t));
    DLL_NodeInit(&p->listNode);
    uint16_t tmp;
    OSALRandom((uint8_t *)&tmp, sizeof(tmp));
    p->value = tmp % 20;
    p->creationOrder = i;
    // LOG_Log("%d\r\n", p->value);
    DLL_PushBack(&list0, &p->listNode);
  }
  EXPECT_TRUE(DLL_Size(&list0) == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&sortedList) == 0);

  LOG_Log("Sorting...\r\n");
  // test sorted insert
  {
    myNode_t *p = 0;
    i = 0;
    do {
      p = (myNode_t *)DLL_PopFront(&list0);
      if (p != NULL) {
        DLL_SortedInsert(&sortedList, &p->listNode,
                               bgosal_TestDllNodeCompareCb, NULL);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&list0) == 0);
  EXPECT_TRUE(DLL_Size(&sortedList) == NUM_ELEMENTS);

  // Test AppendList
  LOG_Log("Appending...\r\n");
  DLL_AppendListToBack(&list0, &sortedList);
  EXPECT_TRUE(DLL_Size(&list0) == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&sortedList) == 0);

  DLL_AppendListToBack(&list1, &list0);
  EXPECT_TRUE(DLL_Size(&list1) == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&list0) == 0);

  LOG_Log("Pop, Push from list1 to list0...\r\n");
  // test poplast, pushfront
  {
    myNode_t *p = 0;
    i = 0;
    do {
      p = (myNode_t *)DLL_PopBack(&list1);
      if (p != NULL) {
        // LOG_Log("%d\r\n", p->value);
        DLL_PushBack(&list0, &p->listNode);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&list0) == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&list1) == 0);

  LOG_Log("Pop, Push from list0 to list1...\r\n");
  // test popfront, pushback
  {
    myNode_t *p = 0;
    i = 0;
    do {
      p = (myNode_t *)DLL_PopFront(&list0);
      if (p != NULL) {
        // LOG_Log("%d\r\n", p->value);
        DLL_PushFront(&list1, &p->listNode);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&list0) == 0);
  EXPECT_TRUE(DLL_Size(&list1) == NUM_ELEMENTS);

  LOG_Log("Checking list still sorted...\r\n");
  // Check that all elements are sorted
  {
    bool sorted = true;
    myNode_t *pIter = (myNode_t *)DLL_Begin(&list1);
    myNode_t *pEnd = (myNode_t *)DLL_End(&list1);
    myNode_t *pLast = NULL;
    while (pIter != pEnd) {
      if (pLast) {
        if (pIter->value < pLast->value) {
          sorted = false;
        } else if (pIter->value == pLast->value) {
          // If they have the same value, then values that are inserted later
          // should
          // end up after earlier insertions.
          if (pIter->creationOrder < pLast->creationOrder) {
            sorted = false;
          }
        }
      }
      pLast = pIter;
      pIter = (myNode_t *)pIter->listNode.pNext;
    }

    EXPECT_TRUE(sorted);
  }

  // Free buffers
  {
    myNode_t *p = 0;
    i = 0;
    do {
      p = (myNode_t *)DLL_PopFront(&list1);
      if (p != NULL) {
        MemPoolsFree(p);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
}

/* ****************************************************************************
   Description: See the prototype at the top of the file.
******************************************************************************/
TEST_F(TestLists, dll2) {
  unsigned int i = 0;
  DLL list0, sortedList;
  DLL_Init(&sortedList);
  DLL_Init(&list0);

  LOG_Log("Creating nodes\r\n");
  // Create nodes
  for (i = 0; i < NUM_ELEMENTS; i++) {
    myNode_t *p = (myNode_t *)MemPoolsHeapMalloc(sizeof(myNode_t));
    DLL_NodeInit(&p->listNode);
    uint16_t tmp;
    OSALRandom((uint8_t *)&tmp, sizeof(tmp));
    p->value = tmp % 20;
    p->creationOrder = i;
    // LOG_Log("%d\r\n", p->value);
    DLL_PushBack(&list0, &p->listNode);
  }
  EXPECT_TRUE(DLL_Size(&list0) == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&sortedList) == 0);

  LOG_Log("Sorting...\r\n");
  // test sorted insert
  {
    myNode_t *p = 0;
    i = 0;
    do {
      p = (myNode_t *)DLL_PopFront(&list0);
      if (p != NULL) {
        DLL_SortedInsertFromBack(&sortedList, &p->listNode,
                                       bgosal_TestDllNodeCompareCb, NULL);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
  EXPECT_TRUE(DLL_Size(&list0) == 0);
  EXPECT_TRUE(DLL_Size(&sortedList) == NUM_ELEMENTS);

  LOG_Log("Checking list still sorted...\r\n");
  // Check that all elements are sorted
  {
    bool sorted = true;
    myNode_t *pIter = (myNode_t *)DLL_Begin(&sortedList);
    myNode_t *pEnd = (myNode_t *)DLL_End(&sortedList);
    myNode_t *pLast = NULL;
    while (pIter != pEnd) {
      if (pLast) {
        if (pIter->value < pLast->value) {
          sorted = false;
        } else if (pIter->value == pLast->value) {
          // If they have the same value, then values that are inserted later
          // should
          // end up after earlier insertions.
          if (pIter->creationOrder < pLast->creationOrder) {
            sorted = false;
          }
        }
      }
      pLast = pIter;
      pIter = (myNode_t *)pIter->listNode.pNext;
    }

    EXPECT_TRUE(sorted);
  }

  // Free buffers
  {
    myNode_t *p = 0;
    i = 0;
    do {
      p = (myNode_t *)DLL_PopFront(&sortedList);
      if (p != NULL) {
        MemPoolsFree(p);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
}

static int bgosal_TestSllNodeCompareCb(void *const pParam,
                                       const SLLNode *const pNode0,
                                       const SLLNode *const pNode1) {
  mySllNode_t *node0 = (mySllNode_t *)pNode0;
  mySllNode_t *node1 = (mySllNode_t *)pNode1;
  (void)pParam;
  return (node0->value - node1->value);
}

/* ****************************************************************************
   Description: See the prototype at the top of the file.
******************************************************************************/
TEST_F(TestLists, sll0) {
  const int numElements = NUM_ELEMENTS/3;
  for (int test = 0; test < 3; test++) {
    LOG_Log("test %d\r\n", test);
    int i;
    SLL list0, list1, sortedList;
    SLL_Init(&list0);
    EXPECT_TRUE(SLL_Size(&list0) == 0);
    SLL_Init(&list1);
    EXPECT_TRUE(SLL_Size(&list1) == 0);
    SLL_Init(&sortedList);
    EXPECT_TRUE(SLL_Size(&sortedList) == 0);

    LOG_Log("Creating nodes\r\n");
    // Create nodes
    for (i = 0; i < numElements; i++) {
      mySllNode_t *p = (mySllNode_t *)MemPoolsHeapMalloc(sizeof(mySllNode_t));
      SLL_NodeInit(&p->listNode);
      switch (test) {
      case 0:
        p->value = i;
        break;
      case 1:
        p->value = numElements - i;
        break;
      default:
        p->value = 1;
        break;
      }
      p->creationOrder = i;
      SLL_PushBack(&list0, &p->listNode);
    }
    EXPECT_TRUE(SLL_Size(&list0) == numElements);
    EXPECT_TRUE(SLL_Size(&sortedList) == 0);

    LOG_Log("Sorting...\r\n");
    // test sorted insert
    {
      mySllNode_t *p = 0;
      i = 0;
      do {
        p = (mySllNode_t *)SLL_PopFront(&list0);
        if (p != NULL) {
          SLL_SortedInsert(&sortedList, &p->listNode,
                           bgosal_TestSllNodeCompareCb, NULL);
          ++i;
        }
      } while (p != 0);
    }
    EXPECT_TRUE(i == numElements);
    EXPECT_TRUE(SLL_Size(&list0) == 0);
    EXPECT_TRUE(list0.dummyNode.pNext == &list0.dummyNode);
    EXPECT_TRUE(list0.pLast == &list0.dummyNode);
    EXPECT_TRUE(SLL_Size(&sortedList) == numElements);

    // Test AppendList
    LOG_Log("Appending...\r\n");
    SLL_AppendListToBack(&list0, &sortedList);
    EXPECT_TRUE(SLL_Size(&list0) == numElements);
    EXPECT_TRUE(SLL_Size(&sortedList) == 0);

    SLL_AppendListToBack(&list1, &list0);
    EXPECT_TRUE(SLL_Size(&list1) == numElements);
    EXPECT_TRUE(SLL_Size(&list0) == 0);

    LOG_Log("Pop, Push from list1 to list0...\r\n");
    // test poplast, pushfront
    {
      mySllNode_t *p = 0;
      i = 0;
      do {
        p = (mySllNode_t *)SLL_PopBack(&list1);
        if (p != NULL) {
          // LOG_Log("%d\r\n", p->value);
          SLL_PushBack(&list0, &p->listNode);
          ++i;
        }
      } while (p != 0);
    }
    EXPECT_TRUE(i == numElements);
    EXPECT_TRUE(SLL_Size(&list0) == numElements);
    EXPECT_TRUE(SLL_Size(&list1) == 0);

    LOG_Log("Pop, Push from list0 to list1...\r\n");
    // test popfront, pushback
    {
      mySllNode_t *p = 0;
      i = 0;
      do {
        p = (mySllNode_t *)SLL_PopFront(&list0);
        if (p != NULL) {
          // LOG_Log("%d\r\n", p->value);
          SLL_PushFront(&list1, &p->listNode);
          ++i;
        }
      } while (p != 0);
    }
    EXPECT_TRUE(i == numElements);
    EXPECT_TRUE(SLL_Size(&list0) == 0);
    EXPECT_TRUE(SLL_Size(&list1) == numElements);

    LOG_Log("Checking list still sorted...\r\n");
    // Check that all elements are sorted
    {
      bool sorted = true;
      mySllNode_t *pIter = (mySllNode_t *)SLL_Begin(&list1);
      mySllNode_t *pEnd = (mySllNode_t *)SLL_End(&list1);
      mySllNode_t *pLast = NULL;
      while (pIter != pEnd) {
        if (pLast) {
          if (pIter->value < pLast->value) {
            sorted = false;
          } else if (pIter->value == pLast->value) {
            // If they have the same value, then values that are inserted later
            // should
            // end up after earlier insertions.
            if (pIter->creationOrder < pLast->creationOrder) {
              sorted = false;
            }
          }
        }
        pLast = pIter;
        pIter = (mySllNode_t *)pIter->listNode.pNext;
      }

      EXPECT_TRUE(sorted);
    }

    // Free buffers
    {
      mySllNode_t *p = 0;
      i = 0;
      do {
        p = (mySllNode_t *)SLL_PopFront(&list1);
        if (p != NULL) {
          MemPoolsFree(p);
          ++i;
        }
      } while (p != 0);
    }
    EXPECT_TRUE(i == numElements);
  }
}

/* ****************************************************************************
   Description: See the prototype at the top of the file.
******************************************************************************/
TEST_F(TestLists, sll1) {
  unsigned int i = 0;
  SLL list0, list1, sortedList;
  SLL_Init(&list0);
  EXPECT_TRUE(SLL_Size(&list0) == 0);
  SLL_Init(&list1);
  EXPECT_TRUE(SLL_Size(&list1) == 0);
  SLL_Init(&sortedList);
  EXPECT_TRUE(SLL_Size(&sortedList) == 0);

  LOG_Log("Creating nodes\r\n");
  // Create nodes
  for (i = 0; i < NUM_ELEMENTS; i++) {
    mySllNode_t *p = (mySllNode_t *)MemPoolsHeapMalloc(sizeof(mySllNode_t));
    SLL_NodeInit(&p->listNode);
    uint16_t tmp;
    OSALRandom((uint8_t *)&tmp, sizeof(tmp));
    p->value = tmp % 20;
    p->creationOrder = i;
    // LOG_Log("%d\r\n", p->value);
    SLL_PushBack(&list0, &p->listNode);
  }
  EXPECT_TRUE(SLL_Size(&list0) == NUM_ELEMENTS);
  EXPECT_TRUE(SLL_Size(&sortedList) == 0);

  LOG_Log("Sorting...\r\n");
  // test sorted insert
  {
    mySllNode_t *p = 0;
    i = 0;
    do {
      p = (mySllNode_t *)SLL_PopFront(&list0);
      if (p != NULL) {
        SLL_SortedInsert(&sortedList, &p->listNode, bgosal_TestSllNodeCompareCb,
                         NULL);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
  EXPECT_TRUE(SLL_Size(&list0) == 0);
  EXPECT_TRUE(list0.dummyNode.pNext == &list0.dummyNode);
  EXPECT_TRUE(list0.pLast == &list0.dummyNode);
  EXPECT_TRUE(SLL_Size(&sortedList) == NUM_ELEMENTS);

  // Test AppendList
  LOG_Log("Appending...\r\n");
  SLL_AppendListToBack(&list0, &sortedList);
  EXPECT_TRUE(SLL_Size(&list0) == NUM_ELEMENTS);
  EXPECT_TRUE(SLL_Size(&sortedList) == 0);

  SLL_AppendListToBack(&list1, &list0);
  EXPECT_TRUE(SLL_Size(&list1) == NUM_ELEMENTS);
  EXPECT_TRUE(SLL_Size(&list0) == 0);

  LOG_Log("Pop, Push from list1 to list0...\r\n");
  // test poplast, pushfront
  {
    mySllNode_t *p = 0;
    i = 0;
    do {
      p = (mySllNode_t *)SLL_PopBack(&list1);
      if (p != NULL) {
        // LOG_Log("%d\r\n", p->value);
        SLL_PushBack(&list0, &p->listNode);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
  EXPECT_TRUE(SLL_Size(&list0) == NUM_ELEMENTS);
  EXPECT_TRUE(SLL_Size(&list1) == 0);

  LOG_Log("Pop, Push from list0 to list1...\r\n");
  // test popfront, pushback
  {
    mySllNode_t *p = 0;
    i = 0;
    do {
      p = (mySllNode_t *)SLL_PopFront(&list0);
      if (p != NULL) {
        // LOG_Log("%d\r\n", p->value);
        SLL_PushFront(&list1, &p->listNode);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
  EXPECT_TRUE(SLL_Size(&list0) == 0);
  EXPECT_TRUE(SLL_Size(&list1) == NUM_ELEMENTS);

  LOG_Log("Checking list still sorted...\r\n");
  // Check that all elements are sorted
  {
    bool sorted = true;
    mySllNode_t *pIter = (mySllNode_t *)SLL_Begin(&list1);
    mySllNode_t *pEnd = (mySllNode_t *)SLL_End(&list1);
    mySllNode_t *pLast = NULL;
    while (pIter != pEnd) {
      if (pLast) {
        if (pIter->value < pLast->value) {
          sorted = false;
        } else if (pIter->value == pLast->value) {
          // If they have the same value, then values that are inserted later
          // should
          // end up after earlier insertions.
          if (pIter->creationOrder < pLast->creationOrder) {
            sorted = false;
          }
        }
      }
      pLast = pIter;
      pIter = (mySllNode_t *)pIter->listNode.pNext;
    }

    EXPECT_TRUE(sorted);
  }

  // Free buffers
  {
    mySllNode_t *p = 0;
    i = 0;
    do {
      p = (mySllNode_t *)SLL_PopFront(&list1);
      if (p != NULL) {
        MemPoolsFree(p);
        ++i;
      }
    } while (p != 0);
  }
  EXPECT_TRUE(i == NUM_ELEMENTS);
}

/* ****************************************************************************
   Description: Test that PushBack followed by PushFront works as expected
******************************************************************************/
TEST_F(TestLists, SLLBackThenFront) {
  SLL list0;
  SLL_Init(&list0);
  EXPECT_TRUE(SLL_Size(&list0) == 0);

  // PushBack
  {
    mySllNode_t *p = (mySllNode_t *) MemPoolsHeapMalloc(sizeof(mySllNode_t));
    SLL_NodeInit(&p->listNode);
    SLL_PushBack(&list0, &p->listNode);
  }
  EXPECT_TRUE(SLL_Size(&list0) == 1);

  // PushFront
  {
    mySllNode_t *p = (mySllNode_t *) MemPoolsHeapMalloc(sizeof(mySllNode_t));
    SLL_NodeInit(&p->listNode);
    SLL_PushFront(&list0, &p->listNode);
  }
  EXPECT_TRUE(SLL_Size(&list0) == 2);

  // Free buffers
  {
    mySllNode_t *p = 0;
    do {
      p = (mySllNode_t *)SLL_PopFront(&list0);
      if (p != NULL) {
        MemPoolsFree(p);
      }
    } while (p != 0);
  }
  EXPECT_TRUE(SLL_Size(&list0) == 0);
}

/* ****************************************************************************
   Description: Test that PushFront followed by PushBack works as expected
******************************************************************************/
TEST_F(TestLists, SLLFrontThenBack) {
  SLL list0;
  SLL_Init(&list0);
  EXPECT_TRUE(SLL_Size(&list0) == 0);

  // PushFront
  {
    mySllNode_t *p = (mySllNode_t *) MemPoolsHeapMalloc(sizeof(mySllNode_t));
    SLL_NodeInit(&p->listNode);
    SLL_PushFront(&list0, &p->listNode);
  }
  EXPECT_TRUE(SLL_Size(&list0) == 1);

  // PushBack
  {
    mySllNode_t *p = (mySllNode_t *) MemPoolsHeapMalloc(sizeof(mySllNode_t));
    SLL_NodeInit(&p->listNode);
    SLL_PushBack(&list0, &p->listNode);
  }
  EXPECT_TRUE(SLL_Size(&list0) == 2);

  // Free buffers
  {
    mySllNode_t *p = 0;
    do {
      p = (mySllNode_t *)SLL_PopFront(&list0);
      if (p != NULL) {
        MemPoolsFree(p);
      }
    } while (p != 0);
  }
  EXPECT_TRUE(SLL_Size(&list0) == 0);
}

#endif

#include "utils/sl_list.h"
#include "utils/platform_log.h"
#include <cstddef>

LOG_MODNAME("sl_list.cpp")


#define SLL_MAX_LIST_SZ 20000

extern "C" {

// ////////////////////////////////////////////////////////////////////////////
void SLL_Init(SLL *const pList) {
  LOG_ASSERT(NULL != pList);

  // When empty, prev and next in the SLL's list both point back at the SLL's
  // dummyNode.
  pList->pLast = &pList->dummyNode;
  pList->dummyNode.pNext = &pList->dummyNode;
}

// ////////////////////////////////////////////////////////////////////////////
/// Insert a SLL node at the end of the linked list
void SLL_PushBack(SLL *const pList, SLLNode *const pNode) {
  LOG_ASSERT(pNode && (pNode == pNode->pNext || pNode->pNext == NULL));
  if ((pList) && (pNode)) {
    // Checks that the list was initialized
    // Checks that the list was initialized
    LOG_ASSERT((pList->dummyNode.pNext) && (pList->pLast));
    pList->pLast->pNext = pNode;
    pNode->pNext = &pList->dummyNode;
    pList->pLast = pNode;
    LOG_ASSERT(pList->dummyNode.pNext != &pList->dummyNode);
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Insert a node at the beginning of the doubly-linked-list
void SLL_PushFront(SLL *const pList, SLLNode *const pNode) {
  LOG_ASSERT(pNode && (pNode == pNode->pNext || pNode->pNext == NULL));

  // Puts it at the beginning of the specified SLL.
  if ((pList) && (pNode)) {
    pNode->pNext = pList->dummyNode.pNext;
    pList->dummyNode.pNext = pNode;

    // When going from empty to non-empty list, update list's pLast
    if (pList->pLast == &pList->dummyNode) {
      pList->pLast = pNode;
    }
    LOG_ASSERT(pList->dummyNode.pNext != &pList->dummyNode);
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Removes all elements from DllToAppendPtr and appends them
/// to the end of pList.
void SLL_AppendListToBack(SLL *const pList, SLL *const pSllToAppend) {
  SLLNode *pList2First = SLL_GetFront(pSllToAppend);
  SLLNode *pList2Last = SLL_GetBack(pSllToAppend);

  if ((pList2First) && (pList2Last)) {
    // The next pointer from the end of the first list must point at
    // the first element from the second list
    pList->pLast->pNext = pList2First;

    // The next pointer for the last element in the last list now must
    // point at the dummy node
    pList2Last->pNext = &pList->dummyNode;

    // Aim the list's last pointer at the last element in the second
    // list.
    pList->pLast = pList2Last;
  }
  SLL_Init(pSllToAppend);
}

// ////////////////////////////////////////////////////////////////////////////
/// Gets the size of the list and validates that the list is
/// circular.
unsigned int SLL_Size(const SLL *const pList) {
  unsigned int size = 0;
  if (pList) {
    SLLNode *pIter = SLL_Begin(pList);
    SLLNode *const pEnd = SLL_End(pList);

    // Use an arbitrary large number to stop the search if stuck in an endless
    // loop.
    while ((pIter != pEnd) && (size < SLL_MAX_LIST_SZ)) {
      pIter = pIter->pNext;
      ++size;
    }
  }
  LOG_ASSERT(size < SLL_MAX_LIST_SZ);
  return size;
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns true if the node is in a list, otherwise returns false
bool SLL_IsListed(const SLL *const pList, const SLLNode *const pNode) {
  SLLNode *pIter = SLL_Begin(pList);
  SLLNode *const pEnd = SLL_End(pList);

  // Use an arbitrary large number to stop the search if stuck in an endless
  // loop.
  while ((pIter != pEnd) && (pIter != pNode)) {
    pIter = pIter->pNext;
  }
  return (pIter == pNode) ? true : false;
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns true if the SLL is empty, otherwise returns false.
bool SLL_IsEmpty(const SLL *const pList) {
  // Returns true if the list is empty, otherwise returns false
  return (pList->dummyNode.pNext == &pList->dummyNode) ? true : false;
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the first item in the list, or NULL if the list
/// is empty.
SLLNode *SLL_GetFront(const SLL *const pList) {
  // Return the element at the beginning of the list
  if (!SLL_IsEmptyFast(pList)) {
    return pList->dummyNode.pNext;
  } else {
    return (SLLNode *)NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the last item in the list, or NULL if the list
/// is empty.
SLLNode *SLL_GetBack(const SLL *const pList) {
  // Return the element at the end of the list
  if (!SLL_IsEmptyFast(pList)) {
    return pList->pLast;
  } else {
    return (SLLNode *)NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Gets the node at the specified index, or NULL if outside range of the list.
SLLNode *SLL_GetNodeAtIdx(const SLL *const pList, const int idx) {
  SLLNode *pIter = SLL_Begin(pList);
  SLLNode *const pEnd = SLL_End(pList);
  int count = 0;
  // Use an arbitrary large number to stop the search if stuck in an endless
  // loop.
  while ((pIter != pEnd) && (count < idx)) {
    count++;
    pIter = pIter->pNext;
  }
  return (pIter == pEnd) ? NULL : pIter;
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the  first item in the list, or a pointer to
/// the list's dummyNode if the list is empty.
SLLNode *SLL_Begin(const SLL *const pList) { return pList->dummyNode.pNext; }

// ////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the item after the last item in the list.
/// See documentation for SLL_Begin() for a usage example.
SLLNode *SLL_End(const SLL *const pList) {
  // Cast because const doesn't match the return type.
  return (SLLNode *)&pList->dummyNode;
}

// ////////////////////////////////////////////////////////////////////////////
/// For each list element, calls the callback function
void SLL_ForEachNode(SLL *const pList, SLL_ForEachNodeCbType *const pCb,
                     void *const pParam) {
  LOG_ASSERT(NULL != pList);
  LOG_ASSERT(NULL != pCb);
  if (!SLL_IsEmptyFast(pList)) {
    bool keepGoing = true;
    SLLNode *pNode = pList->dummyNode.pNext;
    do {
      SLLNode *const pNext = pNode->pNext;
      keepGoing = pCb(pParam, pNode);
      pNode = pNext;
    } while ((keepGoing) && (pNode != &pList->dummyNode));
  }

  if (pList->pLast != &pList->dummyNode){
    LOG_ASSERT(pList->dummyNode.pNext != &pList->dummyNode);
  }

}

// ////////////////////////////////////////////////////////////////////////////
/// Performs a sorted insert on a doubly linked list.  Note, the list must
/// already
/// be sorted before using this function.  (Can not do a sorted insert on an
/// unsorted list.)
void SLL_SortedInsert(SLL *const pList, SLLNode *const pNode,
                      SLL_NodeCompareCbFnT *const pCb, void *const pParam) {
  bool inserted = false;
  LOG_ASSERT((NULL != pList) && (NULL != pCb));
  // LOG_ASSERT_WARN(pNode->pNext == NULL);

  if (SLL_IsEmpty(pList)) {
  } else {
    SLLNode *const pEnd = &pList->dummyNode;
    SLLNode *pPrev = pEnd;
    SLLNode *pNext = pList->dummyNode.pNext;
    while ((!inserted) && (pNext != pEnd)) {
      // Compare the node with the list node.
      int nodeMinusNext = pCb(pParam, pNode, pNext);

      // If negative, then the value of pNode is less than pNext
      if (nodeMinusNext < 0) {
        pNode->pNext = pPrev->pNext;
        pPrev->pNext = pNode;
        inserted = true;
      }

      pPrev = pNext;
      pNext = pNext->pNext;
      LOG_ASSERT(pNext != 0);
    }
  }

  // If not inserted, then insert at the end.
  if (!inserted) {
    SLL_PushBack(pList, pNode);
  }
  LOG_ASSERT(pList->pLast->pNext == &pList->dummyNode);

  if (pList->pLast != &pList->dummyNode){
    LOG_ASSERT(pList->dummyNode.pNext != &pList->dummyNode);
  }

}

// ////////////////////////////////////////////////////////////////////////////
// Unlists the node in pNode from the list it belongs to.
bool SLL_NodeUnlistQuick(SLL *const pList, SLLNode *const pNodeBefore,
                         SLLNode *const pNodeToUnlist) {
  bool rval = false;
  if ((pList) && (pNodeToUnlist) && (pNodeBefore) &&
      (pNodeToUnlist->pNext != NULL)) {
    // Remove the node by linking the previous node's next pointer
    // to this node's next pointer.
    if (pNodeBefore->pNext == pNodeToUnlist) {
      // link pointers
      pNodeBefore->pNext = pNodeToUnlist->pNext;

      // Was this the last node?
      if (pList->pLast == pNodeToUnlist) {
        // The previous node is now the last node.
        pList->pLast = pNodeBefore;
      }
      rval = true;
      pNodeToUnlist->pNext = pNodeToUnlist;
    }
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
// Unlists the node in pNode from the list it belongs to.
bool SLL_NodeUnlist(SLL *const pList, SLLNode *const pNode) {
  bool rval = false;
  if ((pList) && (pNode) && (pNode->pNext != NULL)) {
    bool done = false;
    SLLNode *pNodeBefore = &pList->dummyNode;

    // Find the node before the node to remove.
    while ((!done) & (pNodeBefore->pNext != pNode)) {
      pNodeBefore = pNodeBefore->pNext;
      if (pNodeBefore == &pList->dummyNode) {
        done = true;
        break;
      }
    }

    rval = SLL_NodeUnlistQuick(pList, pNodeBefore, pNode);
  }

  if ((pList) && (pList->pLast != &pList->dummyNode)){
    LOG_ASSERT(pList->dummyNode.pNext != &pList->dummyNode);
  }

  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
// Clear a list by popping all entries.
void SLL_PopAll(SLL *const pList) {
  SLLNode *pNode;
  do {
    pNode = SLL_PopFront(pList);
  } while (pNode != NULL);
}

// ////////////////////////////////////////////////////////////////////////////
/// Pops the first item in the list and returns a pointer to it.
SLLNode *SLL_PopFront(SLL *const pList) {
  SLLNode *pRVal = SLL_GetFront(pList);

  if (NULL != pRVal) {
    if (pRVal == pList->pLast) {
      pList->pLast = &pList->dummyNode;
    }
    pList->dummyNode.pNext = pRVal->pNext;
    pRVal->pNext = pRVal;
  }

  if (pList->pLast != &pList->dummyNode){
    LOG_ASSERT(pList->dummyNode.pNext != &pList->dummyNode);
  }

  return pRVal;
}

/* ****************************************************************************
   Description: See .h-file
******************************************************************************/
SLLNode *SLL_PopBack(SLL *const pList) {
  SLLNode *pRVal = SLL_GetBack(pList);

  if (NULL != pRVal) {
    // Unlisting can not fail, because we know that the element is in the list.
    LOG_ASSERT_FN(SLL_NodeUnlist(pList, pRVal));
    LOG_ASSERT(pList->pLast->pNext == &pList->dummyNode);
  }

  if (pList->pLast != &pList->dummyNode){
    LOG_ASSERT(pList->dummyNode.pNext != &pList->dummyNode);
  }

  return pRVal;
}

// ////////////////////////////////////////////////////////////////////////////
/// Initializes a SLL list node
void SLL_NodeInit(SLLNode *const pNode) {
  if (NULL != pNode) {
    pNode->pNext = pNode;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Inserts pNewNode after pOldNode.
void SLL_NodeInsertAfter(SLLNode *const pNewNode, SLLNode *const pOldNode) {
  LOG_ASSERT(pNewNode && (pNewNode == pNewNode->pNext || pNewNode->pNext == NULL));
  if ((pNewNode) && (pOldNode)) {
    SLL_NodeInit(pNewNode);

    pNewNode->pNext = pOldNode->pNext;
    pOldNode->pNext = pNewNode;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Softs the list pList using the callback function CallbackPtr.
void SLL_SortList(SLL *const pList, SLL_NodeCompareCbFnT *const CallbackPtr,
                  void *const ParamPtr) {
  SLL tmpList;
  SLLNode *pNode;

  // Initialize the temporary list and append the list to the back of it.
  SLL_Init(&tmpList);
  SLL_AppendListToBack(&tmpList, pList);
  LOG_ASSERT(SLL_IsEmpty(pList));

  // Pop all elements from the temporary list and sorted insert them into the
  // list.
  do {
    pNode = SLL_PopFront(&tmpList);
    if (pNode) {
      SLL_SortedInsert(pList, pNode, CallbackPtr, ParamPtr);
    }
  } while (pNode);
}

} // extern "C"

#include "sl_list.hpp"

using namespace sll;


list::list() { SLL_Init(&sll); }

void list::push_back(SLLNode *const pNode) { SLL_PushBack(&sll, pNode); }

void list::push_front(SLLNode *const pNode) { SLL_PushFront(&sll, pNode); }

SLLNode *list::begin() { return SLL_Begin(&sll); }

SLLNode *list::end() { return SLL_End(&sll); }

SLLNode *list::pop_front() { return SLL_PopFront(&sll); }

SLLNode *list::get_front() { return SLL_GetFront(&sll); }

bool list::isEmpty() { return SLL_IsEmpty(&sll); }

bool list::unlist(SLLNode *pNode) { return SLL_NodeUnlist(&sll, pNode); }

void list::for_each(SLL_ForEachNodeCbType cb, void *pUserData) {
  SLL_ForEachNode(&sll, cb, pUserData);
}

void list::for_each_pop(SLL_ForEachNodeCbType cb, void *pUserData) {
  SLLNode * pItem = pop_front();
  while (pItem){
    cb( pUserData, pItem);
    pItem = pop_front();
  }
}

int list::count() {
  return SLL_Size(&sll);
}

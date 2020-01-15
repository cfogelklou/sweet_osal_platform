#include "utils/dl_list.h"
#include "utils/platform_log.h"
#include <cstddef>

LOG_MODNAME("dl_list.cpp")

#define DLL_MAX_LIST_SZ 20000

extern "C" {

// ////////////////////////////////////////////////////////////////////////////
void DLL_Init(DLL *const pList) {
  LOG_ASSERT(NULL != pList);

  // When empty, prev and next in the DLL's list both point back at the DLL's
  // dummyNode.
  pList->dummyNode.pPrev = &pList->dummyNode;
  pList->dummyNode.pNext = &pList->dummyNode;
}

// ////////////////////////////////////////////////////////////////////////////
/// Insert a DLL node at the end of the linked list
void DLL_PushBack(DLL *const pList, DLLNode *const pNode) {
  LOG_ASSERT(pNode && (pNode->pNext == NULL) && (pNode->pPrev == NULL));
  if ((pList) && (pNode)) {
    pList->dummyNode.pPrev->pNext = pNode;
    pNode->pPrev = pList->dummyNode.pPrev;
    pNode->pNext = &pList->dummyNode;
    pList->dummyNode.pPrev = pNode;
    LOG_ASSERT(pNode != pNode->pNext);
    LOG_ASSERT(pNode != pNode->pPrev);
    LOG_ASSERT(pNode->pNext == &pList->dummyNode);
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Insert a node at the beginning of the doubly-linked-list
void DLL_PushFront(DLL *const pList, DLLNode *const pNode) {
  LOG_ASSERT(pNode && (pNode == pNode->pNext || pNode->pNext == NULL) && (pNode == pNode->pPrev || pNode->pPrev == NULL));

  // Puts it at the beginning of the specified DLL.
  if ((pList) && (pNode)) {
    pList->dummyNode.pNext->pPrev = pNode;
    pNode->pPrev = &pList->dummyNode;
    pNode->pNext = pList->dummyNode.pNext;
    pList->dummyNode.pNext = pNode;
    LOG_ASSERT(pNode != pNode->pNext);
    LOG_ASSERT(pNode != pNode->pPrev);
    LOG_ASSERT(pNode->pPrev == &pList->dummyNode);
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Removes all elements from DllToAppendPtr and appends them
/// to the end of pList.
void DLL_AppendListToBack(DLL *const pList, DLL *const pDllToAppend) {
  DLLNode *pList2First = DLL_GetFront(pDllToAppend);
  DLLNode *pList2Last = DLL_GetBack(pDllToAppend);
  if ((pList2First != NULL) && (pList2Last != NULL)) {
    DLLNode *pList1Last = pList->dummyNode.pPrev;
    pList1Last->pNext = pList2First;
    pList2First->pPrev = pList1Last;
    pList2Last->pNext = &pList->dummyNode;
    pList->dummyNode.pPrev = pList2Last;
  }
  DLL_Init(pDllToAppend);
}

// ////////////////////////////////////////////////////////////////////////////
/// Gets the size of the list and validates that the list is
/// circular.
unsigned int DLL_Size(const DLL *const pList) {
  unsigned int size = 0;
  if (pList) {
    DLLNode *pIter = DLL_Begin(pList);
    DLLNode *const pEnd = DLL_End(pList);

    // Use an arbitrary large number to stop the search if stuck in an endless
    // loop.
    while ((pIter != pEnd) && (size < DLL_MAX_LIST_SZ)) {
      LOG_ASSERT(pIter->pNext->pPrev == pIter);
      LOG_ASSERT(pIter->pPrev->pNext == pIter);
      pIter = pIter->pNext;
      ++size;
    }
  }
  LOG_ASSERT(size < DLL_MAX_LIST_SZ);
  return size;
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns true if the node is in a list, otherwise returns false
bool DLL_IsListed(const DLL *const pList, const DLLNode *const pNode) {
  DLLNode *pIter = DLL_Begin(pList);
  DLLNode *const pEnd = DLL_End(pList);

  // Use an arbitrary large number to stop the search if stuck in an endless
  // loop.
  while ((pIter != pEnd) && (pIter != pNode)) {
    pIter = pIter->pNext;
  }
  return (pIter == pNode) ? true : false;
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns true if the DLL is empty, otherwise returns false.
bool DLL_IsEmpty(const DLL *const pList) {
  // Returns true if the list is empty, otherwise returns false
  return (pList->dummyNode.pNext == &pList->dummyNode) ? true : false;
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the first item in the list, or NULL if the list
/// is empty.
DLLNode *DLL_GetFront(const DLL *const pList) {
  // Return the element at the beginning of the list
  if (!DLL_IsEmptyFast(pList)) {
    return pList->dummyNode.pNext;
  } else {
    return (DLLNode *)NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the last item in the list, or NULL if the list
/// is empty.
DLLNode *DLL_GetBack(const DLL *const pList) {
  // Return the element at the end of the list
  if (!DLL_IsEmptyFast(pList)) {
    return pList->dummyNode.pPrev;
  } else {
    return (DLLNode *)NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Gets the node at the specified index, or NULL if outside range of the list.
DLLNode *DLL_GetNodeAtIdx(const DLL *const pList, const int idx) {
  DLLNode *pIter = DLL_Begin(pList);
  DLLNode *const pEnd = DLL_End(pList);
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
DLLNode *DLL_Begin(const DLL *const pList) { return pList->dummyNode.pNext; }

// ////////////////////////////////////////////////////////////////////////////
/// Returns a pointer to the item after the last item in the list.
/// See documentation for DLL_Begin() for a usage example.
DLLNode *DLL_End(const DLL *const pList) {
  // Cast because const doesn't match the return type.
  return (DLLNode *)&pList->dummyNode;
}

// ////////////////////////////////////////////////////////////////////////////
/// For each list element, calls the callback function
void DLL_ForEachNode(DLL *const pList, DLL_ForEachNodeCbType *const pCb,
                     void *const pParam) {
  LOG_ASSERT(NULL != pList);
  LOG_ASSERT(NULL != pCb);
  if (!DLL_IsEmptyFast(pList)) {
    bool keepGoing = true;
    DLLNode *pNode = pList->dummyNode.pNext;
    do {
      DLLNode *const pNext = pNode->pNext;
      keepGoing = pCb(pParam, pNode);
      pNode = pNext;
    } while ((keepGoing) && (pNode != &pList->dummyNode));
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Performs a sorted insert on a doubly linked list.  Note, the list must
/// already
/// be sorted before using this function.  (Can not do a sorted insert on an
/// unsorted list.)
bool DLL_SortedInsert(DLL *const pList, DLLNode *const pNode,
                      DLL_NodeCompareCbType *const pCb, void *const pParam) {
  bool wasInserted = false;
  LOG_ASSERT((NULL != pList) && (NULL != pCb));

  if (DLL_IsEmptyFast(pList)) {
    // empty, so wasInserted will be false and DLL_PushBack() will be called.
  } else {
    DLLNode *pNext = pList->dummyNode.pNext;
    LOG_ASSERT(pNode != pNext);

    while ((!wasInserted) && (pNext != &pList->dummyNode)) {
      // Quit immediately if node already is in the list.

      // Compare the node with the list node.
      const int nodeMinusNext = pCb(pParam, pNode, pNext);

      LOG_ASSERT(pNode != pNext);

      // If negative, then the value of pNode is less than pNext
      if (nodeMinusNext < 0) {
        DLL_NodeInsertBefore(pNode, pNext);
        wasInserted = true;
      }

      if (pNext->pNext == nullptr) {
        return false;
      }
      LOG_ASSERT(pNext->pNext != NULL);
      pNext = pNext->pNext;
    }
  }

  // If not inserted, then insert at the end.
  if (!wasInserted) {
    DLL_PushBack(pList, pNode);
    wasInserted = true;
  }
  return wasInserted;
}

// ////////////////////////////////////////////////////////////////////////////
/// Performs a sorted insert on a doubly linked list.  Note, the list must
/// already
/// be sorted before using this function.  (Can not do a sorted insert on an
/// unsorted list.)
void DLL_SortedInsertFromBack(DLL *const pList, DLLNode *const pNode,
                              DLL_NodeCompareCbType *const pCb,
                              void *const pParam) {
  bool wasInserted = false;
  LOG_ASSERT((NULL != pList) && (NULL != pCb));
  LOG_ASSERT(pNode && (pNode == pNode->pNext || pNode->pNext == NULL) && (pNode == pNode->pPrev || pNode->pPrev == NULL));
  if (DLL_IsEmptyFast(pList)) {
  } else {
    DLLNode *pPrev = pList->dummyNode.pPrev;
    while ((!wasInserted) && (pPrev != &pList->dummyNode)) {
      // Compare the node with the list node.
      int prevMinusNode = pCb(pParam, pPrev, pNode);

      // If negative, then the value of pNode is less than pNext
      if (prevMinusNode <= 0) {
        DLL_NodeInsertAfter(pNode, pPrev);
        wasInserted = true;
      }

      pPrev = pPrev->pPrev;
      LOG_ASSERT(pPrev != 0);
    }
  }

  // If not wasInserted, then insert at the end.
  if (!wasInserted) {
    DLL_PushFront(pList, pNode);
  }
}

// ////////////////////////////////////////////////////////////////////////////
// Clear a list by popping all entries.
void DLL_PopAll(DLL *const pList) {
  DLLNode *pNode;
  do {
    pNode = DLL_PopFront(pList);
  } while (pNode != NULL);
}

// ////////////////////////////////////////////////////////////////////////////
/// Pops the first item in the list and returns a pointer to it.
DLLNode *DLL_PopFront(DLL *const pList) {
  DLLNode *pRval = DLL_GetFront(pList);

  if (NULL != pRval) {
    DLL_NodeUnlist(pRval);
    LOG_ASSERT(pList->dummyNode.pNext != pRval);
  }

  return pRval;
}

// ////////////////////////////////////////////////////////////////////////////
/// Pops the last item in the list and returns a pointer to it.
DLLNode *DLL_PopBack(DLL *const pList) {
  DLLNode *pRval = DLL_GetBack(pList);

  if (NULL != pRval) {
    DLL_NodeUnlist(pRval);
  }

  return pRval;
}

// ////////////////////////////////////////////////////////////////////////////
/// Initializes a DLL list node
void DLL_NodeInit(DLLNode *const pNode) {
  LOG_ASSERT(pNode);
  LOG_ASSERT((pNode == pNode->pNext || pNode->pNext == NULL));
  LOG_ASSERT((pNode == pNode->pPrev || pNode->pPrev == NULL));
  if (NULL != pNode) {
    pNode->pPrev = pNode->pNext = NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Removes the node from whataver list it is a member of.
void DLL_NodeUnlist(DLLNode *const pNode) {
  if ((pNode) && (pNode->pNext)) {
    // Unlist the node
    pNode->pPrev->pNext = pNode->pNext;
    pNode->pNext->pPrev = pNode->pPrev;

    // And point the node's pointers at itself.
    pNode->pPrev = pNode->pNext = NULL;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Returns true if the node is in a list, otherwise returns false
bool DLL_NodeIsListed(const DLLNode *const pNode) {
  bool rval = false;
  if ((pNode) && (pNode->pNext) && (pNode->pPrev) && (pNode->pNext != pNode) &&
      (pNode->pPrev != pNode)) {
    rval = true;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
/// Inserts pNewNode before pOldNode.
void DLL_NodeInsertBefore(DLLNode *const pNewNode, DLLNode *const pOldNode) {
  LOG_ASSERT(pNewNode && (pNewNode == pNewNode->pNext || pNewNode->pNext == NULL) && (pNewNode == pNewNode->pPrev || pNewNode->pPrev == NULL));

  if ((pNewNode) && (pOldNode)) {
    DLL_NodeUnlist(pNewNode);

    pOldNode->pPrev->pNext = pNewNode;
    pNewNode->pPrev = pOldNode->pPrev;
    pNewNode->pNext = pOldNode;
    pOldNode->pPrev = pNewNode;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Inserts pNewNode after pOldNode.
void DLL_NodeInsertAfter(DLLNode *const pNewNode, DLLNode *const pOldNode) {
  LOG_ASSERT(pNewNode && (pNewNode == pNewNode->pNext || pNewNode->pNext == NULL) && (pNewNode == pNewNode->pPrev || pNewNode->pPrev == NULL));

  if ((pNewNode) && (pOldNode)) {
    DLL_NodeUnlist(pNewNode);

    pOldNode->pNext->pPrev = pNewNode;
    pNewNode->pPrev = pOldNode;
    pNewNode->pNext = pOldNode->pNext;
    pOldNode->pNext = pNewNode;
  }
}

// ////////////////////////////////////////////////////////////////////////////
/// Softs the list pList using the callback function CallbackPtr.
void DLL_SortList(DLL *const pList, DLL_NodeCompareCbType *const CallbackPtr,
                  void *const ParamPtr) {
  DLL tmpList;
  DLLNode *pNode;

  // Initialize the temporary list and append the list to the back of it.
  DLL_Init(&tmpList);
  DLL_AppendListToBack(&tmpList, pList);
  LOG_ASSERT(DLL_IsEmpty(pList));

  // Pop all elements from the temporary list and sorted insert them into the
  // list.
  do {
    pNode = DLL_PopBack(&tmpList);
    if (pNode) {
      DLL_SortedInsert(pList, pNode, CallbackPtr, ParamPtr);
    }
  } while (pNode);
}

} // extern "C"

#include "dl_list.hpp"

using namespace dll;

list::list() { DLL_Init(&dll); }
void list::push_back(DLLNode *const pNode) { DLL_PushBack(&dll, pNode); }
void list::push_front(DLLNode *const pNode) { DLL_PushFront(&dll, pNode); }
size_t list::length() const { return DLL_Size(&dll); }

DLLNode *list::begin() const { return DLL_Begin(&dll); }

DLLNode *list::end() const { return DLL_End(&dll); }

DLLNode *list::pop_front() { return DLL_PopFront(&dll); }

DLLNode *list::get_front() const { return DLL_GetFront(&dll); }

DLLNode *list::get_back() const { return DLL_GetBack(&dll); }

bool list::isEmpty() const { return DLL_IsEmpty(&dll); }

void list::unlist(DLLNode *pNode) { DLL_NodeUnlist(pNode); }

void list::for_each(DLL_ForEachNodeCbType cb, void *pUserData) {
  DLL_ForEachNode(&dll, cb, pUserData);
}

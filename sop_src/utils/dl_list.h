#ifndef __DL_LIST_H__
#define __DL_LIST_H__
/**
  @file     dl_list.h
  @brief    Doubly-linked list with C API.
            The difference between this list and a "std::list" list element, and
            the entire reason for this file to "be" is that it adds the possibility
            to make any object a list node by adding DL_LIST_NODE_MEMBERS to the front
            of its struct or including DLLNode at the front of its struct.
*/

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
struct DLLNodeTag;

/// [Inheritance] Inheritable members
#define DL_LIST_NODE_MEMBERS  \
struct DLLNodeTag *pNext;     \
struct DLLNodeTag *pPrev;

typedef struct DLLNodeTag { DL_LIST_NODE_MEMBERS } DLLNode;

typedef struct DLLTag { DLLNode dummyNode; } DLL;

///////////////////////////////////////////////////////////////////////////
/// Get a pointer to the top of an object given the pointer to a list
/// node within the object.
///
/// @param nodePtr - Pointer to the list node within the struct
/// @param structContainingNodeType - The name of the struct that contains the
/// node
/// @param nodeName - The name of the node within the struct
///
/// Node: Include <stddef.h> from your C file to use this macro!
///
///////////////////////////////////////////////////////////////////////////
#define DLL_GetObjectTop(nodePtr, structContainingNodeType, nodeName)          \
  ((structContainingNodeType *)&(                                              \
      (((unsigned char *)                                                      \
            nodePtr)[0 - offsetof(structContainingNodeType, nodeName)])))

///////////////////////////////////////////////////////////////////////////
/// Quick node unlist with no error checking
///
/// @param pN - Pointer to the list node within the struct
///
///////////////////////////////////////////////////////////////////////////
#define DLL_NodeUnlistFast(pN)                                                 \
{                                                                              \
    DLLNode *const pNode = (DLLNode *)(pN);                                    \
    if ((pNode) && (pNode->pPrev)) {                                           \
      pNode->pPrev->pNext = pNode->pNext;                                      \
      pNode->pNext->pPrev = pNode->pPrev;                                      \
      pNode->pPrev = pNode->pNext = NULL;                                      \
    }                                                                          \
}

///////////////////////////////////////////////////////////////////////////
/// Inline pushback function with no error checking
///
/// @param pD - Pointer to the doubly-linked-list push the element onto
/// @param pN - Pointer to the list node within the struct
///
///////////////////////////////////////////////////////////////////////////
#define DLL_PushBackFast(pD, pN)                                               \
  \
{                                                                         \
    DLL *const pList = (DLL *)(pD);                                            \
    DLLNode *const pNode = (DLLNode *)(pN);                                    \
    pList->dummyNode.pPrev->pNext = pNode;                                     \
    pNode->pPrev = pList->dummyNode.pPrev;                                     \
    pNode->pNext = &pList->dummyNode;                                          \
    pList->dummyNode.pPrev = pNode;                                            \
  \
}

///////////////////////////////////////////////////////////////////////////
/// Inline IsEmpty() query function with no error checking
///
/// @param DBLLISTPTR - Pointer to the doubly-linked-list to query for empty
/// status
///
///////////////////////////////////////////////////////////////////////////
#define DLL_IsEmptyFast(DBLLISTPTR)                                            \
  (((DBLLISTPTR)->dummyNode.pNext == &(DBLLISTPTR)->dummyNode) ? true : false)

///////////////////////////////////////////////////////////////////////////
/// Inline DLL_Begin() list iteration function with no error checking.
///  This is used when starting a new list iteration, to get a non-NULL
///  pointer into the list.  Equivalent to list.begin() in c++.
///
/// @param DBLLISTPTR - Pointer to the doubly-linked-list to get the first
/// element for.
/// \return Will never return NULL.  Returns either a pointer to the first
///    node in the list or will return a pointer to the list itself.
///
///////////////////////////////////////////////////////////////////////////
#define DLL_BeginFast(DBLLISTPTR) ((DBLLISTPTR)->dummyNode.pNext)

///////////////////////////////////////////////////////////////////////////
/// Inline DLL_EndFast() list iteration function with no error checking.
///  This is used when starting a new list iteration, to get a non-NULL
///  pointer into the list.  Equivalent to list.end() in c++.
///
/// @param DBLLISTPTR - Pointer to the doubly-linked-list to get the first
/// element for.
/// \return Will never return NULL.  Returns a pointer to the list.
///
///////////////////////////////////////////////////////////////////////////
#define DLL_EndFast(DBLLISTPTR) (&(DBLLISTPTR)->dummyNode)

///////////////////////////////////////////////////////////////////////////
/// Callback type.  Used when calling a callback for every node in a list.
///
/// @param ParamPtr - User-defined parameter, passed into the
/// DLL_ForEachNode() function.
///
/// @param pNode  - pointer to the node in the list.
///
/// \return: Callback should return true if the foreach loop should continue,
///     otherwise, it should return false.
///
///////////////////////////////////////////////////////////////////////////
typedef bool DLL_ForEachNodeCbType(void *ParamPtr, DLLNode *pNode);

///////////////////////////////////////////////////////////////////////////
/// Called for sorted inserts - compares two nodes.
/// The callback should compare the two nodes with each other
/// and return:
/// - A negative number if the value associated with Node0Ptr is greater than
///   the value associated with Node1Ptr.
/// - A positive number if the value associated with Node0Ptr is less than
///   the value associated with Node1Ptr.
/// - Zero if the value associated with Node0Ptr is the same
///   the value associated with Node1Ptr.
///
/// @param ParamPtr - User-defined parameter
///
/// @param Node0Ptr  - pointer to the first node.
///
/// @param Node1Ptr  - pointer to the second node.
///
/// \return:
///     >   0 if (Node0Ptr > Node1Ptr)
///     ==  0 if (Node0Ptr == Node1Ptr)
///     <   0 if (Node0Ptr < Node1Ptr)
///
///////////////////////////////////////////////////////////////////////////
typedef int DLL_NodeCompareCbType(void *const ParamPtr,
                                  const DLLNode *const Node0Ptr,
                                  const DLLNode *const Node1Ptr);

// Initialize a doubly-linked list object
void DLL_Init(DLL *const pList);

// Initialize a node before placing in a doubly-linked list
void DLL_NodeInit(DLLNode *const pNode);

// Insert a node on the back of the list
void DLL_PushBack(DLL *const pList, DLLNode *const pNode);

// Insert a node on the front of the list
void DLL_PushFront(DLL *const pList, DLLNode *const pNode);

// Check if a list is empty.
bool DLL_IsEmpty(const DLL *const pList);

// Gets the first node on the list, or NULL if the list is empty.
DLLNode *DLL_GetFront(const DLL *const pList);

// Gets the last node on the list, or NULL if the list is empty.
DLLNode *DLL_GetBack(const DLL *const pList);

// Gets the node at the specified index, or NULL if outside range of the list.
DLLNode *DLL_GetNodeAtIdx(const DLL *const pList, const int idx);

// Gets the front of the list, or pList if the list is empty
// (Use for iterating over the list.)
DLLNode *DLL_Begin(const DLL *const pList);

// Gets pList
// (Use for iterating over the list.)
DLLNode *DLL_End(const DLL *const pList);

// Pops the front node from the list, or returns NULL if the list is empty.
DLLNode *DLL_PopFront(DLL *const pList);

// Pops the back node fron the list, or returns NULL if the list is empty.
DLLNode *DLL_PopBack(DLL *const pList);

// Appends the list in DllToAppendPtr to the back of pList
void DLL_AppendListToBack(DLL *const pList, DLL *const pDllToAppend);

// Returns the size of the list in pList
unsigned int DLL_Size(const DLL *const pList);

// Returns the size of the list in pList as an integer
int DLL_SizeN(const DLL *const pList);

// Is the node listed?
bool DLL_IsListed(const DLL *const pList, const DLLNode *const pNode);

// Iterates over the list in pList, calling CallbackPtr for each entry in the
// list.
void DLL_ForEachNode(DLL *const pList, DLL_ForEachNodeCbType *const pCb,
                     void *const pParam);

// Inserts the node in pNode in the list in pList.  Uses CallbackPtr to
// compare list nodes.
bool DLL_SortedInsert(DLL *const pList, DLLNode *const pNode,
                      DLL_NodeCompareCbType *const pCb, void *const pParam);

// Inserts the node in pNode in the list in pList.  Uses CallbackPtr to
// compare list nodes.
void DLL_SortedInsertFromBack(DLL *const pList, DLLNode *const pNode,
                              DLL_NodeCompareCbType *const pCb,
                              void *const pParam);

// Clear a list by popping all entries.
void DLL_PopAll(DLL *const pList);

// Unlists the node in pNode from the list it belongs to.
void DLL_NodeUnlist(DLLNode *const pNode);

// Returns true if the node in pNode is listed, otherwise returns false.
bool DLL_NodeIsListed(const DLLNode *const pNode);

// Inserts the node in pNewNode before the listed node pOldNode.
void DLL_NodeInsertBefore(DLLNode *const pNewNode, DLLNode *const pOldNode);

// Inserts the node in pNewNode after the listed node pOldNode.
void DLL_NodeInsertAfter(DLLNode *const pNewNode, DLLNode *const pOldNode);

void DLL_SortList(DLL *const pList, DLL_NodeCompareCbType *const CallbackPtr,
                  void *const ParamPtr);

#ifdef __cplusplus
}
#endif

#endif // __DL_LIST_H__

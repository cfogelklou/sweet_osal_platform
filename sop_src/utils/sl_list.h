#ifndef __SL_LIST_H__
#define __SL_LIST_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
struct SLLNodeTag;

/// [Inheritance] Inheritable members
#define SL_LIST_NODE_MEMBERS \
  struct SLLNodeTag *pNext;

typedef struct SLLNodeTag { SL_LIST_NODE_MEMBERS } SLLNode;

typedef struct SLLTag {
  SLLNode dummyNode;
  SLLNode *pLast;
} SLL;

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
#define SLL_GetObjectTop(nodePtr, structContainingNodeType, nodeName)          \
  ((structContainingNodeType *)&(                                              \
      (((unsigned char *)                                                      \
            nodePtr)[0 - offsetof(structContainingNodeType, nodeName)])))

///////////////////////////////////////////////////////////////////////////
/// Inline IsEmpty() query function with no error checking
///
/// @param SLLPTR - Pointer to the doubly-linked-list to query for empty
/// status
///
///////////////////////////////////////////////////////////////////////////
#define SLL_IsEmptyFast(SLLPTR)                                                \
  (((SLLPTR)->dummyNode.pNext == &(SLLPTR)->dummyNode) ? true : false)

///////////////////////////////////////////////////////////////////////////
/// Inline SLL_Begin() list iteration function with no error checking.
///  This is used when starting a new list iteration, to get a non-NULL
///  pointer into the list.  Equivalent to list.begin() in c++.
///
/// @param SLLPTR - Pointer to the doubly-linked-list to get the first
/// element for.
/// \return Will never return NULL.  Returns either a pointer to the first
///    node in the list or will return a pointer to the list itself.
///
///////////////////////////////////////////////////////////////////////////
#define SLL_BeginFast(SLLPTR) ((SLLPTR)->dummyNode.pNext)

///////////////////////////////////////////////////////////////////////////
/// Inline SLL_EndFast() list iteration function with no error checking.
///  This is used when starting a new list iteration, to get a non-NULL
///  pointer into the list.  Equivalent to list.end() in c++.
///
/// @param SLLPTR - Pointer to the doubly-linked-list to get the first
/// element for.
/// \return Will never return NULL.  Returns a pointer to the list.
///
///////////////////////////////////////////////////////////////////////////
#define SLL_EndFast(SLLPTR) (&(SLLPTR)->dummyNode)

///////////////////////////////////////////////////////////////////////////
/// Callback type.  Used when calling a callback for every node in a list.
///
/// @param ParamPtr - User-defined parameter, passed into the
/// SLL_ForEachNode() function.
///
/// @param pNode  - pointer to the node in the list.
///
/// \return: Callback should return true if the foreach loop should continue,
///     otherwise, it should return false.
///
///////////////////////////////////////////////////////////////////////////
typedef bool SLL_ForEachNodeCbType(void *ParamPtr, SLLNode *pNode);

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
typedef int SLL_NodeCompareCbFnT(void *ParamPtr, const SLLNode *const Node0Ptr,
                                 const SLLNode *const Node1Ptr);

// Initialize a doubly-linked list object
void SLL_Init(SLL *const pList);

// Initialize a node before placing in a doubly-linked list
void SLL_NodeInit(SLLNode *const pNode);

// Insert a node on the back of the list
void SLL_PushBack(SLL *const pList, SLLNode *const pNode);

// Insert a node on the front of the list
void SLL_PushFront(SLL *const pList, SLLNode *const pNode);

// Check if a list is empty.
bool SLL_IsEmpty(const SLL *const pList);

// Gets the first node on the list, or NULL if the list is empty.
SLLNode *SLL_GetFront(const SLL *const pList);

// Gets the last node on the list, or NULL if the list is empty.
SLLNode *SLL_GetBack(const SLL *const pList);

// Gets the node at the specified index, or NULL if outside range of the list.
SLLNode *SLL_GetNodeAtIdx(const SLL *const pList, const int idx);

// Gets the front of the list, or pList if the list is empty
// (Use for iterating over the list.)
SLLNode *SLL_Begin(const SLL *const pList);

// Gets pList
// (Use for iterating over the list.)
SLLNode *SLL_End(const SLL *const pList);

// Pops the front node from the list, or returns NULL if the list is empty.
SLLNode *SLL_PopFront(SLL *const pList);

// Pops the back node from the list, or returns NULL if the list is empty.
SLLNode *SLL_PopBack(SLL *const pList);

// Appends the list in pSllToAppend to the back of pList
void SLL_AppendListToBack(SLL *const pList, SLL *const pSllToAppend);

// Returns the size of the list in pList
unsigned int SLL_Size(const SLL *const pList);

// Is the node listed?
bool SLL_IsListed(const SLL *const pList, const SLLNode *const pNode);

// Iterates over the list in pList, calling CallbackPtr for each entry in the
// list.
void SLL_ForEachNode(SLL *const pList, SLL_ForEachNodeCbType *const pCb,
                     void *const pParam);

// Inserts the node in pNode in the list in pList.  Uses CallbackPtr to
// compare list nodes.
void SLL_SortedInsert(SLL *const pList, SLLNode *const pNode,
                      SLL_NodeCompareCbFnT *const pCb, void *const pParam);

// Unlists the node in pNode from the list it belongs to.
bool SLL_NodeUnlistQuick(SLL *const pList, SLLNode *const pNodeBefore,
                         SLLNode *const pNodeToUnlist);

// Unlists the node in pNode from the list it belongs to.
bool SLL_NodeUnlist(SLL *const pList, SLLNode *const pNode);

// Clear a list by popping all entries.
void SLL_PopAll(SLL *const pList);

// Inserts the node in pNewNode after the listed node pOldNode.
void SLL_NodeInsertAfter(SLLNode *const pNewNode, SLLNode *const pOldNode);

void SLL_SortList(SLL *const pList, SLL_NodeCompareCbFnT *const CallbackPtr,
                  void *const ParamPtr);

#ifdef __cplusplus
}
#endif

#endif // __SL_LIST_H__

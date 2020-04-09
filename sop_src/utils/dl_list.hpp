#ifndef BLE_DL_LIST_HPP
#define BLE_DL_LIST_HPP

#ifdef __cplusplus
#include "dl_list.h"
#include <stdlib.h> // for size_t

namespace dll {

class list {
public:
  DLL dll;
  list();
  void push_back(DLLNode *const pNode);
  void push_front(DLLNode *const pNode);
  size_t length() const;

  DLLNode *begin() const;

  DLLNode *end() const;

  DLLNode *pop_front();

  DLLNode *get_front() const;

  DLLNode *get_back() const;

  bool isEmpty() const;

  void unlist( DLLNode * pNode );

  void sorted_insert(
    DLLNode &nNode, 
    DLL_NodeCompareCbType* const pCb, 
    void* const pParam = nullptr);

  void for_each( DLL_ForEachNodeCbType cb, void *pUserData );
};

} // namespace

#endif // __cplusplus

#endif

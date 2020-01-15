#ifndef BLE_SL_LIST_HPP
#define BLE_SL_LIST_HPP

#ifdef __cplusplus
#include "sl_list.h"

namespace sll {

class list {
public:
  SLL sll;
  list();
  void push_back(SLLNode *const pNode);
  void push_front(SLLNode *const pNode);

  SLLNode *begin();

  SLLNode *end();

  SLLNode *pop_front();

  SLLNode *get_front();

  bool isEmpty();

  bool unlist( SLLNode * pNode );

  void for_each( SLL_ForEachNodeCbType cb, void *pUserData );

  void for_each_pop(SLL_ForEachNodeCbType cb, void *pUserData);
  
  int count();
};

} // namespace

#endif // __cplusplus

#endif

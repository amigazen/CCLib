#ifndef STDLIST_H
#define STDLIST_H 1

/*
 * stdlist - doubly-linked list helpers used by CClib.
 * _node / _list types live in <libraries/cclib.h> (CCLIB_STDLIST_TYPES).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef LIBRARIES_CCLIB_H
#include <libraries/cclib.h>
#endif

#ifdef ANSIC
_node *inst_node(long);
void put_head(_list *,_node *);
void *add_head(_list *,long);
void put_tail(_list *,_node *);
void *add_tail(_list *,long);
void *remv_head(_list *, void **);
void *remv_tail(_list *, void **);
void remv_list(_list *);
void *r_node(_list *,_node *,void **);
void *remv_node(_list *,void *);
#else
_node *inst_node();
void put_head();
void *add_head();
void put_tail();
void *add_tail();
void *remv_head();
void *remv_tail();
void remv_list();
void *r_node();
void *remv_node();
#endif

#endif

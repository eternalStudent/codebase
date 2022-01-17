#define LINKEDLIST_ADD(list, n)				do{if ((list)->last) (list)->last->next=(n);\
											else (list)->first=n;\
											(n)->prev=(list)->last;\
											(list)->last=(n);}while(0)

#define LINKEDLIST_REMOVE(list, n)			do{if ((n)->prev) (n)->prev->next = (n)->next;\
											else (list)->first = (n)->next;\
											if ((n)->next) (n)->next->prev = (n)->prev;\
											else (list)->last = (n)->prev;}while(0)

#define LINKEDLIST_FOREACH(list, t, child)	for(t* child = (list)->first; child != NULL; child = child->next)
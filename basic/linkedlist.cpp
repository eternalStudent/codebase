#define LINKEDLIST_ADD(list, n)				do{if ((list)->last) (list)->last->next=(n);\
											else (list)->first=n;\
											(n)->prev=(list)->last;\
											(list)->last=(n);}while(0)

#define LINKEDLIST_ADD_AFTER(list, m, n) 	do{(n)->prev=(m);(n)->next=(m)->next;\
											if ((m)->next){(m)->next->prev=(n);}\
											else{(list)->last=(n);}\
											(m)->next=(n);}while(0)

#define LINKEDLIST_ADD_TO_START(list, n)	do{(n)->next=(list)->first;\
											if ((list)->first){(list)->first->prev=(n);}\
											else {(list)->last=(n);}\
											(list)->first=(n);}while(0)

#define LINKEDLIST_REMOVE(list, n)			do{if ((n)->prev) (n)->prev->next = (n)->next;\
											else (list)->first = (n)->next;\
											if ((n)->next) (n)->next->prev = (n)->prev;\
											else (list)->last = (n)->prev;}while(0)

#define LINKEDLIST_FOREACH(list, t, n)		for(t* n = (list)->first; n != NULL; n = n->next)
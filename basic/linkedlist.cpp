#define LINKEDLIST_ADD(list, n)				do{if ((list)->last) (list)->last->next=(n);\
											else (list)->first=n;\
											(n)->prev=(list)->last;\
											(list)->last=(n);}while(0)

#define LINKEDLIST_ADD_AFTER(m, n) 			do{(n)->prev=(m);(n)->next=(m)->next;\
											if ((m)->next){(m)->next->prev=(n)};\
											(m)->next=(n);}while(0)

#define LINKEDLIST_REMOVE(list, n)			do{if ((n)->prev) (n)->prev->next = (n)->next;\
											else (list)->first = (n)->next;\
											if ((n)->next) (n)->next->prev = (n)->prev;\
											else (list)->last = (n)->prev;}while(0)

#define LINKEDLIST_FOREACH(list, t, n)		for(t* n = (list)->first; n != NULL; n = n->next)
#define sizeofitem(array)				sizeof(*(array)->table)

#define Reserve(array, new_capacity)	do{if ((array)->table == NULL || (array)->capacity<(new_capacity)){ \
										(array)->data = ReAlloc((array)->table, (array)->capacity*sizeofitem(array), (new_capacity)*sizeofitem(array), 1); \
										(array)->capacity = (new_capacity);}}while(0)

#define Add(array, item)				do{if ((array)->capacity == (array)->size) {Reserve(array, ((array)->size ? (array)->size * 2 : 4));} \
										(array)->table[(array)->size] = item; (array)->size++;}while(0)

#define AddN(array, n, items)			do{if ((array)->capacity < (array)->size + (n)) Reserve(array, (array)->size + (n));\
										memcpy(&((array)->table[(array)->size]), &(items[0]), ((n)*sizeofitem(array))); (array)->size+=(n);}while(0)

#define UnorderedRemove(array, i)		do{(array)->table[i]=(array)->table[(array)->size-1]; (array)->size--;}while(0)

#define Pop(array)						((array)->size--, (array)->table[(array)->size])

#define Clear(array)					(array)->size=0

#define QAt(array, i)					((array)->table[((array)->front+(i)) % (array)->capacity])

#define QReserve(array, new_capacity)	do{if ((array)->table == NULL || (array)->capacity<(new_capacity)){ \
										(array)->data = ReAlloc((array)->table, (array)->capacity*sizeofitem(array), (new_capacity)*sizeofitem(array)); \
										if ((array)->rear < (array)->front) {\
										memmove(&((array)->table[(array)->capacity]), &((array)->table[0]), (array)->rear*sizeofitem(array));\
										(array)->rear += (array)->capacity;}\
										(array)->capacity = (new_capacity);}}while(0)

#define Enqueue(array, item)			do{if ((array)->capacity == (array)->size) \
										{QReserve(array, ((array)->size ? (array)->size * 2 : 4));} \
										(array)->table[(array)->rear] = item; (array)->size++;\
										(array)->rear = ((array)->rear == (array)->capacity-1 && ((array)->size!=(array)->capacity)) ? 0 : (array)->rear + 1; \
										}while(0)

#define Dequeue(array)					((array)->rear = ((array)->rear==(array)->capacity) ? 0 : (array)->rear,\
										(array)->front = ((array)->front == (array)->capacity-1) ? 0 : (array)->front + 1,\
										(array)->size--, QAt(array, (array)->capacity-1))

#define QClear(array)					do{(array)->size = 0;(array)->rear = 0;(array)->front = 0;}while(0)

// [5,6,7, , , , , , , , ,0,1,2,3,4]
//        ^               ^         ^
//      rear            front     capacity
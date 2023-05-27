#define sizeofitem(array)					sizeof(*(array)->table)

#define ARRAY_RESERVE(array, new_capacity)	do{if ((array)->table == NULL || (array)->capacity<(new_capacity)){ \
											(array)->data = ArenaReAlloc((array)->arena, (array)->table, (array)->capacity*sizeofitem(array), (new_capacity)*sizeofitem(array)); \
											(array)->capacity = (new_capacity);}}while(0)

#define ARRAY_ADD(array, item)				do{if ((array)->capacity == (array)->count) {ARRAY_RESERVE(array, ((array)->count ? (array)->count * 2 : 4));} \
											(array)->table[(array)->count] = item; (array)->count++;}while(0)

#define ARRAY_ADD_N(array, items, n)		do{if ((array)->capacity < (array)->count + (n)) ARRAY_RESERVE(array, (array)->count + (n));\
											memcpy(&((array)->table[(array)->count]), &(items[0]), ((n)*sizeofitem(array)));\
											(array)->count+=(n);}while(0)

#define UnorderedRemove(array, i)			do{(array)->table[i]=(array)->table[(array)->count-1]; (array)->count--;}while(0)

#define ARRAY_POP(array)					((array)->count--, (array)->table[(array)->count])

#define ARRAY_CLEAR(array)					(array)->count=0


/*-----------------------------------
 *    Queue
 *-----------------------------------
 *
 *  [5,6,7, , , , , , , , ,0,1,2,3,4]
 *         ^               ^         ^
 *       rear            front     capacity
 */


#define QAt(array, i)					((array)->table[((array)->front+(i)) % (array)->capacity])

#define QReserve(array, new_capacity)	do{if ((array)->table == NULL || (array)->capacity<(new_capacity)){ \
										(array)->data = ReAlloc((array)->table, (array)->capacity*sizeofitem(array), (new_capacity)*sizeofitem(array)); \
										if ((array)->rear < (array)->front) {\
										memmove(&((array)->table[(array)->capacity]), &((array)->table[0]), (array)->rear*sizeofitem(array));\
										(array)->rear += (array)->capacity;}\
										(array)->capacity = (new_capacity);}}while(0)

#define Enqueue(array, item)			do{if ((array)->capacity == (array)->count) \
										{QReserve(array, ((array)->count ? (array)->count * 2 : 4));} \
										(array)->table[(array)->rear] = item; (array)->count++;\
										(array)->rear = ((array)->rear == (array)->capacity-1 && ((array)->count!=(array)->capacity)) ? 0 : (array)->rear + 1; \
										}while(0)

#define Dequeue(array)					((array)->rear = ((array)->rear==(array)->capacity) ? 0 : (array)->rear,\
										(array)->front = ((array)->front == (array)->capacity-1) ? 0 : (array)->front + 1,\
										(array)->count--, QAt(array, (array)->capacity-1))

#define QClear(array)					do{(array)->count = 0;(array)->rear = 0;(array)->front = 0;}while(0)

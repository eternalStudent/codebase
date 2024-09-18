StringList CreateStringList(String string, FixedSize* allocator) {
	StringNode* node = (StringNode*)FixedSizeAlloc(allocator);
	node->string = string;
	return {node, node, string.length};
}

void DestroyStringList(StringList* list, FixedSize* allocator) {
	StringNode* node = list->first;
	while (node) {
		StringNode* next = node->next;
		FixedSizeFree(allocator, node);
		node = next;
	}
	list->first = NULL;
	list->last = NULL;
}

void StringListAppend(StringList* list, StringNode* node) {
	if (node->string.length == 0) return;
	LINKEDLIST_ADD(list, node);
	list->length += node->string.length;
}

ssize StringListCopy(StringList list, BigBuffer* buffer) {
	BigBufferEnsureCapacity(buffer, list.length);
	return StringListCopy(list, buffer->pos);
}

String StringListToString(StringList list, BigBuffer* buffer) {
	if (list.first == list.last)
		return list.first->string;

	byte* data = buffer->pos;
	ssize length = StringListCopy(list, buffer);
	buffer->pos += length;
	return {data, length};
}

String StringListToTempString(StringList list, BigBuffer* buffer) {
	if (list.first == list.last)
		return list.first->string;

	return {buffer->pos, StringListCopy(list, buffer)};
}

struct StringListPos {
	StringNode* node;
	ssize index;
};

#define SL_START(list)  StringListPos{list.first, 0}
#define SL_END(list)  	StringListPos{list.last, list.last ? list.last->string.length : 0}
#define SL_AT(pos)		((pos).node->string.data[(pos).index])

ssize StringListCopy(StringListPos start, StringListPos end, byte* buffer) {
	byte* ptr = buffer;
	for (StringNode* node = start.node; node != NULL; node = node->next) {
		String string = node->string;
		ssize startIndex = node == start.node ? start.index : 0;
		ssize endIndex = node == end.node ? end.index : string.length;
		ptr += StringCopy({string.data + startIndex, endIndex - startIndex}, ptr);

		if (node == end.node) break;
	}
	return ptr - buffer;
}

bool StringListIsStart(StringListPos pos) {
	return pos.node == NULL || pos.node->prev == NULL && pos.index == 0;
}

bool StringListIsEnd(StringListPos pos) {
	return pos.node == NULL || pos.node->next == NULL && pos.index == pos.node->string.length;
}

StringListPos StringListPosInc(StringListPos pos) {
	if (pos.index == pos.node->string.length && pos.node->next != NULL) {
		pos.index = 0;
		pos.node = pos.node->next;
	}

	if (pos.index < pos.node->string.length - 1 ||
		 pos.index == pos.node->string.length - 1 && pos.node->next == NULL)

		pos.index++;
	else {
		pos.node = pos.node->next;
		pos.index = 0;
	}
	return pos;
}

void StringListPosInc(StringListPos* pos) {
	if (pos->index == pos->node->string.length && pos->node->next != NULL) {
		pos->index = 0;
		pos->node = pos->node->next;
	}

	if (pos->index < pos->node->string.length - 1 ||
		 pos->index == pos->node->string.length - 1 && pos->node->next == NULL)

		pos->index++;
	else {
		pos->node = pos->node->next;
		pos->index = 0;
	}
}

StringListPos StringListPosDec(StringListPos pos) {
	if (pos.index > 0 ||
		 pos.index == 0 && pos.node->prev == NULL)

		pos.index--;
	else {
		pos.node = pos.node->prev;
		pos.index = pos.node->string.length - 1;
	}
	return pos;
}

void StringListPosDec(StringListPos* pos) {
	if (pos->index > 0 ||
		 pos->index == 0 && pos->node->prev == NULL)
		pos->index--;
	else {
		pos->node = pos->node->prev;
		pos->index = pos->node->string.length - 1;
	}
}

// NOTE: not the most efficient
bool StringListEquals(StringList a, StringList b) {
	if (a.length != b.length) return false;

	StringListPos pos0 = SL_START(a);
	StringListPos pos1 = SL_START(b);	
	for (ssize i = 0; i < a.length; i++) {
		if (SL_AT(pos0) != SL_AT(pos1))
			return false;

		StringListPosInc(&pos0);
		StringListPosInc(&pos1);
	}
	return true;
}

// NOTE: not the most efficient
bool StringListEquals(StringList a, String b) {
	if (a.length != b.length) return false;

	if (a.first == a.last) 
		return StringEquals(a.first->string, b);

	StringListPos pos0 = SL_START(a);
	byte* pos1 = b.data;	
	for (ssize i = 0; i < a.length; i++) {
		if (SL_AT(pos0) != *pos1)
			return false;

		StringListPosInc(&pos0);
		pos1++;
	}
	return true;
}

// NOTE: not the most efficient
bool StringListStartsWith(StringList all, StringList start) {
	if (start.length == 0) return true;
	if (all.length < start.length) return false;

	StringListPos pos0 = SL_START(all);
	StringListPos pos1 = SL_START(start);	
	for (ssize i = 0; i < start.length; i++) {
		if (SL_AT(pos0) != SL_AT(pos1))
			return false;
		
		StringListPosInc(&pos0);
		StringListPosInc(&pos1);
	}
	return true;
}

bool StringListPosEquals(StringListPos a, StringListPos b) {
	return a.node == b.node && a.index == b.index;
}

// NOTE: returns true if and only if a < b
bool StringListPosCompare(StringListPos a, StringListPos b) {
	if (a.node == b.node)
		return a.index < b.index;

	for (StringNode* node = a.node; node != NULL; node = node->next) {
		if (node == b.node) return true;
	}

	return false;
}

StringListPos StringListGetFirstPosOf(StringListPos start, StringListPos end, byte b) {
	for (StringNode* node = start.node; node != NULL; node = node->next) {
		String string = node->string;
		
		ssize startIndex = node == start.node ? start.index : 0;
		ssize endIndex = node == end.node ? end.index : string.length;
		String substring = {string.data + startIndex, endIndex - startIndex};

		ssize index = StringGetFirstIndexOf(substring, b);
		if (index != string.length) return {node, startIndex + index};

		if (node == end.node) break;
	}

	return end;
}

StringListPos StringListGetLastPosOf(StringListPos start, StringListPos end, byte b) {
	for (StringNode* node = start.node; node != NULL; node = node->next) {
		String string = node->string;

		ssize startIndex = node == start.node ? start.index : 0;
		ssize endIndex = node == end.node ? end.index : string.length;
		String substring = {string.data + startIndex, endIndex - startIndex};
		
		ssize index = StringGetLastIndexOf(substring, b);
		if (index != -1) return {node, startIndex + index};

		if (node == end.node) break;
	}

	return StringListPosDec(start);
}

void StringListFindWord(StringList list, StringListPos pos, StringListPos* start, StringListPos* end) {
	*start = SL_START(list);
	bool afterPos = false;
	LINKEDLIST_FOREACH(&list, StringNode, node) {
		String string = node->string;
		for (ssize i = 0; i < string.length; i++) {
			byte b = string.data[i];
			bool alphaNumeric = 'a' <= b && b <= 'z' || 'A' <= b && b <= 'Z' || '0' <= b && b <= '9' || b == '_';
			if (pos.node == node && pos.index == i) {
				if (!alphaNumeric) {
					*start = {node, i};
					*end = {node, i + 1};
					return;
				}
				afterPos = true;
				continue;
			}

			if (!afterPos && !alphaNumeric) {
				*start = {node, i + 1};
			}
			if (afterPos && !alphaNumeric) {
				*end = {node, i};
				return;
			}
		}
	}
	*end = SL_END(list);
}

StringListPos StringListDelete(StringList* list, StringListPos tail, StringListPos head, 
		FixedSize* allocator, BigBuffer* buffer) {

	if (tail.node == head.node && tail.index == head.index) {
		if (StringListIsStart(head)) {
			return head;
		}
		if (head.index == head.node->string.length) {
			if (head.node->string.data + head.node->string.length == buffer->pos) 
				buffer->pos--;
			if (head.node->string.length == 1) {
				StringNode* next = head.node->next;
				StringNode* prev = head.node->prev;
				LINKEDLIST_REMOVE(list, head.node);
				FixedSizeFree(allocator, head.node);
				if (next || !prev) {
					head.node = next;
					head.index = 1;
				}
				else {
					head.node = prev;
					head.index = prev->string.length + 1;
				}
			}
			else {
				head.node->string.length--;
			}
		}
		else if (head.index == 0) {
			head.node = head.node->prev;
			head.index = head.node->string.length;

			if (head.node->string.data + head.node->string.length == buffer->pos) 
				buffer->pos--;
			if (head.node->string.length == 1) {
				StringNode* next = head.node->next;
				LINKEDLIST_REMOVE(list, head.node);
				FixedSizeFree(allocator, head.node);
				head.node = next;
				head.index = 1;
			}
			else {
				head.node->string.length--;
			}
		}
		else {
			ssize length1 = head.index - 1;
			ssize length2 = head.node->string.length - length1 - 1;

			if (length1 == 0) {
				head.node->string.data++;
				head.node->string.length--;
			}
			else {
				head.node->string.length = length1;
				StringNode* node2 = (StringNode*)FixedSizeAlloc(allocator);
				LINKEDLIST_ADD_AFTER(list, head.node, node2);
				node2->string.data = head.node->string.data + head.index;
				node2->string.length = length2;
			}
		}
		list->length--;
		StringListPosDec(&head);
	}
	else {
		StringListPos start, end;
		if (StringListPosCompare(tail, head)) {
			start = tail;
			end = head;
		}
		else {
			start = head;
			end = tail;
		}
		StringNode* next;
		for (StringNode* node = start.node; node != NULL; node = next) {
			next = node->next;
			String string = node->string;
			ssize startIndex = node == start.node ? start.index : 0;
			ssize endIndex = node == end.node ? end.index : string.length;

			if (endIndex == string.length && string.data + endIndex == buffer->pos) {
				buffer->pos -= endIndex - startIndex;
			}

			if (startIndex == 0 && endIndex == string.length) {
				LINKEDLIST_REMOVE(list, node);
				FixedSizeFree(allocator, node);
			}
			else if (startIndex == 0) {
				node->string.data += endIndex;
				node->string.length -= endIndex;
			}
			else if (endIndex == string.length) {
				node->string.length = startIndex;
			}
			else {
				node->string.length = startIndex;
				StringNode* node1 = (StringNode*)FixedSizeAlloc(allocator);
				node1->string.data = string.data + endIndex;
				node1->string.length = string.length - endIndex;
				LINKEDLIST_ADD_AFTER(list, node, node1);
			}

			list->length -= endIndex - startIndex;
			if (node == end.node) break;
		}

		head = start;
	}
	
	return head;
}

StringListPos StringListInsert(StringList* list, String newString, 
		StringListPos tail, StringListPos head, 
		FixedSize* allocator, BigBuffer* buffer) {

	if (!newString.length) 
		return head;

	StringCopy(newString, buffer);
	if (tail.node != head.node || tail.index != head.index) {
		// TODO: if the selected length is smaller or equal to newString length, insert it in place.
		head = StringListDelete(list, tail, head, allocator, buffer);
	}
	if (head.node == NULL) {
		StringNode* node = (StringNode*)FixedSizeAlloc(allocator);
		LINKEDLIST_ADD(list, node);
		node->string.data = buffer->pos;
		node->string.length = newString.length;
		head.node = node;
		head.index = 0;
	}
	else if (head.index == head.node->string.length) {
		if (head.node->string.data + head.node->string.length == buffer->pos) {
			head.node->string.length += newString.length;
		}
	 	else {
	 		StringNode* node = (StringNode*)FixedSizeAlloc(allocator);
	 		LINKEDLIST_ADD_AFTER(list, head.node, node);
	 		node->string.data = buffer->pos;
	 		node->string.length = newString.length;
	 		head.node = node;
	 		head.index = 0;
	 	}
	}
	else if (StringListIsStart(head)) {
		StringNode* node = (StringNode*)FixedSizeAlloc(allocator);
	 	LINKEDLIST_ADD_TO_START(list, node);
		node->string.data = buffer->pos;
		node->string.length = newString.length;
		head.node = node;
		head.index = 0;
	}
	else if (head.index == 0) {
		head.node = head.node->prev;
		head.index = head.node->string.length;

		String string = head.node->string;
		if (string.data + string.length == buffer->pos) {
			head.node->string.length += newString.length;
		}
		else {
			StringNode* node = (StringNode*)FixedSizeAlloc(allocator);
			LINKEDLIST_ADD_AFTER(list, head.node, node);
			node->string.data = buffer->pos;
			node->string.length = newString.length;
			head.node = node;
			head.index = 0;
		}
	}
	else {
		String string = head.node->string;
		ssize length1 = head.index;
		ssize length2 = string.length - length1;
		head.node->string.length = length1;
		StringNode* node1 = (StringNode*)FixedSizeAlloc(allocator);
		LINKEDLIST_ADD_AFTER(list, head.node, node1);
		node1->string.data = head.node->string.data + head.node->string.length;
		node1->string.length = length2;
		StringNode* node2 = (StringNode*)FixedSizeAlloc(allocator);
		LINKEDLIST_ADD_AFTER(list, head.node, node2);
		node2->string.data = buffer->pos;
		node2->string.length = newString.length;
		head.node = node2;
		head.index = 0;
	}

	buffer->pos += newString.length;
	list->length += newString.length;
	head.index += newString.length;
	return head;
}

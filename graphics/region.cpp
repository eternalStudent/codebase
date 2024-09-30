struct RegionNode {
	union {
		struct {int32 x, y, width, height;};
		struct {Point2i pos; Dimensions2i dim;};
	};
	RegionNode* parent;
	RegionNode* children[4];
	bool isTaken;

	// NOTE: one of its descendants is taken
	bool partiallyTaken;

	// NOTE: for debug
	Dimensions2i allocated;
};

RegionNode* RegionAlloc(RegionNode* node, FixedSize* nodeAllocator, Dimensions2i size) {

	if (node->isTaken)
		return NULL;

	// NOTE: cannot be split further
	if (node->width/2 < size.width || node->height/2 < size.height) {
		if (node->partiallyTaken)
			return NULL;

		node->isTaken = true;
		node->allocated = size;
		return node;
	}

	// NOTE: can be split further
	for (int32 i = 0; i < 4; i++) {
		RegionNode* child = node->children[i];

		if (!child) {
			child = (RegionNode*)FixedSizeAlloc(nodeAllocator);
			child->parent = node;
			child->dim = {node->width/2, node->height/2};
			child->x = node->x + (i&1)*child->width;
			child->y = node->y + ((i&2) >> 1)*child->height;
			node->children[i] = child;
		}

		RegionNode* descentant = RegionAlloc(child, nodeAllocator, size);
		if (descentant) {
			node->partiallyTaken = true;
			return descentant;
		}
	}

	// NOTE: none of the children have enough space
	return NULL;
}

void FreeSubtree(RegionNode* node, FixedSize* nodeAllocator) {
	if (!node) return;

	for (int32 i = 0; i < 4; i++) {
		FreeSubtree(node->children[i], nodeAllocator);
	}

	if (node->parent) for (int32 i = 0; i < 4; i++) {
		if (node->parent->children[i] == node) {
			node->parent->children[i] = NULL;
			break;
		}
	}

	FixedSizeFree(nodeAllocator, node);
}

void RegionFree(RegionNode* node, FixedSize* nodeAllocator = NULL) {
	RegionNode* parent = node->parent;

	// NOTE: not strictly necessary
	if (nodeAllocator)
		FreeSubtree(node, nodeAllocator);

	node = parent;
	while (node) {
		if (
			(!node->children[0] || (!node->children[0]->isTaken && !node->children[0]->partiallyTaken))
				&&
			(!node->children[1] || (!node->children[1]->isTaken && !node->children[1]->partiallyTaken))
				&&
			(!node->children[2] || (!node->children[2]->isTaken && !node->children[2]->partiallyTaken))
				&&
			(!node->children[3] || (!node->children[3]->isTaken && !node->children[3]->partiallyTaken))
		) {

			node->partiallyTaken = false;
			node = node->parent;
		}
		else {
			break;
		}
	}
}

RegionNode* RegionGetByPosAndDim(RegionNode* root, Point2i pos, Dimensions2i dim) {

	uint32 maxDim = MAX(dim.width, dim.height);
	if (!IsPowerOf2OrZero(maxDim)) {
		int64 bit = HighBit((uint32)maxDim, -1);
		maxDim = 1 << (bit + 1);
	}

	RegionNode* node = root;
	for (uint32 mask = root->width >> 1; mask >= maxDim; mask = mask >> 1) {
		int32 i = 0;
		if ((uint32)pos.x & mask) i += 1;
		if ((uint32)pos.y & mask) i += 2;
		RegionNode* child = node->children[i];
		if (!child) return NULL;
		node = child; 
	}

	return node;
}
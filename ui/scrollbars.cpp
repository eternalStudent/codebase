inline float32 GetThumbDim(float32 contentDim, float32 outerDim) {
	return outerDim*outerDim/contentDim;
}

inline float32 GetThumbOffset(float32 contentDim, float32 outerDim, float32 scrollOffset) {
	return scrollOffset*outerDim/contentDim;
}

inline void GetThumbDimOffset(float32 contentDim, float32 outerDim, float32 scrollOffset, float32* thumbDim, float32* thumbOffset) {
	float32 scale = outerDim/contentDim;
	*thumbOffset = scrollOffset*scale;
	*thumbDim = outerDim*scale;
}

inline float32 GetScrollOffset(float32 contentDim, float32 outerDim, float32 thumbOffset) {
	return thumbOffset*contentDim/outerDim;
}

inline float32 GetMaxScrollOffset(float32 contentDim, float32 outerDim) {
	return contentDim - outerDim;
}

inline float32 GetMaxThumbOffset(float32 contentDim, float32 outerDim) {
	return (contentDim - outerDim)*outerDim/contentDim;
}

Point2 ScrollToView(Box2 content, Box2 view) {
	float32 x = 0;
	float32 y = 0;
	if (content.x1 > view.x1) x = content.x1 - view.x1;
	if (content.x0 < view.x0) x = content.x0 - view.x0;
	if (content.y1 > view.y1) y = content.y1 - view.y1;
	if (content.y0 < view.y0) y = content.y0 - view.y0;
	return {x, y};
}
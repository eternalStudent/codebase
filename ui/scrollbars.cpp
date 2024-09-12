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
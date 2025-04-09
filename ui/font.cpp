struct TextMetrics {
	union {
		struct {float32 x, y;};
		Point2 pos;
	};

	float32 maxx;
	bool lastCharIsWhiteSpace;
	bool lastCharIsNewLine;
};

enum FontAlphaMode {
	FAM_Linear,
	FAM_GammaCorrected
};

#if FONT_BAKED
#  include "bakedfont.cpp"
#endif

#if FONT_CACHED
#  include "../graphics/region.cpp"
#  include "../basic/encoding.cpp"
#  include "cachedfont.cpp"
#endif
#if FONT_BAKED
#  include "bakedfont.cpp"
#endif

#if FONT_CACHED
#  include "../graphics/region.cpp"
#  include "../basic/encoding.cpp"
#  include "cachedfont.cpp"
#endif
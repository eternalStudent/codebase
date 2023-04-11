enum : byte {
	Icon_none,
	
	Icon_down_dir,
	Icon_up_dir,
	Icon_left_dir,
	Icon_right_dir,
	Icon_check,
	Icon_resize_small,
	Icon_resize_full,
	Icon_down_open,
	Icon_left_open,
	Icon_right_open,
	Icon_up_open,
	Icon_trash_empty,
	Icon_doc,
	Icon_ok,
	Icon_floppy,
	Icon_cancel,
	Icon_check_empty,
	Icon_resize_full_alt,
	Icon_docs,
	Icon_angle_left,
	Icon_angle_right,
	Icon_angle_up,
	Icon_angle_down,
	Icon_circle_empty,
	Icon_circle,
	Icon_minus_squared_alt,
	Icon_dot_circled,
	Icon_plus_squared_alt,
	Icon_trash,

	Icon_count
};

// TODO: load icons-font from resource
BakedFont LoadAndBakeIconsFont(Arena* arena, AtlasBitmap* atlas, float32 height) {
	File iconsFile = OSOpenFile(L"fontello.ttf");
	FontInfo iconsInfo = TTLoadFont(OSReadAll(iconsFile, arena).data);
	int32 codepoints[] = {
		0xe800, 0xe801, 0xe802, 0xe803, 0xe804, 0xe805, 
		0xe806, 0xe807, 0xe808, 0xe809, 0xe80a, 0xe80b,
		0xe80c, 0xe80d, 0xe80e, 0xe810, 0xf096, 0xf0b2,
		0xf0c5, 0xf104, 0xf105, 0xf106, 0xf107, 0xf10c,
		0xf111, 0xf147, 0xf192, 0xf196, 0xf1f8
	};
	return TTBakeFont(arena, iconsInfo, atlas, height, codepoints, sizeof(codepoints)/sizeof(int32));
}
struct Theme {
	union {
		Color colors[8];
		struct {
			Color background;
			Color background2;
			Color background3;
			Color dropShadow;
			Color border;
			Color text;
			Color text2;
			Color cursor;
			Color highlight;
		};
	};
};


Theme GetDarkTheme() {
	Theme theme;
	theme.background = HSL(210u, 10u, 22u);
	theme.background2 = HSL(220u, 10u, 33u);
	theme.background3 = HSL(210u, 10u, 42u);
	theme.dropShadow = sRGB(15, 15, 15);
	theme.border = HSL(210u, 10u, 58u);
	theme.text = HSL(210u, 15u, 73u);
	theme.text2 = HSL(0u, 20u, 73u);
	theme.cursor = sRGB(41, 41, 41);
	theme.highlight = HSL(220u, 10u, 22u);
	return theme;
}
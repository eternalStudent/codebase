struct UIColorPicker {
	union {
		struct {float32 x, y, width, height;};
		struct {Point2 pos; Dimensions2 dim;};
	};
	bool isVisible;
	UISliderMixer mixer;
	UIText text;
	float32 h, s, l;
	float32 hInvStep, sInvStep, lInvStep;

	// NOTE: extra allocated memory
	union {
		UITextBox boxes[6];
		struct {
			UITextBox hText;
			UITextBox sText;
			UITextBox lText;
			UITextBox rText;
			UITextBox gText;
			UITextBox bText;
		};
	};
	union {
		Box2 handles[2];
		struct {
			Box2 hHandls;
			Box2 slHandle;
		};
	};
	union {
		Box2 boundaries[2];
		struct {
			Box2 hBoundaries;
			Box2 slBoundaries;
		};
	};
	byte smallBuffer[24];
};

void UIColorPickerInit(UIColorPicker* picker, BakedFont* font, FixedSize* allocator, BigBuffer* bigBuffer) {
	memset(picker, 0, sizeof(UIColorPicker));
	picker->mixer.handles = picker->handles;
	picker->mixer.boundaries = picker->boundaries;
	picker->mixer.count = 2;
	picker->text.buffer = bigBuffer;
	picker->text.allocator = allocator;
	picker->text.boxes = picker->boxes;
	picker->text.count = 6;
	picker->text.pad = {2, 2};
	picker->text.font = font;
	picker->h = 0.5f;
	picker->s = 0.5f;
	picker->l = 0.5f;
	picker->isVisible = true;
	picker->hText.data = CreateStringList(STR("180"), allocator);
	picker->sText.data = CreateStringList(STR("50"), allocator);
	picker->lText.data = CreateStringList(STR("50"), allocator);
	picker->rText.data = CreateStringList(STR("64"), allocator);
	picker->gText.data = CreateStringList(STR("191"), allocator);
	picker->bText.data = CreateStringList(STR("191"), allocator);

	for (int32 i = 0; i < 6; i++) {
		picker->boxes[i].flags = UIText_DigitsOnly;
		
		// TODO: not sure that I should set this here
		picker->boxes[i].dim = {48, font->height + 4};
	}
}

void UIColorPickerSetText(UIColorPicker* picker, Color rgb) {
	ssize length;
	UIText* text = &picker->text;

	length = UnsignedToDecimal((int64)round(360*picker->h), picker->smallBuffer);
	UITextReplaceData(text, text->boxes + 0, {picker->smallBuffer, length});
	length = UnsignedToDecimal((int64)round(100*picker->s), picker->smallBuffer + 3);
	UITextReplaceData(text, text->boxes + 1, {picker->smallBuffer + 3, length});
	length = UnsignedToDecimal((int64)round(100*picker->l), picker->smallBuffer + 6);
	UITextReplaceData(text, text->boxes + 2, {picker->smallBuffer + 6, length});

	length = UnsignedToDecimal((int64)round(255*rgb.r), picker->smallBuffer + 9);
	UITextReplaceData(text, text->boxes + 3, {picker->smallBuffer + 9, length});
	length = UnsignedToDecimal((int64)round(255*rgb.g), picker->smallBuffer + 12);
	UITextReplaceData(text, text->boxes + 4, {picker->smallBuffer + 12, length});
	length = UnsignedToDecimal((int64)round(255*rgb.b), picker->smallBuffer + 15);
	UITextReplaceData(text, text->boxes + 5, {picker->smallBuffer + 15, length});
}

bool UIColorPickerProcessEvent(UIColorPicker* picker, OSEvent event) {
	if (!picker->isVisible)
		return false;

	UIText* text = &picker->text;

	if (UISliderMixerProcessEvent(&picker->mixer, event)) {
		picker->h = (picker->mixer.handles[0].y0 - picker->mixer.boundaries[0].y0)/picker->hInvStep;
		picker->s = (picker->mixer.handles[1].x0 - picker->mixer.boundaries[1].x0)/picker->sInvStep;
		picker->l = 1 - (picker->mixer.handles[1].y0 - picker->mixer.boundaries[1].y0)/picker->lInvStep;
		Color rgb = HSL(picker->h, picker->s, picker->l);

		UIColorPickerSetText(picker, rgb);

		return true;
	}

	if (UITextProcessEvent(text, event)) {
		if (text->active == &picker->hText
			 	|| 
			text->active == &picker->sText
			 	|| 
			text->active == &picker->lText) {

			picker->h = saturate(UITextGetUnsigned(&picker->text, &picker->hText)/360.f);
			picker->s = saturate(UITextGetUnsigned(&picker->text, &picker->sText)/100.f);
			picker->l = saturate(UITextGetUnsigned(&picker->text, &picker->lText)/100.f);
			Color rgb = HSL(picker->h, picker->s, picker->l);
			
			UIColorPickerSetText(picker, rgb);
		}

		if (text->active == &picker->rText
			 	|| 
			text->active == &picker->gText
			 	|| 
			text->active == &picker->bText) {

			float32 r = saturate(UITextGetUnsigned(&picker->text, &picker->rText)/255.f);
			float32 g = saturate(UITextGetUnsigned(&picker->text, &picker->gText)/255.f);
			float32 b = saturate(UITextGetUnsigned(&picker->text, &picker->bText)/255.f);
			Color rgb = {r, g, b, 1};
			ToHSL(rgb, &picker->h, &picker->s, &picker->l);

			UIColorPickerSetText(picker, rgb);
		}

		return true;
	}

	return false;
}
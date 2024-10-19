struct UIColorPicker {
	union {
		struct {float32 x, y, width, height;};
		struct {Point2 pos; Dimensions2 dim;};
	};
	bool isVisible;
	UISliderMixer mixer;
	UIText text;
	float32 h, s, l, a;
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
		Box2 handles[3];
		struct {
			Box2 hHandls;
			Box2 slHandle;
			Box2 aHandle;
		};
	};
	union {
		Box2 boundaries[3];
		struct {
			Box2 hBoundaries;
			Box2 slBoundaries;
			Box2 aBoundaries;
		};
	};
	byte smallBuffer[24];
};

void UIColorPickerInit(UIColorPicker* picker, 
#if FONT_CACHED
	ScaledFont* font,
#else
	BakedFont* font, 
#endif
	FixedSize* allocator, BigBuffer* bigBuffer) {
	memset(picker, 0, sizeof(UIColorPicker));
	picker->mixer.handles = picker->handles;
	picker->mixer.boundaries = picker->boundaries;
	picker->mixer.count = 3;
	picker->text.buffer = bigBuffer;
	picker->text.allocator = allocator;
	picker->text.boxes = picker->boxes;
	picker->text.count = 6;
	picker->text.pad = {2, 2};
	picker->text.font = font;
	picker->h = 0.5f;
	picker->s = 0.5f;
	picker->l = 0.5f;
	picker->a = 1.0f;
	picker->hText.data = CreateStringList(STR("180"), allocator);
	picker->sText.data = CreateStringList(STR("50"), allocator);
	picker->lText.data = CreateStringList(STR("50"), allocator);
	picker->rText.data = CreateStringList(STR("64"), allocator);
	picker->gText.data = CreateStringList(STR("191"), allocator);
	picker->bText.data = CreateStringList(STR("191"), allocator);

	for (int32 i = 0; i < 6; i++) {
		picker->boxes[i].flags = UIText_DigitsOnly;
		picker->boxes[i].height = font->height + 4;
	}
}

Color UIColorPickerGetColor(UIColorPicker* picker) {
	return HSL(picker->h, picker->s, picker->l, picker->a);
}

void UIColorPickerSetText(UIColorPicker* picker, UITextBox* box, float32 value, ssize bufferOffset) {
	UIText* text = &picker->text;
	if (text->active != box) {
		ssize length = UnsignedToDecimal((int64)round(value), picker->smallBuffer + bufferOffset);
		UITextReplaceData(text, box, {picker->smallBuffer + bufferOffset, length});
	}
}

void UIColorPickerSetText(UIColorPicker* picker, Color rgb) {
	UIColorPickerSetText(picker, &picker->hText, 360*picker->h, 0);
	UIColorPickerSetText(picker, &picker->sText, 100*picker->s, 4);
	UIColorPickerSetText(picker, &picker->lText, 100*picker->l, 8);

	UIColorPickerSetText(picker, &picker->rText, 255*rgb.r, 12);
	UIColorPickerSetText(picker, &picker->gText, 255*rgb.g, 16);
	UIColorPickerSetText(picker, &picker->bText, 255*rgb.b, 20);
}

void UIColorPickerSetHSL(UIColorPicker* picker, float32 h, float32 s, float32 l, float32 a = 1) {
	picker->h = saturate(h);
	picker->s = saturate(s);
	picker->l = saturate(l);
	picker->a = a;

	Color rgb = HSL(picker->h, picker->s, picker->l);
			
	UIColorPickerSetText(picker, rgb);
}

void UIColorPickerSetRGB(UIColorPicker* picker, Color rgb) {
	ToHSL(rgb, &picker->h, &picker->s, &picker->l);
	picker->a = rgb.a;

	UIColorPickerSetText(picker, rgb);
}

bool UIColorPickerProcessEvent(UIColorPicker* picker, OSEvent event) {
	if (!picker->isVisible)
		return false;

	UIText* text = &picker->text;

	if (UISliderMixerProcessEvent(&picker->mixer, event)) {
		picker->h = (picker->mixer.handles[0].y0 - picker->mixer.boundaries[0].y0)/picker->hInvStep;
		picker->s = (picker->mixer.handles[1].x0 - picker->mixer.boundaries[1].x0)/picker->sInvStep;
		picker->l = 1 - (picker->mixer.handles[1].y0 - picker->mixer.boundaries[1].y0)/picker->lInvStep;
		picker->a = (picker->mixer.handles[2].y0 - picker->mixer.boundaries[2].y0)/picker->hInvStep;
		Color rgb = HSL(picker->h, picker->s, picker->l);

		UIColorPickerSetText(picker, rgb);

		if (picker->mixer.active) {
			text->active = NULL;
		}

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
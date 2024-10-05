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
};

bool UIColorPickerProcessEvent(UIColorPicker* picker, OSEvent event) {
	if (!picker->isVisible)
		return false;

	if (UISliderMixerProcessEvent(&picker->mixer, event)) {
		picker->h = (picker->mixer.handles[0].y0 - picker->mixer.boundaries[0].y0)/picker->hInvStep;
		picker->s = (picker->mixer.handles[1].x0 - picker->mixer.boundaries[1].x0)/picker->sInvStep;
		picker->l = 1 - (picker->mixer.handles[1].y0 - picker->mixer.boundaries[1].y0)/picker->lInvStep;

		// TODO: update text

		return true;
	}

	/*
	TODO:
	if (UITextProcessEvent(&picker->text, event)) {
		// TODO: update hsl, mixer

		return true;
	}
	*/

	return false;
}
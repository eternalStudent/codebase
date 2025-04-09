struct OpenGLQuad {
	Point2 pos0;
	Point2 pos1;
	Color color00;
	Color color01;
	Color color10;
	Color color11;
	float32 cornerRadius;
	float32 borderThickness;
	Color borderColor;
	Box2 crop;
	float32 blur;
};

struct OpenGLGlyph {
	uint32 type;
	//----------
	Point2 pos0;
	Point2 pos1;
	Point2 uv0;
	Point2 uv1;
	Color color;
	//----------
	float32 thickness;
	float32 period;
};

struct OpenGLHue {
	Point2 pos0;
	Point2 pos1;
	Point2 uv;
};

struct OpenGLSLBox {
	Point2 pos0;
	Point2 pos1;
	float32 hue;
};

// TODO: remove?
struct OpenGLSegment {
	Point2 pos0;
	Point2 pos1;
	Point2 pos2;
	Point2 pos3;
	Color color;
};

struct OpenGLInputElement {
	GLint size;
	ssize offset;
};

struct OpenGLProgram {
	GLuint handle;
	GLsizei stride;
	OpenGLInputElement elements[12];
	int32 elementCount;
	const GLchar* textureName;
	const GLchar* dimName;
	GLuint texture;
	Dimensions2 dim;
};

struct OpenGLVBuffer {
	OpenGLProgram* currentProgram;
	void* quads;
	int32 quadCount;
	int32 size;
};

struct {
	OpenGLProgram quadProgram;
	OpenGLProgram glyphProgram;
	OpenGLProgram hueProgram;
	OpenGLProgram slProgram;

	// TODO: remove?
	OpenGLProgram segmentProgram;
	OpenGLProgram imageProgram;

	OpenGLVBuffer vbuffers[4];
	int32 activeBuffer;

	Color backgorundColor;
	Dimensions2i dim;
	Box2 crop;
} opengl;

GLchar* quadVertexSource = (GLchar*)R"STRING(
#version 420

layout (location =  0) in vec2  in_pos0;
layout (location =  1) in vec2  in_pos1;
layout (location =  2) in vec4  in_color00;
layout (location =  3) in vec4  in_color01;
layout (location =  4) in vec4  in_color10;
layout (location =  5) in vec4  in_color11;
layout (location =  6) in float in_cornerRadius;
layout (location =  7) in float in_borderThickness;
layout (location =  8) in vec4  in_borderColor;
layout (location =  9) in vec4  in_crop;
layout (location = 10) in float in_blur;

uniform mat4 mvp;

out vec4  color;
out vec2  pixelpos;
out float cornerRadius;
out float borderThickness;
out vec4  borderColor;
out vec2  halfSize;
out vec2  center;
out vec4  crop;
out float blur;

void main() {
	vec2 poses[4] = {
		vec2(in_pos0.x, in_pos0.y),
		vec2(in_pos0.x, in_pos1.y),
		vec2(in_pos1.x, in_pos0.y),
		vec2(in_pos1.x, in_pos1.y),
	};
	pixelpos = poses[gl_VertexID];
	gl_Position = mvp * vec4(pixelpos, 0.0, 1.0);
	vec4 colors[4] = {
		in_color00,
		in_color01,
		in_color10,
		in_color11
	};
	color = colors[gl_VertexID];
	cornerRadius = in_cornerRadius;
	borderThickness = in_borderThickness;
	borderColor = in_borderColor;
	halfSize = 0.5*(in_pos1 - in_pos0 - in_blur);
	center = 0.5*(in_pos1 + in_pos0);
	crop = in_crop;
	blur = in_blur;
} 
	)STRING";

GLchar* quadFragmentSource = (GLchar*)R"STRING(
#version 420
out vec4 out_color;

in vec4  color;
in vec2  pixelpos;
in float cornerRadius;
in float borderThickness;
in vec4  borderColor;
in vec2  halfSize;
in vec2  center;
in vec4  crop;
in float blur;

float sd(vec2 pos, vec2 halfSize, float radius) {
	pos = abs(pos) - halfSize + radius;
	return length(max(pos, 0)) + min(max(pos.x, pos.y), 0) - radius;
}

vec3 sRGB_To_Linear(vec3 srgb) {
	vec3 low  = srgb/12.92;
	vec3 high = pow((srgb + 0.055)/1.055, vec3(2.4));
	return mix(low, high, step(0.04045, srgb));
}

void main() {
	if (pixelpos.x < crop.x || pixelpos.x > crop.z || pixelpos.y < crop.y || pixelpos.y > crop.w)
		discard;

	float sd_from_outer = sd(pixelpos - center, halfSize, cornerRadius);
	float sd_from_inner = sd_from_outer + borderThickness;
	
	float smoothness = 0.573896787348 + 0.5*blur;
	float a1 = 1.0f - smoothstep(-smoothness, smoothness, sd_from_outer);
	float a2 = 1.0f - smoothstep(-smoothness, smoothness, sd_from_inner);
	
	if (borderThickness > 0)
		out_color = mix(borderColor, color, a2);
	else
		out_color = color;
	out_color = vec4(sRGB_To_Linear(out_color.rgb), out_color.a*a1);
}
	)STRING";

GLchar* glyphVertexSource = (GLchar*)R"STRING(
#version 420

layout (location = 0) in uint  in_type;
layout (location = 1) in vec2  in_pos0;
layout (location = 2) in vec2  in_pos1;
layout (location = 3) in vec2  in_uv0;
layout (location = 4) in vec2  in_uv1;
layout (location = 5) in vec4  in_color;
layout (location = 6) in float in_thickness;
layout (location = 7) in float in_period;

flat out uint  type;
out vec2  uv;
out vec4  color;
out vec2  xy;
out float thickness;
out float pixel;

uniform mat4 mvp;
uniform vec2 atlasDim;

vec3 sRGB_To_Linear(vec3 srgb) {
	vec3 low  = srgb/12.92;
	vec3 high = pow((srgb + 0.055)/1.055, vec3(2.4));
	return mix(low, high, step(0.04045, srgb));
}

void main() {
	vec2 poses[4] = {
		vec2(in_pos0.x, in_pos0.y),
		vec2(in_pos0.x, in_pos1.y),
		vec2(in_pos1.x, in_pos0.y),
		vec2(in_pos1.x, in_pos1.y),
	};
	vec2 pixelpos = poses[gl_VertexID];
	vec2 uvs[4] = {
		vec2(in_uv0.x, in_uv0.y),
		vec2(in_uv0.x, in_uv1.y),
		vec2(in_uv1.x, in_uv0.y),
		vec2(in_uv1.x, in_uv1.y),
	};

	float tau = 6.28318548;
	float x1 = abs(in_pos1.x - in_pos0.x)*tau/in_period;

	vec2 xys[] = {
		vec2(0, -2),
		vec2(0, 2),
		vec2(x1, -2),
		vec2(x1, 2),
	};
	xy = xys[gl_VertexID];
	
	pixel = 2/abs(in_pos1.y - in_pos0.y);
	thickness = in_thickness;

	type = in_type;
	uv = uvs[gl_VertexID]/atlasDim;
	color = vec4(sRGB_To_Linear(in_color.rgb), in_color.a);
	gl_Position = mvp * vec4(pixelpos, 0.0, 1.0);
}
	)STRING";

GLchar* glyphFragmentSource = (GLchar*)R"STRING(
#version 420

flat in uint  type;
in vec2  uv;
in vec4  color;
in vec2  xy;
in float thickness;
in float pixel;

out vec4 out_color;

uniform sampler2D atlas;

void main() {
	out_color = color;
	switch (type) {
	case 0: {
		float a = texture(atlas, uv).r;
		out_color.a *= a;
	} break;
	case 1: {
		float value = sin(xy.x);
		float dist = abs(value - xy.y) - thickness*pixel;
		float a = 1 - smoothstep(-pixel, pixel, dist);
		out_color.a *= a;
	} break;
	};
}   
	)STRING";

GLchar* imageVertexSource = (GLchar*)R"STRING(
#version 420

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_uv;

out vec2 uv;

uniform mat4 mvp;

void main() {
	uv = in_uv;

	gl_Position = mvp * vec4(in_position, 0.0, 1.0);
}
	)STRING";

GLchar* imageFragmentSource = (GLchar*)R"STRING(
#version 420

in vec2 uv;

out vec4 out_color;

uniform sampler2D image;

void main() {
	out_color = texture(image, uv);
}   
	)STRING";

GLchar* segmentVertexSource = (GLchar*)R"STRING(
#version 420

layout (location = 0) in vec2 in_pos0;
layout (location = 1) in vec2 in_pos1;
layout (location = 2) in vec2 in_pos2;
layout (location = 3) in vec2 in_pos3;
layout (location = 4) in vec4 in_color;

out vec4 color;

uniform mat4 mvp;

vec3 sRGB_To_Linear(vec3 srgb) {
	vec3 low  = srgb/12.92;
	vec3 high = pow((srgb + 0.055)/1.055, vec3(2.4));
	return mix(low, high, step(0.04045, srgb));
}

void main() {
	color = vec4(sRGB_To_Linear(in_color.rgb), in_color.a);

	vec2 poses[4] = {
		in_pos0,
		in_pos1,
		in_pos2,
		in_pos3,
	};
	vec2 pixelpos = poses[gl_VertexID];
	gl_Position = mvp * vec4(pixelpos, 0.0, 1.0);
}
	)STRING";

GLchar* segmentFragmentSource = (GLchar*)R"STRING(
#version 420

in vec4 color;

out vec4 out_color;

void main() {
	out_color = color;
}   
	)STRING";

GLchar* hueVertexSource = (GLchar*)R"STRING(
#version 420

layout (location = 0) in vec2 in_pos0;
layout (location = 1) in vec2 in_pos1;
layout (location = 2) in vec2 in_uv;

uniform mat4 mvp;

out float hue;

void main() {
	vec2 poses[] = {
		vec2(in_pos0.x, in_pos0.y),
		vec2(in_pos0.x, in_pos1.y),
		vec2(in_pos1.x, in_pos0.y),
		vec2(in_pos1.x, in_pos1.y),
	};
	vec2 pixelpos = poses[gl_VertexID];
	gl_Position = mvp * vec4(pixelpos, 0.0, 1.0);
	
	hue = (gl_VertexID & 1) > 0 ? in_uv.y : in_uv.x;
}
)STRING";

GLchar* hueFragmentSource = (GLchar*)R"STRING(
#version 420

out vec4 out_color;

in float hue;

vec3 sRGB_To_Linear(vec3 srgb) {
	vec3 low  = srgb/12.92;
	vec3 high = pow((srgb + 0.055)/1.055, vec3(2.4));
	return mix(low, high, step(0.04045, srgb));
}

void main() {
	float r = clamp(abs(hue * 6 - 3) - 1, 0.0, 1.0);
	float g = clamp(2 - abs(hue * 6 - 2), 0.0, 1.0);
	float b = clamp(2 - abs(hue * 6 - 4), 0.0, 1.0);
	out_color = vec4(sRGB_To_Linear(vec3(r, g, b)), 1);
}
)STRING";

GLchar* slVertexSource = (GLchar*)R"STRING(
#version 420
layout (location = 0) in vec2 in_pos0;
layout (location = 1) in vec2 in_pos1;
layout (location = 2) in float in_hue;

uniform mat4 mvp;

out vec3 hsl;

void main() {
	vec2 poses[] = {
		vec2(in_pos0.x, in_pos0.y),
		vec2(in_pos0.x, in_pos1.y),
		vec2(in_pos1.x, in_pos0.y),
		vec2(in_pos1.x, in_pos1.y),
	};
	vec2 pixelpos = poses[gl_VertexID];
	gl_Position = mvp * vec4(pixelpos, 0.0, 1.0);

	float xInvStep = abs(in_pos1.x - in_pos0.x) - 1;
	float yInvStep = abs(in_pos1.y - in_pos0.y) - 1;
	
	hsl.x = in_hue;
	hsl.y = (gl_VertexID & 2) > 0
		? 1.f + (1/(2*xInvStep))
		: 0.f - (1/(2*xInvStep));
	hsl.z = (gl_VertexID & 1) > 0
		? 0.f - (1/(2*yInvStep))
		: 1.f + (1/(2*yInvStep));
}
)STRING";

GLchar* slFragmentSource = (GLchar*)R"STRING(
#version 420

out vec4 out_color;

in vec3 hsl;

vec3 sRGB_To_Linear(vec3 srgb) {
	vec3 low  = srgb/12.92;
	vec3 high = pow((srgb + 0.055)/1.055, vec3(2.4));
	return mix(low, high, step(0.04045, srgb));
}

void main() {
	float h = hsl.x;
	float s = hsl.y;
	float l = hsl.z;

	float d = s*min(l, 1 - l);
	if (d == 0) {
		out_color = vec4(l, l, l, 1);
		return;
	}

	float hh = 12*h;

	float r, g, b;
	if (hh < 2) {
		r = l + d;
		g = l + d*(hh - 1);
		b = l - d;
	}
	else if (hh < 4) {
		r = l - d*(hh - 3);
		g = l + d;
		b = l - d;
	}
	else if (hh < 6) {
		r = l - d;
		g = l + d;
		b = l + d*(hh - 5);
	}
	else if (hh < 8) {
		r = l - d;
		g = l - d*(hh - 7);
		b = l + d;
	}
	else if (hh < 10) {
		r = l + d*(hh - 9);
		g = l - d;
		b = l + d;
	}
	else {
		r = l + d;
		g = l - d;
		b = l - d*(hh - 11);
	}

	out_color = vec4(sRGB_To_Linear(vec3(r, g, b)), 1);
}
)STRING";

void OpenGLUIInit(uint32 globalFlags) {
	opengl = {};
	OSOpenGLInit(globalFlags & GFX_MULTISAMPLE ? 4 : 1);

	opengl.vbuffers[0].quads = OSAllocate(KB(256));
	opengl.vbuffers[0].size = KB(256);
	opengl.vbuffers[1].quads = OSAllocate(KB(32));
	opengl.vbuffers[1].size = KB(32);
	opengl.vbuffers[2].quads = OSAllocate(KB(32));
	opengl.vbuffers[2].size = KB(32);
	opengl.vbuffers[3].quads = OSAllocate(KB(16));
	opengl.vbuffers[3].size = KB(16);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (globalFlags & GFX_SRGB)
		glEnable(GL_FRAMEBUFFER_SRGB); 

	if (globalFlags & GFX_SCISSOR)
		glEnable(GL_SCISSOR_TEST);
	
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	opengl.quadProgram = {
		CreateProgram(quadVertexSource, quadFragmentSource),
		sizeof(OpenGLQuad),
		{
			{2, offsetof(OpenGLQuad, pos0)},
			{2, offsetof(OpenGLQuad, pos1)},
			{4, offsetof(OpenGLQuad, color00)},
			{4, offsetof(OpenGLQuad, color01)},
			{4, offsetof(OpenGLQuad, color10)},
			{4, offsetof(OpenGLQuad, color11)},
			{1, offsetof(OpenGLQuad, cornerRadius)},
			{1, offsetof(OpenGLQuad, borderThickness)},
			{4, offsetof(OpenGLQuad, borderColor)},
			{4, offsetof(OpenGLQuad, crop)},
			{1, offsetof(OpenGLQuad, blur)},
		},
		11
	};
	opengl.glyphProgram = {
		CreateProgram(glyphVertexSource, glyphFragmentSource),
		sizeof(OpenGLGlyph),
		{
			{1, offsetof(OpenGLGlyph, type)},
			{2, offsetof(OpenGLGlyph, pos0)},
			{2, offsetof(OpenGLGlyph, pos1)},
			{2, offsetof(OpenGLGlyph, uv0)},
			{2, offsetof(OpenGLGlyph, uv1)},
			{4, offsetof(OpenGLGlyph, color)},
			{1, offsetof(OpenGLGlyph, thickness)},
			{1, offsetof(OpenGLGlyph, period)},
		},
		8,
		"atlas",
		"atlasDim"
	};
	opengl.segmentProgram = {
		CreateProgram(segmentVertexSource, segmentFragmentSource),
		sizeof(OpenGLSegment),
		{
			{2, offsetof(OpenGLSegment, pos0)},
			{2, offsetof(OpenGLSegment, pos1)},
			{2, offsetof(OpenGLSegment, pos2)},
			{2, offsetof(OpenGLSegment, pos3)},
			{4, offsetof(OpenGLSegment, color)},
		},
		5,
		"atlas"
	};
	opengl.hueProgram = {
		CreateProgram(hueVertexSource, hueFragmentSource),
		sizeof(OpenGLHue),
		{
			{2, offsetof(OpenGLHue, pos0)},
			{2, offsetof(OpenGLHue, pos1)},
			{2, offsetof(OpenGLHue, uv)},
		},
		3
	};
	opengl.slProgram = {
		CreateProgram(slVertexSource, slFragmentSource),
		sizeof(OpenGLSLBox),
		{
			{2, offsetof(OpenGLSLBox, pos0)},
			{2, offsetof(OpenGLSLBox, pos1)},
			{1, offsetof(OpenGLSLBox, hue)},
		},
		3
	};

	Dimensions2i dim = OSGetWindowDimensions();
	opengl.crop = {0, 0, (float32)dim.width, (float32)dim.height};
}

void OpenGLUISetBackgroundColor(Color color) {
	opengl.backgorundColor = color;
}

void OpenGLUISetFontAtlas(Image image) {
	opengl.glyphProgram.texture = OpenGLGenerateTexture(image, GL_LINEAR);
	opengl.glyphProgram.dim = {(float32)image.dimensions.width, (float32)image.dimensions.height};
}

void PixelSpaceYIsDown(GLuint program) {
	float32 matrix[4][4] = {
		{2.0f/opengl.dim.width, 0, 0, -1},
		{0, -2.0f/opengl.dim.height, 0, 1},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	};
	glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, &matrix[0][0]);
}

void OpenGLUIBeginDrawing() {
	opengl.dim = OSGetWindowDimensions();
	glClearColor(opengl.backgorundColor.r, opengl.backgorundColor.g, opengl.backgorundColor.b, opengl.backgorundColor.a);
	glScissor(0, 0, opengl.dim.width, opengl.dim.height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, opengl.dim.width, opengl.dim.height);

	opengl.vbuffers[opengl.activeBuffer].quadCount = 0;
}

void OpenGLUIFlush() {
	OpenGLVBuffer* vbuffer = opengl.vbuffers + opengl.activeBuffer;
	if (!vbuffer->currentProgram || !vbuffer->quadCount) return;

	glUseProgram(vbuffer->currentProgram->handle);

	if (vbuffer->currentProgram->textureName) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, vbuffer->currentProgram->texture);
		GLint location = glGetUniformLocation(vbuffer->currentProgram->handle, vbuffer->currentProgram->textureName);
		glUniform1i(location, 0);
	}
	if (vbuffer->currentProgram->dimName) {
		GLint location = glGetUniformLocation(vbuffer->currentProgram->handle, vbuffer->currentProgram->dimName);
		glUniform2f(location, vbuffer->currentProgram->dim.width, vbuffer->currentProgram->dim.height);	
	}
	PixelSpaceYIsDown(vbuffer->currentProgram->handle);

	// TODO: do I need to generate and delete buffers every frame?
	//       Or can I do it once?
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, vbuffer->currentProgram->stride * vbuffer->quadCount, vbuffer->quads, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	for (int32 i = 0; i < vbuffer->currentProgram->elementCount; i++) {
		OpenGLInputElement* e = vbuffer->currentProgram->elements + i;
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, e->size, GL_FLOAT, GL_FALSE, vbuffer->currentProgram->stride, (void*)e->offset);
		glVertexAttribDivisor(i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(vertexArray);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, vbuffer->quadCount);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vertexArray);
	glDeleteBuffers(1, &buffer);

	vbuffer->quadCount = 0;
}

void OpenGLUIEndDrawing() {
	OpenGLUIFlush();
	OSOpenGLSwapBuffers();
}

void OpenGLUISetActiveVBuffer(int32 index) {
	opengl.activeBuffer = index;
}

void SwitchProgram(OpenGLProgram* program) {
	OpenGLVBuffer* buffer = opengl.vbuffers + opengl.activeBuffer;
	if (buffer->currentProgram != program) {
		OpenGLUIFlush();
		buffer->currentProgram = program;
	}
}

void AddQuadToActiveVBuffer(void* quad, ssize size) {
	OpenGLVBuffer* vbuffer = opengl.vbuffers + opengl.activeBuffer;
	if (vbuffer->quadCount*(size + 1) > vbuffer->size) {
		ASSERT(0);
		return;
	}

	memcpy((byte*)vbuffer->quads + vbuffer->quadCount*size, quad, size);
	vbuffer->quadCount++;
}

void DrawQuad(OpenGLQuad quad) {
	SwitchProgram(&opengl.quadProgram);

	AddQuadToActiveVBuffer(&quad, sizeof(quad));
}

void OpenGLUIDrawSolidColorQuad(
	Point2 pos, 
	Dimensions2 dim, 
	Color color, 
	float32 cornerRadius = 0, 
	float32 borderThickness = 0, 
	Color borderColor = {}) {

	OpenGLQuad quad = {
		pos,
		pos + dim,
		color,
		color,
		color, 
		color,
		cornerRadius,
		borderThickness,
		borderColor,
		{pos.x, pos.y, pos.x + dim.width, pos.y + dim.height}
	};

	DrawQuad(quad);
}

void OpenGLUIDrawSolidColorQuad(
	Box2 box, 
	Color color, 
	float32 cornerRadius = 0, 
	float32 borderThickness = 0, 
	Color borderColor = {}) {

	OpenGLQuad quad = {
		box.p0,
		box.p1,
		color,
		color,
		color, 
		color,
		cornerRadius,
		borderThickness,
		borderColor,
		box
	};

	DrawQuad(quad);
}

void OpenGLUIDrawVerticalGradQuad(
	Point2 pos, 
	Dimensions2 dim, 
	Color color1,
	Color color2, 
	float32 cornerRadius = 0, 
	float32 borderThickness = 0, 
	Color borderColor = {}) {

	OpenGLQuad quad = {
		pos,
		pos + dim,
		color1,
		color2,
		color1, 
		color2,
		cornerRadius,
		borderThickness,
		borderColor,
		{pos.x, pos.y, pos.x + dim.width, pos.y + dim.height}
	};

	DrawQuad(quad);
}

void OpenGLUIDrawVerticalGradQuad(
	Box2 box, 
	Color color1,
	Color color2, 
	float32 cornerRadius = 0, 
	float32 borderThickness = 0, 
	Color borderColor = {}) {

	OpenGLQuad quad = {
		box.p0,
		box.p1,
		color1,
		color2,
		color1, 
		color2,
		cornerRadius,
		borderThickness,
		borderColor,
		box
	};

	DrawQuad(quad);
}

void OpenGLUIDrawHorizontalGradQuad(
	Point2 pos, 
	Dimensions2 dim, 
	Color color1, 
	Color color2,
	float32 cornerRadius = 0, 
	float32 borderThickness = 0, 
	Color borderColor = {}) {

	OpenGLQuad quad = {
		pos,
		pos + dim,
		color1,
		color1,
		color2, 
		color2,
		cornerRadius,
		borderThickness,
		borderColor,
		{pos.x, pos.y, pos.x + dim.width, pos.y + dim.height}
	};

	DrawQuad(quad);
}

void OpenGLUIDrawHorizontalGradQuad(
	Box2 box, 
	Color color1, 
	Color color2,
	float32 cornerRadius = 0, 
	float32 borderThickness = 0, 
	Color borderColor = {}) {

	OpenGLQuad quad = {
		box.p0,
		box.p1,
		color1,
		color1,
		color2, 
		color2,
		cornerRadius,
		borderThickness,
		borderColor,
		box
	};

	DrawQuad(quad);
}

void OpenGLUIDrawOutlineQuad(Point2 pos, Dimensions2 dim, float32 cornerRadius, float32 thickness, Color color) {

	OpenGLQuad quad = {
		pos,
		pos + dim,
		{},
		{},
		{}, 
		{},
		0,
		thickness,
		color,
		{pos.x, pos.y, pos.x + dim.width, pos.y + dim.height}
	};

	DrawQuad(quad);
}

void OpenGLUIDrawOutlineQuad(Box2 box, float32 cornerRadius, float32 thickness, Color color) {

	OpenGLQuad quad = {
		box.p0,
		box.p1,
		{},
		{},
		{}, 
		{},
		0,
		thickness,
		color,
		box
	};

	DrawQuad(quad);
}

void OpenGLUIDrawSphere(Point2 center, float32 radius, Color color, float32 borderThickness, Color borderColor) {

	Point2 pos0 = {center.x - radius, center.y - radius};
	Point2 pos1 = {center.x + radius, center.y + radius};	

	OpenGLQuad quad = {
		pos0,
		pos1,
		color,
		color,
		color, 
		color,
		radius,
		borderThickness,
		borderColor,
		{pos0.x, pos0.y, pos1.x, pos1.y}
	};

	DrawQuad(quad);
}

void OpenGLUIDrawSemiSphere(Point2 pos, float32 radius, Quadrant quadrant, float32 thickness, Color color) {
	Point2 pos0 = {};
	Point2 pos1 = {};
	{
		switch (quadrant) {
			case Quadrant_I: {
				pos0 = {pos.x - radius, pos.y};
				pos1 = {pos.x + radius, pos.y + 2*radius};
			} break;
			case Quadrant_II: {
				pos0 = pos;
				pos1 = {pos.x + 2*radius, pos.y + 2*radius};
			} break;
			case Quadrant_III: {
				pos0 = {pos.x, pos.y - radius};
				pos1 = {pos.x + 2*radius, pos.y + radius};
			} break;
			case Quadrant_IV: {
				pos0 = {pos.x - radius, pos.y - radius};
				pos1 = {pos.x + radius, pos.y + radius};
			} break;
		}
	}

	OpenGLQuad quad = {
		pos0,
		pos1,
		{},
		{},
		{}, 
		{},
		radius,
		thickness,
		color,
		{pos.x, pos.y, pos.x + radius, pos.y + radius},
	};

	DrawQuad(quad);
}

void OpenGLUIDrawShadow(Point2 pos, Dimensions2 dim, float32 radius, Point2 offset, float32 blur, Color color) {
	Box2 box = 
		{pos.x + offset.x - blur,             pos.y + offset.y - blur,
		 pos.x + offset.x + blur + dim.width, pos.y + offset.y + blur + dim.height};

	OpenGLQuad quad = {
		{box.x0, box.y0},
		{box.x1, box.y1},
		color,
		color,
		color, 
		color,
		radius,
		0,
		color,
		box,
		blur
	};

	DrawQuad(quad);
}

// TESTME!
void GfxDrawImage(Point2 pos, Dimensions2 dim, GLuint texture, Box2 crop) {
	OpenGLUIFlush();
	glUseProgram(opengl.imageProgram.handle);
	glDisable(GL_MULTISAMPLE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(opengl.imageProgram.handle, "image"), 0);
	PixelSpaceYIsDown(opengl.imageProgram.handle);

	Point2 p0 = pos;
	Point2 p1 = {pos.x + dim.width, pos.y + dim.height};
	float32 vertices[] = {
		p0.x, p0.y, crop.x0, crop.y1,
		p1.x, p0.y, crop.x1, crop.y1,
		p0.x, p1.y, crop.x0, crop.y0,
		p1.x, p1.y, crop.x1, crop.y0
	};	

	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float32), NULL);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float32), (void*)(2*sizeof(float32)) );

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDeleteVertexArrays(1, &vertexArray);
}

void OpenGLUIDrawGlyph(Point2 pos, Dimensions2 dim, Box2 crop, Color color) {
	SwitchProgram(&opengl.glyphProgram);

	OpenGLGlyph glyph = {
		0,
		pos,
		pos + dim,
		crop.p0,
		crop.p1,
		color,
		0, 0
	};

	AddQuadToActiveVBuffer(&glyph, sizeof(glyph));
}

void OpenGLUIDrawCurve(Point2 p0, Point2 p1, Point2 p2, Point2 p3, float32 thick, Color color) {
	SwitchProgram(&opengl.segmentProgram);
	
#define SEGMENTS	32
	Point2 prev = p0;
    Point2 current = {};
	for (int32 i = 0; i < SEGMENTS; i++) {
	    float32 t = (i+1) / (float32)SEGMENTS;
	    current = Cerp(t, p0, p1, p2, p3);

		Point2 delta = {current.x - prev.x, current.y - prev.y};
		float32 length = sqrt(delta.x*delta.x + delta.y*delta.y);
		
		float32 size = (0.5f*thick)/length;
		Point2 radius = {-size*delta.y, size*delta.x};
		
		OpenGLSegment segment = {
			prev - radius,
			prev + radius,
			current - radius,
			current + radius,
			color
		};

		AddQuadToActiveVBuffer(&segment, sizeof(segment));
		prev = current;
	}
#undef SEGMENTS
}

void OpenGLUIDrawWavyLine(Point2 pos, Dimensions2 dim, float32 thickness, Color color) {
	SwitchProgram(&opengl.glyphProgram);

	Point2 unused = {};
	OpenGLGlyph glyph = {
		1,
		{pos.x, pos.y - dim.height},
		{pos.x + dim.width, pos.y + dim.height},
		unused,
		unused,
		color,
		thickness,
		2.5f*dim.height,
	};

	AddQuadToActiveVBuffer(&glyph, sizeof(glyph));
}

void OpenGLUIDrawStraightLine(Point2 pos, float32 width, float32 thickness, Color color) {
	SwitchProgram(&opengl.glyphProgram);

	Point2 unused = {};
	OpenGLGlyph glyph = {
		2,
		{pos.x, pos.y - 0.5f*thickness},
		{pos.x + width, pos.y + 0.5f*thickness},
		unused,
		unused,
		color,
		0, 0
	};

	AddQuadToActiveVBuffer(&glyph, sizeof(glyph));
}

void OpenGLUIDrawHueGrad(Point2 pos0, Point2 pos1, Point2 uv) {
	SwitchProgram(&opengl.hueProgram);

	OpenGLHue hue = {pos0, pos1, uv};

	AddQuadToActiveVBuffer(&hue, sizeof(hue));
}

void OpenGLUIDrawSLQuad(Point2 pos0, Point2 pos1, float32 hue) {
	SwitchProgram(&opengl.slProgram);

	OpenGLSLBox sl = {pos0, pos1, hue};

	AddQuadToActiveVBuffer(&sl, sizeof(sl));
}

Box2 OpenGLUIGetCurrentCrop() {
	return opengl.crop;
}

void OpenGLUICropScreen(Point2 pos, Dimensions2 dim) {
	OpenGLUIFlush();
	opengl.crop = {pos.x, pos.y, pos.x + dim.width, pos.y + dim.height};

	glScissor((GLint)pos.x, 
			  (GLint)(opengl.dim.height - pos.y - dim.height), 
			  (GLsizei)dim.width, 
			  (GLsizei)dim.height);
}

void OpenGLUICropScreen(Box2 box) {
	OpenGLUIFlush();
	opengl.crop = box;

	glScissor((GLint)box.x0, 
			  (GLint)(opengl.dim.height - box.y1), 
			  (GLsizei)(box.x1 - box.x0), 
			  (GLsizei)(box.y1 - box.y0));
}

void OpenGLUIClearCrop() {
	OpenGLUIFlush();
	opengl.crop = {0, 0, (float32)opengl.dim.width, (float32)opengl.dim.height};

	glScissor(0, 0, opengl.dim.width, opengl.dim.height);
}
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
};

struct OpenGLGlyph {
	Point2 pos0;
	Point2 pos1;
	Point2 uv0;
	Point2 uv1;
	Color color;
};

struct OpenGLSegment {
	Point2 pos0;
	Point2 pos1;
	Point2 pos2;
	Point2 pos3;
	Color color;
};

struct OpenGLShadow {
	Point2 pos0;
	Point2 pos1;
	float32 cornerRadius;
	float32 blur;
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
	GLchar* textureName;
	GLchar* dimName;
	GLuint texture;
	Dimensions2 dim;
};

struct {
	OpenGLProgram quadProgram;
	OpenGLProgram glyphProgram;
	OpenGLProgram segmentProgram;
	OpenGLProgram imageProgram;
	OpenGLProgram shadowProgram;
	OpenGLProgram* currentProgram;
	void* quads;
	int32 quadCount;
	Color backgorundColor;
	Dimensions2i dim;
} opengl;

GLchar* quadVertexSource = (GLchar*)R"STRING(
#version 330 core

layout (location = 0) in vec2 in_pos0;
layout (location = 1) in vec2 in_pos1;
layout (location = 2) in vec4 in_color00;
layout (location = 3) in vec4 in_color01;
layout (location = 4) in vec4 in_color10;
layout (location = 5) in vec4 in_color11;
layout (location = 6) in float in_cornerRadius;
layout (location = 7) in float in_borderThickness;
layout (location = 8) in vec4 in_borderColor;

uniform mat4 mvp;

out vec4 color;
out vec2 pixelpos;
out float cornerRadius;
out float borderThickness;
out vec4 borderColor;
out vec2 halfSize;
out vec2 center;

void main()
{
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
	halfSize = 0.5*(in_pos1 - in_pos0);
	center = 0.5*(in_pos1 + in_pos0);
} 
	)STRING";

GLchar* quadFragmentSource = (GLchar*)R"STRING(
#version 330 core
out vec4 out_color;

in vec4 color;
in vec2 pixelpos;
in float cornerRadius;
in float borderThickness;
in vec4 borderColor;
in vec2 halfSize;
in vec2 center;

float sd(vec2 pos, vec2 halfSize, float radius) {
	pos = abs(pos) - halfSize + radius;
	return length(max(pos, 0)) + min(max(pos.x, pos.y), 0) - radius;
}

void main()
{
	float sd_from_outer = sd(pixelpos - center, halfSize, cornerRadius);
	float sd_from_inner = sd_from_outer + borderThickness;
	
	float a1 = 1.0f - smoothstep(-0.5, +0.5, sd_from_outer);
	float a2 = 1.0f - smoothstep(-0.5, +0.5, sd_from_inner);
	
	out_color = mix(borderColor, color, a2);
	out_color.a = out_color.a*a1;
}
	)STRING";

GLchar* glyphVertexSource = (GLchar*)R"STRING(
#version 330 core

layout (location = 0) in vec2 in_pos0;
layout (location = 1) in vec2 in_pos1;
layout (location = 2) in vec2 in_uv0;
layout (location = 3) in vec2 in_uv1;
layout (location = 4) in vec4 in_color;

out vec2 uv;
out vec4 color;

uniform mat4 mvp;
uniform vec2 atlasDim;

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

	uv = uvs[gl_VertexID]/atlasDim;
	color = in_color;
	gl_Position = mvp * vec4(pixelpos, 0.0, 1.0);
}
	)STRING";

GLchar* glyphFragmentSource = (GLchar*)R"STRING(
#version 330 core

in vec2 uv;
in vec4 color;

out vec4 out_color;

uniform sampler2D atlas;

void main() {
	float a = texture(atlas, uv).r;
    out_color = vec4(color.rgb, color.a*a);
}   
	)STRING";

GLchar* imageVertexSource = (GLchar*)R"STRING(
#version 330 core

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
#version 330 core

in vec2 uv;

out vec4 out_color;

uniform sampler2D image;

void main() {
	out_color = texture(image, uv);
}   
	)STRING";

GLchar* segmentVertexSource = (GLchar*)R"STRING(
#version 330 core

layout (location = 0) in vec2 in_pos0;
layout (location = 1) in vec2 in_pos1;
layout (location = 2) in vec2 in_pos2;
layout (location = 3) in vec2 in_pos3;
layout (location = 4) in vec4 in_color;

out vec4 color;

uniform mat4 mvp;

void main() {
	color = in_color;

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
#version 330 core

in vec4 color;

out vec4 out_color;

void main() {
	out_color = color;
}   
	)STRING";

GLchar* shadowVertexSource = (GLchar*)R"STRING(
#version 330 core

layout (location = 0) in vec2 in_pos0;
layout (location = 1) in vec2 in_pos1;
layout (location = 2) in float in_cornerRadius;
layout (location = 3) in float in_blur;
layout (location = 4) in vec4 in_color;

uniform mat4 mvp;

out vec4 color;
out vec2 pixelpos;
out float cornerRadius;
out float blur;
out vec2 halfSize;
out vec2 center;

void main()
{
	vec2 poses[4] = {
		vec2(in_pos0.x, in_pos0.y),
		vec2(in_pos0.x, in_pos1.y),
		vec2(in_pos1.x, in_pos0.y),
		vec2(in_pos1.x, in_pos1.y),
	};
	pixelpos = poses[gl_VertexID];
	gl_Position = mvp * vec4(pixelpos, 0.0, 1.0);

	color = in_color;
	blur = in_blur;
	cornerRadius = in_cornerRadius;
	halfSize = 0.5*(in_pos1 - in_pos0);
	center = 0.5*(in_pos1 + in_pos0);
} 
	)STRING";

GLchar* shadowFragmentSource = (GLchar*)R"STRING(
#version 330 core
out vec4 out_color;

in vec4 color;
in vec2 pixelpos;
in float cornerRadius;
in float blur;
in vec2 halfSize;
in vec2 center;

float sd(vec2 pos, vec2 halfSize, float radius) {
	pos = abs(pos) - halfSize + radius;
	return length(max(pos, 0)) + min(max(pos.x, pos.y), 0) - radius;
}

void main()
{
	float sd_ = sd(pixelpos - center, halfSize, cornerRadius + blur);
	
	float a = 1.0f - smoothstep(-blur, +0.5, sd_);
	
	out_color = color;
	out_color.a = out_color.a*a;
}
	)STRING";

void OpenGLUIInit(uint32 globalFlags) {
	opengl = {};
	OSOpenGLInit(globalFlags & GFX_MULTISAMPLE ? 4 : 1);
	opengl.quads = OSAllocate(sizeof(OpenGLQuad)*1024);

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
			{4, offsetof(OpenGLQuad, borderColor)}
		},
		9
	};
	opengl.glyphProgram = {
		CreateProgram(glyphVertexSource, glyphFragmentSource),
		sizeof(OpenGLGlyph),
		{
			{2, offsetof(OpenGLGlyph, pos0)},
			{2, offsetof(OpenGLGlyph, pos1)},
			{2, offsetof(OpenGLGlyph, uv0)},
			{2, offsetof(OpenGLGlyph, uv1)},
			{4, offsetof(OpenGLGlyph, color)},
		},
		5,
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
	opengl.shadowProgram = {
		CreateProgram(shadowVertexSource, shadowFragmentSource),
		sizeof(OpenGLShadow),
		{
			{2, offsetof(OpenGLShadow, pos0)},
			{2, offsetof(OpenGLShadow, pos1)},
			{1, offsetof(OpenGLShadow, cornerRadius)},
			{1, offsetof(OpenGLShadow, blur)},
			{4, offsetof(OpenGLShadow, color)}
		},
		5
	};
	opengl.quadCount = 0;
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

	opengl.quadCount = 0;
}

void FlushVertices() {
	if (!opengl.currentProgram || !opengl.quadCount) return;

	glUseProgram(opengl.currentProgram->handle);

	if (opengl.currentProgram->textureName) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, opengl.currentProgram->texture);
		GLint location = glGetUniformLocation(opengl.currentProgram->handle, opengl.currentProgram->textureName);
		glUniform1i(location, 0);
	}
	if (opengl.currentProgram->dimName) {
		GLint location = glGetUniformLocation(opengl.currentProgram->handle, opengl.currentProgram->dimName);
		glUniform2f(location, opengl.currentProgram->dim.width, opengl.currentProgram->dim.height);	
	}
	PixelSpaceYIsDown(opengl.currentProgram->handle);

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, opengl.currentProgram->stride * opengl.quadCount, opengl.quads, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	for (int32 i = 0; i < opengl.currentProgram->elementCount; i++) {
		OpenGLInputElement* e = opengl.currentProgram->elements + i;
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, e->size, GL_FLOAT, GL_FALSE, opengl.currentProgram->stride, (void*)e->offset);
		glVertexAttribDivisor(i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(vertexArray);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, opengl.quadCount);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vertexArray);

	opengl.quadCount = 0;
}

void OpenGLUIEndDrawing() {
	FlushVertices();
	OSOpenGLSwapBuffers();
}

void DrawQuad(OpenGLQuad quad) {
	if (opengl.currentProgram != &opengl.quadProgram) {
		FlushVertices();
		opengl.currentProgram = &opengl.quadProgram;
		glDisable(GL_MULTISAMPLE);
	}

	memcpy((OpenGLQuad*)opengl.quads + opengl.quadCount, &quad, sizeof(quad));
	opengl.quadCount++;
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
	};

	DrawQuad(quad);
}

void OpenGLUIDrawOutlineQuad(Point2 pos, Dimensions2 dim, float32 thickness, Color color) {

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
	};

	DrawQuad(quad);
}

void OpenGLUIDrawGlyph(Point2 pos, Dimensions2 dim, Box2 crop, Color color) {
	if (opengl.currentProgram != &opengl.glyphProgram) {
		FlushVertices();
		opengl.currentProgram = &opengl.glyphProgram;
		glDisable(GL_MULTISAMPLE);
	}

	OpenGLGlyph glyph = {
		pos,
		pos + dim,
		crop.p0,
		crop.p1,
		color
	};

	memcpy((OpenGLGlyph*)opengl.quads + opengl.quadCount, &glyph, sizeof(glyph));
	opengl.quadCount++;
}

void OpenGLUIDrawLine(Point2 p0, Point2 p1, float32 thick, Color color) {
	if (opengl.currentProgram != &opengl.segmentProgram) {
		FlushVertices();
		opengl.currentProgram = &opengl.segmentProgram;
		glEnable(GL_MULTISAMPLE);
	}

	Point2 delta = p1 - p0;
	float32 length = sqrt(delta.x*delta.x + delta.y*delta.y);
		
	float32 size = (0.5f*thick)/length;
	Point2 radius = {size*delta.y, size*delta.x};
		
	OpenGLSegment segment = {
		p0 - radius,
		p0 + radius,
		p1 - radius,
		p1 + radius,
		color
	};

	memcpy((OpenGLSegment*)opengl.quads + opengl.quadCount, &segment, sizeof(segment));
	opengl.quadCount++;
}

void OpenGLUIDrawCurve(Point2 p0, Point2 p1, Point2 p2, Point2 p3, float32 thick, Color color) {
	if (opengl.currentProgram != &opengl.segmentProgram) {
		FlushVertices();
		opengl.currentProgram = &opengl.segmentProgram;
		glEnable(GL_MULTISAMPLE);
	}
	
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

		memcpy((OpenGLSegment*)opengl.quads + opengl.quadCount, &segment, sizeof(segment));
		opengl.quadCount++;
		prev = current;
	}
#undef SEGMENTS
}

void OpenGLUIDrawShadow(Point2 pos, Dimensions2 dim, float32 radius, Point2 offset, float32 blur, Color color) {
	if (opengl.currentProgram != &opengl.shadowProgram) {
		FlushVertices();
		opengl.currentProgram = &opengl.shadowProgram;
		glDisable(GL_MULTISAMPLE);
	}

	OpenGLShadow shadow = {
		{pos.x + offset.x - blur/2,             pos.y + offset.y - blur/2}, 
		{pos.x + offset.x + blur/2 + dim.width, pos.y + offset.y + blur/2 + dim.height}, 
		radius, blur, color
	};

	memcpy((OpenGLShadow*)opengl.quads + opengl.quadCount, &shadow, sizeof(shadow));
	opengl.quadCount++;
}

// TESTME!
void GfxDrawImage(Point2 pos, Dimensions2 dim, GLuint texture, Box2 crop) {
	FlushVertices();
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

void OpenGLUICropScreen(int32 x, int32 y, int32 width, int32 height) {
	FlushVertices();
	glScissor(x, opengl.dim.height - y - height, width, height);
}

void OpenGLUIClearCrop() {
	FlushVertices();
	glScissor(0, 0, opengl.dim.width, opengl.dim.height);
}
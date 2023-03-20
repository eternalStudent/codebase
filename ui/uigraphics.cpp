/*
 * NOTE:
 * This is temporary, until I will get a properly functioning
 * graphics layer.
 * Much of the code here should eventually move to opengl.cpp
 */

struct {
	Dimensions2i dim;
	GLuint quadProgram;
	GLuint textProgram;
	GLuint imageProgram;
	GLuint currentProgram;
	float32* vertices;
	int32 quadCount;
	Color backgorundColor;
	GLuint fontAtlas;
} gfx;

GLuint CreateQuadProgram() {
	GLchar* vertexSource = (GLchar*)R"STRING(
#version 330 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_center;
layout (location = 2) in vec2 in_size;
layout (location = 3) in float in_radius;
layout (location = 4) in float in_borderWidth;
layout (location = 5) in vec4 in_color;
layout (location = 6) in vec4 in_borderColor;

out vec2 position;
out vec2 center;
out vec2 size;
out float radius;
out float borderWidth;
out vec4 color;
out vec4 borderColor;

uniform mat4 mvp;

void main() {
	gl_Position = mvp * vec4(in_position, 0.0, 1.0);

	position = in_position;
	center = in_center;
	size = in_size;
	radius = in_radius;
	borderWidth = in_borderWidth;
	color = in_color;
	borderColor = in_borderColor;
}
	)STRING";

	GLchar* fragmentSource = (GLchar*)R"STRING(
#version 330 core

in vec2 position;
in vec2 center;
in vec2 size;
in float radius;
in float borderWidth;
in vec4 color;
in vec4 borderColor;

out vec4 out_color;

float sd(vec2 pos, vec2 halfSize, float radius) {
	pos = abs(pos) - halfSize + radius;
	return length(max(pos, 0)) + min(max(pos.x, pos.y), 0) - radius;
}

void main() {
	float sd_from_outer = sd(position - center, size/2.f, radius) + 0.5;
	float sd_from_inner = sd_from_outer + borderWidth;
	
    float a1 = 1.0f - smoothstep(0.0f, 1.0f, sd_from_outer);
    float a2 = 1.0f - smoothstep(0.0f, 1.0f, sd_from_inner);
	
	out_color = mix(borderColor, color, a2);
	out_color.a = out_color.a*a1;
}   
	)STRING";

	return CreateProgram(vertexSource, fragmentSource);
}

GLuint CreateTextProgram() {
	GLchar* vertexSource = (GLchar*)R"STRING(
#version 330 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_color;

out vec2 uv;
out vec4 color;

uniform mat4 mvp;

void main() {
	uv = in_uv;
	color = in_color;

	gl_Position = mvp * vec4(in_position, 0.0, 1.0);
}
	)STRING";

	GLchar* fragmentSource = (GLchar*)R"STRING(
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

	return CreateProgram(vertexSource, fragmentSource);
}

GLuint CreateImageProgram() {
	GLchar* vertexSource = (GLchar*)R"STRING(
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

	GLchar* fragmentSource = (GLchar*)R"STRING(
#version 330 core

in vec2 uv;

out vec4 out_color;

uniform sampler2D image;

void main() {
	out_color = texture(image, uv);
}   
	)STRING";

	return CreateProgram(vertexSource, fragmentSource);
}

void GfxInit(Arena* persist) {
	gfx = {};
	OSOpenGLInit();
	gfx.vertices = (float32*)ArenaAlloc(persist, sizeof(float32)*16*6*1024);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FRAMEBUFFER_SRGB); 
	glEnable(GL_SCISSOR_TEST);
	
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	gfx.quadProgram = CreateQuadProgram();
	gfx.textProgram = CreateTextProgram();
	gfx.imageProgram = CreateImageProgram();

	gfx.currentProgram = gfx.quadProgram;
	glUseProgram(gfx.currentProgram);
}

void GfxSetBackgroundColor(Color color) {
	gfx.backgorundColor = color;
}

void GfxSetFontAtlas(GLuint texture) {
	gfx.fontAtlas = texture;
}

void GfxBeginDrawing() {
	gfx.dim = OSGetWindowDimensions();
	glClearColor(gfx.backgorundColor.r, gfx.backgorundColor.g, gfx.backgorundColor.b, gfx.backgorundColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, gfx.dim.width, gfx.dim.height);
	gfx.currentProgram = 0;
}

void GfxFlushVertices() {
	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	if (gfx.currentProgram == gfx.quadProgram) {
		glBufferData(GL_ARRAY_BUFFER, gfx.quadCount*16*6*sizeof(float32), gfx.vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16*sizeof(float32), NULL);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16*sizeof(float32), (void*)(2*sizeof(float32)) );
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 16*sizeof(float32), (void*)(4*sizeof(float32)) );
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 16*sizeof(float32), (void*)(6*sizeof(float32)) );
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 16*sizeof(float32), (void*)(7*sizeof(float32)) );
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float32), (void*)(8*sizeof(float32)) );
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 16*sizeof(float32), (void*)(12*sizeof(float32)) );
	}

	if (gfx.currentProgram == gfx.textProgram) {
		glBufferData(GL_ARRAY_BUFFER, gfx.quadCount*8*6*sizeof(float32), gfx.vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float32), NULL);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float32), (void*)(2*sizeof(float32)) );
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float32), (void*)(4*sizeof(float32)) );
	}

	glDrawArrays(GL_TRIANGLES, 0, gfx.quadCount*6);
	glDeleteVertexArrays(1, &verticesHandle);
	gfx.quadCount = 0;
}

void PixelSpaceYIsDown(GLuint program) {
	float32 matrix[4][4] = {
		{2.0f/gfx.dim.width, 0, 0, -1},
		{0, -2.0f/gfx.dim.height, 0, 1},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	};
	glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, &matrix[0][0]);
}

void PixelSpaceYIsUp(GLuint program) {
	float32 matrix[4][4] = {
		{2.0f/gfx.dim.width, 0, 0, -1},
		{0, 2.0f/gfx.dim.height, 0, -1},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	};
	glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, &matrix[0][0]);
}

void GfxDrawQuad(Point2 pos, Dimensions2 dim, Color color1, float32 radius, float32 borderWidth, Color borderColor,
	Color color2) {
	if (gfx.currentProgram != gfx.quadProgram) {
		GfxFlushVertices();
		gfx.currentProgram = gfx.quadProgram;
		PixelSpaceYIsDown(gfx.quadProgram);		
	}
	glUseProgram(gfx.quadProgram);
	Point2 center = {pos.x+dim.width/2.f, pos.y+dim.height/2.f};
	Point2 p0 = pos;
	Point2 p1 = {pos.x + dim.width, pos.y + dim.height};
	float32 vertices[] = {
		p1.x, p0.y, center.x, center.y, dim.width, dim.height,	radius,	borderWidth, color2.r, color2.g, color2.b, color2.a, borderColor.r, borderColor.g, borderColor.b, borderColor.a,
		p0.x, p1.y, center.x, center.y, dim.width, dim.height,	radius,	borderWidth, color1.r, color1.g, color1.b, color1.a, borderColor.r, borderColor.g, borderColor.b, borderColor.a,
		p0.x, p0.y, center.x, center.y, dim.width, dim.height,	radius,	borderWidth, color2.r, color2.g, color2.b, color2.a, borderColor.r, borderColor.g, borderColor.b, borderColor.a,

		p0.x, p1.y, center.x, center.y, dim.width, dim.height,	radius,	borderWidth, color1.r, color1.g, color1.b, color1.a, borderColor.r, borderColor.g, borderColor.b, borderColor.a,
		p1.x, p1.y, center.x, center.y, dim.width, dim.height,	radius,	borderWidth, color1.r, color1.g, color1.b, color1.a, borderColor.r, borderColor.g, borderColor.b, borderColor.a,
		p1.x, p0.y, center.x, center.y, dim.width, dim.height,	radius,	borderWidth, color2.r, color2.g, color2.b, color2.a, borderColor.r, borderColor.g, borderColor.b, borderColor.a
	};
	memcpy(gfx.vertices + 16*6*gfx.quadCount, vertices, sizeof(vertices));
	gfx.quadCount++;
}

void GfxDrawGlyph(Point2 pos, Dimensions2 dim, Box2 crop, Color color) {
	if (gfx.currentProgram != gfx.textProgram) {
		GfxFlushVertices();
		gfx.currentProgram = gfx.textProgram;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gfx.fontAtlas);
		glUniform1i(glGetUniformLocation(gfx.textProgram, "atlas"), 0);
		PixelSpaceYIsDown(gfx.textProgram);
	}
	glUseProgram(gfx.textProgram);
	Point2 p0 = pos;
	Point2 p1 = {pos.x + dim.width, pos.y + dim.height};
	float32 vertices[] = {
		p1.x, p0.y, crop.x1, crop.y0, color.r, color.g, color.b, color.a,
		p0.x, p1.y, crop.x0, crop.y1, color.r, color.g, color.b, color.a,
		p0.x, p0.y, crop.x0, crop.y0, color.r, color.g, color.b, color.a,

		p0.x, p1.y, crop.x0, crop.y1, color.r, color.g, color.b, color.a,
		p1.x, p1.y, crop.x1, crop.y1, color.r, color.g, color.b, color.a,
		p1.x, p0.y, crop.x1, crop.y0, color.r, color.g, color.b, color.a
	};
	memcpy(gfx.vertices + 8*6*gfx.quadCount, vertices, sizeof(vertices));
	gfx.quadCount++;
}

void GfxDrawImage(Point2 pos, Dimensions2 dim, GLuint texture, Box2 crop) {
	GfxFlushVertices();
	glUseProgram(gfx.imageProgram);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(gfx.imageProgram, "image"), 0);
	PixelSpaceYIsDown(gfx.imageProgram);

	Point2 p0 = pos;
	Point2 p1 = {pos.x + dim.width, pos.y + dim.height};
	float32 vertices[] = {
		p1.x, p0.y, crop.x1, crop.y1,
		p0.x, p1.y, crop.x0, crop.y0,
		p0.x, p0.y, crop.x0, crop.y1,

		p0.x, p1.y, crop.x0, crop.y0,
		p1.x, p1.y, crop.x1, crop.y0,
		p1.x, p0.y, crop.x1, crop.y1
	};	

	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	glBufferData(GL_ARRAY_BUFFER, 4*6*sizeof(float32), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float32), NULL);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float32), (void*)(2*sizeof(float32)) );

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDeleteVertexArrays(1, &verticesHandle);
}

void GfxEndDrawing() {
	GfxFlushVertices();
	GfxSwapBuffers();
}

void GfxCropScreen(int32 x, int32 y, int32 width, int32 height) {
	GfxFlushVertices();
	// NOTE: y is down
	glScissor(x, gfx.dim.height - y - height, width, height);
}

void GfxClearCrop() {
	GfxFlushVertices();
	glScissor(0, 0, gfx.dim.width, gfx.dim.height);
}
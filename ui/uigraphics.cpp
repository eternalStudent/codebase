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
	GLuint shapeProgram;
	GLuint currentProgram;
	float32* vertices;
	int32 quadCount;
	Color backgorundColor;
	GLuint fontAtlas;
} gfx;

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
	gfx.shapeProgram = CreateShapeProgram();

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
	glScissor(0, 0, gfx.dim.width, gfx.dim.height);
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

	if (gfx.currentProgram == gfx.shapeProgram) {
		glBufferData(GL_ARRAY_BUFFER, gfx.quadCount*6*6*sizeof(float32), gfx.vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float32), NULL);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6*sizeof(float32), (void*)(2*sizeof(float32)) );
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

void GfxDrawCurve(Point2 p0, Point2 p1, Point2 p2, Point2 p3, float32 thick, Color color) {
	if (gfx.currentProgram != gfx.shapeProgram) {
		GfxFlushVertices();
		gfx.currentProgram = gfx.shapeProgram;
		PixelSpaceYIsDown(gfx.shapeProgram);
	}
	glUseProgram(gfx.shapeProgram);
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
		
		Point2 p[4] = {
			{prev.x - radius.x, prev.y - radius.y},
			{prev.x + radius.x, prev.y + radius.y},
			{current.x - radius.x, current.y - radius.y},
			{current.x + radius.x, current.y + radius.y}
		};
		
		float32 vertices[] = {
			p[0].x, p[0].y, color.r, color.g, color.b, color.a,
			p[1].x, p[1].y, color.r, color.g, color.b, color.a,
			p[2].x, p[2].y, color.r, color.g, color.b, color.a,
		
			p[3].x, p[3].y, color.r, color.g, color.b, color.a,
			p[2].x, p[2].y, color.r, color.g, color.b, color.a,
			p[1].x, p[1].y, color.r, color.g, color.b, color.a
		};
		memcpy(gfx.vertices + 6*6*gfx.quadCount, vertices, sizeof(vertices));
		gfx.quadCount++;
		prev = current;
	}
#undef SEGMENTS
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
struct {
	GLuint drawImage;
	GLuint drawShape;
	Window window;
	Dimensions2i dimensions;
	union {
		Dimensions2 invdimensions;
		struct {float32 iwidth, iheight;};
	};
	uint32 rgba;
	Arena* arena;
} opengl;

GLuint CreateProgram(GLchar* vertexSource, GLchar* fragmentSource) {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLchar* vertexSourceArr[1] = { vertexSource };
	glShaderSource(vertexShader, 1, vertexSourceArr, NULL);
	glCompileShader(vertexShader);
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		Log("fail to compile vertex shader");
		Log(infoLog);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLchar* fragmentSourceArr[1] = { fragmentSource };
	glShaderSource(fragmentShader, 1, fragmentSourceArr, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		Log("fail to compile fragment shader");
		Log(infoLog);
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		Log("fail to link program");
		Log(infoLog);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

GLuint OpenGLGenerateTexture(Image image, GLint filter) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

void OpenGLInit(Arena* arena, Window window, Dimensions2i dimensions, uint32 rgba) {
	OsOpenGLInit(window);

	opengl.window = window;
	opengl.dimensions = dimensions;
	opengl.invdimensions = {1.0f / opengl.dimensions.width, 1.0f / opengl.dimensions.height};
	opengl.rgba = rgba;
	opengl.arena = arena;

	GLchar* vertexSource = (GLchar*)R"STRING(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;

void main()
{
    TexCoords = vertex.zw;
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
}
	)STRING";

	GLchar* fragmentSource = (GLchar*)R"STRING(
#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D image;

void main()
{    
    color = texture(image, TexCoords);
} 
	)STRING";
	opengl.drawImage = CreateProgram(vertexSource, fragmentSource);

	vertexSource = (GLchar*)R"STRING(
#version 330 core
layout (location = 0) in vec2 vertex;

void main()
{
    gl_Position = vec4(vertex, 0.0, 1.0);
}
	)STRING";

	fragmentSource = (GLchar*)R"STRING(
#version 330 core
out vec4 FragColor;
  
uniform vec4 color;

void main()
{
    FragColor = color;
}   
	)STRING";
	opengl.drawShape = CreateProgram(vertexSource, fragmentSource);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

void OpenGLUpdateDimensions(Dimensions2i dimensions){
	opengl.dimensions = dimensions;
	opengl.invdimensions = {1.0f / opengl.dimensions.width, 1.0f / opengl.dimensions.height};
}

void OpenGLClearScreen() {
	glViewport(0, 0, opengl.dimensions.width, opengl.dimensions.height);
	Color color = RgbaToColor(opengl.rgba);
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

Point2 AdjustCoordinates(Point2 p){
	return {(2.0f * p.x * opengl.iwidth) - 1, (2.0f * p.y * opengl.iheight) - 1};
}

void OpenGLDrawImage(GLuint texture, Box2 crop, Box2 pos) {
	Point2 p0 = AdjustCoordinates(pos.p0);
	Point2 p1 = AdjustCoordinates(pos.p1);

	float32 bottomTex = crop.y0;
	float32 topTex = crop.y1;
	float32 rightTex = crop.x1;
	float32 leftTex = crop.x0;

	float32 vertices[] = {
		// pos            // tex
		p1.x, p0.y,       rightTex, bottomTex,   // bottom-right
		p0.x, p1.y,       leftTex, topTex,       // top-left
		p0.x, p0.y,       leftTex, bottomTex,    // bottom-left

		p0.x, p1.y,       leftTex, topTex,       // top-left
		p1.x, p1.y,       rightTex, topTex,      // top-right
		p1.x, p0.y,       rightTex, bottomTex    // bottom-right
	};

	GLsizeiptr size = sizeof(vertices);
	GLuint buffersHandle;
	glGenBuffers(1, &buffersHandle);
	glBindBuffer(GL_ARRAY_BUFFER, buffersHandle);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float32), NULL);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUseProgram(opengl.drawImage);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(opengl.drawImage, "image"), 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDeleteVertexArrays(1, &verticesHandle);
	glDeleteBuffers(1, &buffersHandle);
}

void OpenGLDrawBox2(uint32 rgba, Box2 crop, Box2 pos) {
	Point2 p0 = AdjustCoordinates(pos.p0);
	Point2 p1 = AdjustCoordinates(pos.p1);

	float32 vertices[] = {
		p0.x, p0.y,
		p0.x, p1.y,
		p1.x, p1.y,
		p1.x, p0.y
	};

	GLsizeiptr size = sizeof(vertices);
	GLuint buffersHandle;
	glGenBuffers(1, &buffersHandle);
	glBindBuffer(GL_ARRAY_BUFFER, buffersHandle);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float32), NULL);

	GLint location = glGetUniformLocation(opengl.drawShape, "color");
	Color color = RgbaToColor(rgba);
	glUseProgram(opengl.drawShape);
	glUniform4f(location, color.r, color.g, color.b, color.a);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDeleteVertexArrays(1, &verticesHandle);
	glDeleteBuffers(1, &buffersHandle);
}

void OpenGLDrawLine(uint32 rgba, float32 lineWidth, Line2 line) {
	ArenaAlign(opengl.arena, 4);
	float32* vertices = (float32*)ArenaAlloc(opengl.arena, 2*line.count*sizeof(float32));

	for (int32 i = 0; i<line.count; i++) {
		Point2 p = AdjustCoordinates(line.points[i]);
		vertices[i*2  ] = p.x;
		vertices[i*2+1] = p.y;
	}

	GLuint buffersHandle;
	glGenBuffers(1, &buffersHandle);
	glBindBuffer(GL_ARRAY_BUFFER, buffersHandle);
	glBufferData(GL_ARRAY_BUFFER, 2*line.count*sizeof(float32), vertices, GL_STATIC_DRAW);

	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float32), NULL);

	GLint location = glGetUniformLocation(opengl.drawShape, "color");
	Color color = RgbaToColor(rgba);
	glUseProgram(opengl.drawShape);
	glUniform4f(location, color.r, color.g, color.b, color.a);
	glLineWidth(lineWidth ? lineWidth : 1);

	glDrawArrays(GL_LINE_STRIP, 0, line.count);

	glDeleteVertexArrays(1, &verticesHandle);
	glDeleteBuffers(1, &buffersHandle);
}

void OpenGLDrawCircle(uint32 rgba, float32 lineWidth, Sphere2 circle, int32 startAngle, int32 endAngle) {
	int32 count = (endAngle-startAngle+1);
	float32 vertices[362*2];
	for (int32 i=0; i<=count; i++) {
		int32 deg = i+startAngle;
		float32 rad = deg*DEG_TO_RAD();
		Point2 p = AdjustCoordinates(PointOnSphere(circle, rad));

		vertices[i*2  ] = p.x;
		vertices[i*2+1] = p.y;
	}

	GLuint buffersHandle;
	glGenBuffers(1, &buffersHandle);
	glBindBuffer(GL_ARRAY_BUFFER, buffersHandle);
	glBufferData(GL_ARRAY_BUFFER, 2*count*sizeof(float32), vertices, GL_STATIC_DRAW);

	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float32), NULL);

	GLint location = glGetUniformLocation(opengl.drawShape, "color");
	Color color = RgbaToColor(rgba);
	glUseProgram(opengl.drawShape);
	glUniform4f(location, color.r, color.g, color.b, color.a);
	glLineWidth(lineWidth ? lineWidth : 1);

	glDrawArrays(GL_LINE_STRIP, 0, count);

	glDeleteVertexArrays(1, &verticesHandle);
	glDeleteBuffers(1, &buffersHandle);
}

void OpenGLDrawDisc(uint32 rgba, Sphere2 disc, int32 startAngle, int32 endAngle) {
	int32 count = (endAngle-startAngle+1);
	float32 vertices[363*2];
	Point2 center = AdjustCoordinates(disc.center);
	vertices[0] = center.x;
	vertices[1] = center.y;
	for (int32 i=0; i<=count; i++) {
		int32 j = (i+1)*2;
		int32 deg = i+startAngle;
		float32 rad = deg*DEG_TO_RAD();
		Point2 p = AdjustCoordinates(PointOnSphere(disc, rad));
		vertices[j  ] = p.x;
		vertices[j+1] = p.y;
	}

	GLuint buffersHandle;
	glGenBuffers(1, &buffersHandle);
	glBindBuffer(GL_ARRAY_BUFFER, buffersHandle);
	glBufferData(GL_ARRAY_BUFFER, 2*(count+1)*sizeof(float32), vertices, GL_STATIC_DRAW);

	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float32), NULL);

	GLint location = glGetUniformLocation(opengl.drawShape, "color");
	Color color = RgbaToColor(rgba);
	glUseProgram(opengl.drawShape);
	glUniform4f(location, color.r, color.g, color.b, color.a);

	glDrawArrays(GL_TRIANGLE_FAN, 0, count+1);

	glDeleteVertexArrays(1, &verticesHandle);
	glDeleteBuffers(1, &buffersHandle);
}

void OpenGLDrawCurve(uint32 rgba, float32 lineWidth, Point2 p0, Point2 p1, Point2 p2, Point2 p3) {
	float32 vertices[257*2];

	float32 fraction = LoadExp(-8);
	for (int32 i = 0; i<=256; i++) {
		float32 t = i*fraction;
		Point2 p = AdjustCoordinates(Cerp2(t, p0, p1, p2, p3));
		vertices[i*2  ] = p.x;
		vertices[i*2+1] = p.y;
	}

	GLuint buffersHandle;
	glGenBuffers(1, &buffersHandle);
	glBindBuffer(GL_ARRAY_BUFFER, buffersHandle);
	glBufferData(GL_ARRAY_BUFFER, 2*257*sizeof(float32), vertices, GL_STATIC_DRAW);

	GLuint verticesHandle;
	glGenVertexArrays(1, &verticesHandle);
	glBindVertexArray(verticesHandle);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float32), NULL);

	GLint location = glGetUniformLocation(opengl.drawShape, "color");
	Color color = RgbaToColor(rgba);
	glUseProgram(opengl.drawShape);
	glUniform4f(location, color.r, color.g, color.b, color.a);
	glLineWidth(lineWidth ? lineWidth : 1);

	glDrawArrays(GL_LINE_STRIP, 0, 257);

	glDeleteVertexArrays(1, &verticesHandle);
	glDeleteBuffers(1, &buffersHandle);
}
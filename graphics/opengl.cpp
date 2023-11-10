#if defined(_WIN32)
#  include "win32opengl.cpp"
#  define OSOpenGLInit			Win32OpenGLInit
#  define OSOpenGLSwapBuffers	Win32OpenGLSwapBuffers
#endif

#if defined(__gnu_linux__)
#  include "linuxopengl.cpp"
#  define OSOpenGLInit			LinuxOpenGLInit
#  define OSOpenGLSwapBuffers	LinuxOpenGLSwapBuffers
#endif

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
		LOG("fail to compile vertex shader: %s", infoLog);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLchar* fragmentSourceArr[1] = { fragmentSource };
	glShaderSource(fragmentShader, 1, fragmentSourceArr, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		LOG("fail to compile fragment shader: %s", infoLog);
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		LOG("fail to link program: %s:", infoLog);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

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

GLuint CreateShapeProgram() {
	GLchar* vertexSource = (GLchar*)R"STRING(
#version 330 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_color;

out vec4 color;

uniform mat4 mvp;

void main() {
	color = in_color;

	gl_Position = mvp * vec4(in_position, 0.0, 1.0);
}
	)STRING";

	GLchar* fragmentSource = (GLchar*)R"STRING(
#version 330 core

in vec4 color;

out vec4 out_color;

void main() {
	out_color = color;
}   
	)STRING";

	return CreateProgram(vertexSource, fragmentSource);
}

GLuint OpenGLGenerateTexture(Image image, GLint filter) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	static GLenum formats[] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
	static GLenum internalFormats[] = {GL_RED, GL_RG, GL_SRGB, GL_SRGB8_ALPHA8};
	glTexImage2D(
		GL_TEXTURE_2D, 
		0, 
		internalFormats[image.channels - 1], 
		image.width, 
		image.height, 
		0, 
		formats[image.channels - 1], 
		GL_UNSIGNED_BYTE, 
		image.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}
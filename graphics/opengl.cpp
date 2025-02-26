#if defined(OS_WINDOWS)
#  include "win32opengl.cpp"
#  define OSOpenGLInit			Win32OpenGLInit
#  define OSOpenGLSwapBuffers	Win32OpenGLSwapBuffers
#endif

#if defined(OS_LINUX)
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
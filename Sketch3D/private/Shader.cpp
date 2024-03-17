

#include "Strings.hpp"
#include "Buffer.hpp"
#include "LocalConnection.hpp"
#include "Shader.hpp"
#include "GraphicsApi.hpp"

#include <cstdio>

using namespace tp;

RenderShader::RenderShader() {
	programm = 0;
	VertexShaderID = 0;
	FragmentShaderID = 0;
	GeometryShaderID = 0;
}

void RenderShader::vert_bind_source(const char* vert_src) {
	VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	compile_shader(vert_src, VertexShaderID);
}

void RenderShader::frag_bind_source(const char* frag_src) {
	FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	compile_shader(frag_src, FragmentShaderID);
}

void RenderShader::geom_bind_source(const char* geom_src) {
	GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
	compile_shader(geom_src, GeometryShaderID);
}

void RenderShader::compile() {
	GLint Result = GL_FALSE;
	int InfoLogLength;

	programm = glCreateProgram();
	glAttachShader(programm, VertexShaderID);
	if (GeometryShaderID) glAttachShader(programm, GeometryShaderID);
	glAttachShader(programm, FragmentShaderID);
	glLinkProgram(programm);

	// Check the program
	glGetProgramiv(programm, GL_LINK_STATUS, &Result);
	glGetProgramiv(programm, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if (InfoLogLength > 0) {
		Buffer<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(programm, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(programm, VertexShaderID);
	glDetachShader(programm, FragmentShaderID);
	if (GeometryShaderID) glDetachShader(programm, GeometryShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);
	if (GeometryShaderID) glDeleteShader(GeometryShaderID);
}

bool RenderShader::compile_shader(const char* ShaderCode, uint4 ShaderID) {
	GLint Result = GL_FALSE;
	int InfoLogLength;

	char const* SourcePointer = ShaderCode;
	glShaderSource(ShaderID, 1, &SourcePointer, NULL);
	glCompileShader(ShaderID);

	// Check Shader
	glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if (InfoLogLength > 0) {
		Buffer<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(ShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	return Result;
}


void RenderShader::load(const char* pvert, const char* pgeom, const char* pfrag, bool paths) {

	// Create the shaders
	VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	if (pgeom) GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
	else GeometryShaderID = 0;

	GLint Result = GL_FALSE;
	int InfoLogLength = 0;

	if (paths) {

		String content;

		auto loadFile = [&](const char* path) {
			LocalConnection file;
			if (!file.connect(LocalConnection::Location(path), LocalConnection::Type(true))) {
				content = "";
				return false;
			}

			content.resize(file.size());
			file.readBytes(content.write(), content.size());
			return true;
		};

		printf("Compiling shader : %s\n", pvert);
		if (loadFile(pvert)) {
			compile_shader(content.read(), VertexShaderID);
		}

		if (GeometryShaderID) {
			printf("Compiling shader : %s\n", pgeom);
			if (loadFile(pgeom)) {
				compile_shader(content.read(), GeometryShaderID);
			}
		}

		printf("Compiling shader : %s\n", pfrag);
		if (loadFile(pfrag)) {
			compile_shader(content.read(), FragmentShaderID);
		}
	}
	else {
		compile_shader(pvert, VertexShaderID);
		if (GeometryShaderID) {
			compile_shader(pgeom, GeometryShaderID);
		}
		compile_shader(pfrag, FragmentShaderID);
	}

	compile();
}

RenderShader::RenderShader(const char* vert, const char* geom, const char* frag, bool paths) {
	load(vert, geom, frag, paths);
}

void RenderShader::bind() {
	glUseProgram(programm);
}

GLuint RenderShader::getu(const char* uid) {
	return glGetUniformLocation(programm, uid);
}

void RenderShader::unbind() {
	glUseProgram(0);
}

RenderShader::~RenderShader() {
	glDeleteProgram(programm);
}
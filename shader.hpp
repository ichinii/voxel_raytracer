#pragma once

#include <GL/glew.h>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>

static GLuint loadShaderFromSourceCode(GLenum type, const char* sourcecode, int length)
{
	GLuint shaderId = glCreateShader(type);

	glShaderSource(shaderId, 1, &sourcecode, &length);
	glCompileShader(shaderId);

	GLint isCompiled = 0;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);

		auto errorLog = std::make_unique<GLchar[]>(maxLength);
		glGetShaderInfoLog(shaderId, maxLength, &maxLength, &errorLog[0]);

		std::cout << "Error compiling " << std::endl
			<< &errorLog[0] << std::endl;
		glDeleteShader(shaderId); // Don't leak the shader.
		return 0;
	}

	return shaderId;
}

static GLuint loadShaderFromFile(GLenum type, const char* filepath)
{
	std::ifstream fstream;
	fstream.open(filepath);

	if (!fstream.is_open())
	{
		std::cout << "Unable to open file '" << filepath << "'" << std::endl;
		return 0;
	}

	std::stringstream sstream;
	std::string line;
	while (std::getline(fstream, line))
		sstream << line << '\n';
	line = sstream.str();

	GLuint shaderId = loadShaderFromSourceCode(type, line.c_str(), line.length());
	if (!shaderId)
		std::cout << "...with filepath '" << filepath << "'"; 

	return shaderId;
}

struct shader_load_data_t {
	GLenum type;
	const char* filepath;
};

static GLuint createProgram(std::vector<shader_load_data_t> shader_load_data)
{
	GLuint program;
	program = glCreateProgram();

	std::vector<GLuint> shaders;
	shaders.reserve(shader_load_data.size());
	for (auto& s : shader_load_data) {
		GLuint shader = loadShaderFromFile(s.type, s.filepath);
		shaders.push_back(shader);
		glAttachShader(program, shader);
	}

	glLinkProgram(program);

	for (auto& s : shaders)
		glDeleteShader(s);

	return program;
}

#include <iostream>
#include <fstream>
#include <sstream>

#include "util.hpp"
#include "Shader.h"

Program::Program(const GLchar* vertexPath, const GLchar* fragmentPath) {
    // 1. Получаем исходный код шейдера из filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Удостоверимся, что ifstream объекты могут выкидывать исключения
    vShaderFile.exceptions(std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::badbit);
    try
    {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch(std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }

    const GLchar *vertexShaderSource = vertexCode.c_str();
    const GLchar *fragmentShaderSource = fragmentCode.c_str();

    Shader fragmentShader(GL_VERTEX_SHADER, vertexShaderSource),
           vertexShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    this->program = glCreateProgram();
    glAttachShader(this->program, vertexShader.id());
    glAttachShader(this->program, fragmentShader.id());
    glLinkProgram(this->program);

//    LOG(vertexShaderSource);
//    LOG(fragmentShaderSource);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(this->program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(this->program, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
}

bool Program::Shader::checkCompilation(GLuint shaderProgram) {
    GLint success;
    glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "COMPILE::PROGRAM::ERROR [" << this->shaderFile << "]\n" << infoLog << std::endl;
    }

    return success;
}

void Program::use() const {
    glUseProgram(this->program);
}

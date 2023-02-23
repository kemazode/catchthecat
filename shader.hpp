#ifndef SHADER_HPP
#define SHADER_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

class Program {
    GLuint program;

public:    
    class Shader {
        GLuint shaderId;
        const std::string shaderFile;
        bool checkCompilation(GLuint shaderProgram);
    public:
        Shader(GLuint type, const GLchar* src) : shaderId(glCreateShader(type)), shaderFile(src)
        {
            glShaderSource(shaderId, 1, &src, nullptr);
            glCompileShader(shaderId);
            checkCompilation(shaderId);
        }

        GLuint id() const { return shaderId; }
        ~Shader() {
            glDeleteShader(shaderId);
        }
    };

    void setUniform(const char* name, float value) {
        GLint uniformId = glGetUniformLocation(this->program, name);

        glUseProgram(this->program);
        glUniform1f(uniformId, value);
    }
    void setUniform(const char* name, const glm::mat4x4& mat) {
        GLint uniformId = glGetUniformLocation(this->program, name);

        glUseProgram(this->program);
        glUniformMatrix4fv(uniformId, 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setUniform(const char* name, int value) {
        GLint uniformId = glGetUniformLocation(this->program, name);

        glUseProgram(this->program);
        glUniform1i(uniformId, value);
    }

    void setUniform(const char* name, const glm::vec3& vec) {
        GLint uniformId = glGetUniformLocation(this->program, name);

        glUseProgram(this->program);
        glUniform3fv(uniformId, 1, glm::value_ptr(vec));
    }

    void setUniform(const char* name, bool vec) {
        GLint uniformId = glGetUniformLocation(this->program, name);

        glUseProgram(this->program);
        glUniform1i(uniformId, vec);
    }

//    void setUniform(const char* name, unsigned char* value) {
//        GLint uniformId = glGetUniformLocation(this->program, name);

//        glUseProgram(this->program);
//        glUniform(uniformId, value);
//    }

    Program(const GLchar* vectorPath, const GLchar* fragmentPath);
    void use() const;
    GLuint get() const { return this->program; }
};

#endif // SHADER_HPP

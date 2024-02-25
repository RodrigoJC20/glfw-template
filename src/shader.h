#ifndef SHADER_H
#define SHADER_H

#include <glad/gl.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
    public:
        unsigned int ID;

        Shader(const char* vertexPath, const char* fragmentPath) {
            // 1. Retrieve the vertex and fragment shaders source code
            std::string vertexSource;
            std::string fragmentSource;
            std::ifstream vShaderFile;
            std::ifstream fShaderFile;

            vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try {
                // Openn files
                vShaderFile.open(vertexPath);
                fShaderFile.open(fragmentPath);
                std::stringstream vShaderStream, fShaderStream;

                vShaderStream << vShaderFile.rdbuf();
                fShaderStream << fShaderFile.rdbuf();

                vShaderFile.close();
                fShaderFile.close();

                vertexSource = vShaderStream.str();
                fragmentSource = fShaderStream.str();

            } catch(std::ifstream::failure e) {
                std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
            }

            const char* vShaderCode = vertexSource.c_str();
            const char* fShaderCode = fragmentSource.c_str();

            // 2. Compile shaders
            unsigned int vertex, fragment;
            compileShader(vertex, vShaderCode, true);
            compileShader(fragment, fShaderCode, false);

            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            glLinkProgram(ID);

            int success;
            char infoLog[512];
            glGetProgramiv(ID, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(ID, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            }

            glDeleteShader(vertex);
            glDeleteShader(fragment);
        }

        void use() {
            glUseProgram(ID);
        }

        void setBool(const std::string &name, bool value) const {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
        }

        void setInt(const std::string &name, int value) const {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
        }

        void setFloat(const std::string &name, float value) const {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
        }

        void deleted() {
            glDeleteProgram(ID);
        }
    private:
        void compileShader(unsigned int& shader, const char* sourceCode, bool vertex) {
            shader = glCreateShader(vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
            glShaderSource(shader, 1, &sourceCode, NULL);
            glCompileShader(shader);

            int success;
            char infoLog[512];
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::" 
                            << (vertex ? "VERTEX" : "FRAGMENT")
                            << "COMPILATION_FAILED\n" 
                            << infoLog << std::endl;
            }
        }
};

#endif

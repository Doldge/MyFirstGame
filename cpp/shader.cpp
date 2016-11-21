// shader stuff.
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "shader.hpp"

GLuint loadShaders(const char * vertex_file_path, const char * fragment_file_path)
{
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vertex_shader_code;
    std::ifstream vertex_shader_stream(vertex_file_path, std::ios::in);
    if ( vertex_shader_stream.is_open() )
    {
        std::string line = "";
        while ( getline(vertex_shader_stream, line) )
        {
            vertex_shader_code += "\n" + line;  // TODO \r\n ?
        }
        vertex_shader_stream.close();
    } else {
        //bitch about it
        fprintf(stderr, "Failed to find vertex Shader");
        getchar();
        return 0;
    }
   
    std::string fragment_shader_code;
    std::ifstream fragment_shader_stream(fragment_file_path, std::ios::in);
    if ( fragment_shader_stream.is_open() )
    {
        std::string line = "";
        while ( getline(fragment_shader_stream, line) )
        {
            fragment_shader_code += "\n" + line;
        }
        fragment_shader_stream.close();
    } else {
        // bitch about it
        fprintf( stderr, "Failed to fragment Shader");
        getchar();
        return 0;
    }

    GLint result = GL_FALSE;
    int info_log_length;
    
    fprintf( stdout, "Compiling Shader: %s\n", vertex_file_path);
    const char * vertex_source_pointer = vertex_shader_code.c_str();
    glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, NULL);
    glCompileShader(vertex_shader_id);

    glGetShaderiv( vertex_shader_id, GL_COMPILE_STATUS, &result );
    glGetShaderiv( vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length );
    if ( info_log_length > 0 )
    {
        std::vector<char> vertex_shader_error_message(info_log_length+1); // +1 for null terminator.
        glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, &vertex_shader_error_message[0]);
        fprintf( stderr, "%s\n", &vertex_shader_error_message[0] );
    }

    fprintf(stdout, "Compiling Shader: %s\n", fragment_file_path);
    const char * fragment_source_pointer = fragment_shader_code.c_str();
    glShaderSource(fragment_shader_id, 1, &fragment_source_pointer, NULL);
    glCompileShader(fragment_shader_id);

    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result );
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length );
    if ( info_log_length > 0 )
    {
        std::vector<char> fragment_shader_error_message(info_log_length+1);
        glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, &fragment_shader_error_message[0]);
        fprintf( stderr, "%s\n", &fragment_shader_error_message[0] );
    }

    fprintf( stdout, "Linking the program\n" );
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if ( info_log_length > 0 )
    {
        std::vector<char> program_link_error_message(info_log_length+1);
        glGetProgramInfoLog(program_id, info_log_length, NULL, &program_link_error_message[0]);
        fprintf( stderr, "%s\n", &program_link_error_message[0] );
    }

    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, fragment_shader_id);
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    
    return program_id;
};

// Standard Includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

// Mutha-fuckin, OpenGL, bitches!
#include <GL/glew.h>
#include <GLFW/glfw3.h>
GLFWwindow * window;
#include <glm/glm.hpp>

// C++ shit that I don't really understand.
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

//Local includes
#include "shader.hpp"

//In True linux Fashion, a return 0 = success. everything else is an error.


int main() {
    //Mutha-fuckin Main function, bitches!
    if ( ! glfwInit() ) {
        //One of our openGL starter calls failed. Fuck everything!
        fprintf(stderr, "Failed to initialize GLFW.");
        return -1;
    };

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1024, 768, "Game Client", NULL, NULL);
    if ( window == NULL ) {  // Better to be explicit..
        fprintf(stderr, "Failed to initialize Window");
        glfwTerminate();
        return -2;
    }
    glfwMakeContextCurrent(window);
    // Bug, sometimes the render doesn't take. I think it's becuse X11 is async, 
    // so sometimes the code makes it to the draw before we've actually made a window or something.
    // end result is we get a black screen. Adding a sleep for about 0.1 sec is enough to fix the issue.
    usleep(80000); //I suspect this is rounding up to 100,000 usec (100msec), as anything less than 80000 doesn't fix the issue.

    if ( glewInit() != GLEW_OK ) {
        fprintf(stderr, "Failed to initialize Glew");
        glfwTerminate();
        return -3;
    };

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    
    GLuint vertex_array_id;
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);

    GLuint program_id = loadShaders( "vertex.glsl", "fragment.glsl" );
    
    static const GLfloat vertex_buffer_data[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

    //main loop.
    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program_id);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *) 0
        );
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while ( glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) != 0 );
    
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteVertexArrays(1, &vertex_array_id);
    glDeleteProgram(program_id);
    glfwTerminate();

    return 0;
};

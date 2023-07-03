#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

int
slurp(FILE *file, char **buf)
{
    char *tmp;
    size_t size = 64, read = 0, n;
    *buf = malloc(size);
    while ((n = fread(*buf + read, 1, size - read, file)) > 0) {
        read += n;
        if (read == size) {
            size *= 2;
            if ((tmp = realloc(*buf, size)) == NULL) {
                fprintf(stderr, "Failed to reallocate!\n");
                free(*buf);
                *buf = NULL;
                return 0;
            }
            *buf = tmp;
        }
    }
    return read;
}

int
shaderSourcePath(GLuint shader, char *path)
{
    GLint length;
    char *source;
    FILE *file;

    file = fopen(path, "r");
    length = slurp(file, &source);
    glShaderSource(shader, 1, (char const* const*) &source, &length);
    free(source);
    return length;
}

int
main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "SDL OpenGL Window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL
    );

    SDL_GLContext context = SDL_GL_CreateContext(window);

    // Initialize GLEW
    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK)
    {
        printf("Failed to initialize GLEW: %s\n", glewGetErrorString(glewInitResult));
        return 1;
    }

    // Compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    shaderSourcePath(fragmentShader, "./fragment.glsl");
    glCompileShader(fragmentShader);

    // Check for shader compilation errors
    GLint compileStatus;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("Failed to compile fragment shader: %s\n", infoLog);
        return 1;
    }

    // Create shader program and attach the fragment shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for shader program linking errors
    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
    {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Failed to link shader program: %s\n", infoLog);
        return 1;
    }

    // Use the shader program
    glUseProgram(shaderProgram);

    // Set up the OpenGL viewport
    glViewport(0, 0, 800, 600);

    // Main loop
    SDL_Event event;
    int quit = 0;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = 1;
            }
        }

        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the triangle
        glBegin(GL_TRIANGLES);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);
        glEnd();

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteShader(fragmentShader);
    glDeleteProgram(shaderProgram);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

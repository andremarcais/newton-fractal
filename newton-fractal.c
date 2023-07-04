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

void
compileLinkShaders(GLuint shaderProgram)
{
    static struct shader {
        GLenum type;
        char* path;
        GLuint id;
    } shaders[] = {
        { GL_VERTEX_SHADER, "./vertex.glsl" },
        { GL_FRAGMENT_SHADER, "./fragment.glsl" },
        { 0, NULL },
    };

    for (struct shader *shader = shaders; shader->path; ++shader) {
        shader->id = glCreateShader(shader->type);
        shaderSourcePath(shader->id, shader->path);
        glCompileShader(shader->id);

        // Check for shader compilation errors
        GLint compileStatus;
        glGetShaderiv(shader->id, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus != GL_TRUE) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader->id, 512, NULL, infoLog);
            fprintf(stderr, "Failed to compile %s: %s\n", shader->path, infoLog);
            exit(1);
        }

        glAttachShader(shaderProgram, shader->id);
        glDeleteShader(shader->id); // Contrary to the name, it just --refcount
    }

    GLint linkStatus;
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "Failed to link shader program: %s\n", infoLog);
        exit(1);
    }
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

    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW: %s\n", glewGetErrorString(glewInitResult));
        return 1;
    }

    GLuint shaderProgram = glCreateProgram();
    compileLinkShaders(shaderProgram);

    char *attrNames[] = { "vertPos", "vertRoot" };
    const GLint nverts = 6;
    GLfloat bufdat[] = {
        // vertex buffer
        1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        // complex roots
        0.0f, 1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, -1.0f, 0.0f,
    };
    GLuint bufobjs[2], attrs[2], bufsz[2] = { 3, 4 };
    glGenBuffers(2, bufobjs);
    for (int i = 0, o = 0; i < 2; o += nverts*bufsz[i], ++i) {
        if ((attrs[i] = glGetAttribLocation(shaderProgram, attrNames[i])) == -1) {
            fprintf(stderr, "Failed to get location of resource %s\n", attrNames[i]);
            return 1;
        }
        glBindBuffer(GL_ARRAY_BUFFER, bufobjs[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bufdat[0])*bufsz[i]*nverts, bufdat+o, GL_STATIC_DRAW);
        // Probably should be in draw, it it's stateful so who cares!
        glEnableVertexAttribArray(attrs[i]);
        glVertexAttribPointer(attrs[i], bufsz[i], GL_FLOAT, GL_FALSE, 0, 0);
    }

    glUseProgram(shaderProgram);

    glViewport(0, 0, 800, 600);

    SDL_Event event;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the triangle
        glDrawArrays(GL_TRIANGLES, 0, nverts);

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    glDeleteProgram(shaderProgram);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

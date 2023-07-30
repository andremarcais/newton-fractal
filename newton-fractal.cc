#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

int
slurp(FILE *file, char **buf)
{
    char *tmp;
    size_t size = 64, read = 0, n;
    *buf = (char*) malloc(size);
    while ((n = fread(*buf + read, 1, size - read, file)) > 0) {
        read += n;
        if (read == size) {
            size *= 2;
            if ((tmp = (char*) realloc(*buf, size)) == NULL) {
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
shaderSourcePath(GLuint shader, const char *path)
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
    struct shader {
        const GLenum type;
        const char* path;
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


inline glm::vec3
fromWinCoord(int x, int y)
{
    // TODO unhardcode window dimensions
    glm::vec3 tmp(x - 400.0f, -y + 400.0f, 0.0f);
    return (2.0f/800)*tmp;
}

int
main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "SDL OpenGL Window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 800,
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

    const char *attrNames[] = { "vertPos", "vertRoot" };
    const GLint nverts = 6;
    const GLuint bufsz[2] = { 3 };
    const GLfloat bufdat[3*nverts] = {
        // vertex buffer
        1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
    };
    GLuint bufobjs[2];
    GLint attrs[2];
    glGenBuffers(2, bufobjs);
    for (int i = 0, o = 0; i < 1; o += nverts*bufsz[i], ++i) {
        if ((attrs[i] = glGetAttribLocation(shaderProgram, attrNames[i])) == -1) {
            fprintf(stderr, "warn: Failed to get location of resource %s\n", attrNames[i]);
            continue;
        }
        glBindBuffer(GL_ARRAY_BUFFER, bufobjs[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(bufdat[0])*bufsz[i]*nverts, bufdat+o, GL_STATIC_DRAW);
        // Probably should be in draw, but it's stateful so who cares!
        glEnableVertexAttribArray(attrs[i]);
        glVertexAttribPointer(attrs[i], bufsz[i], GL_FLOAT, GL_FALSE, 0, 0);
    }

    GLint viewTransAttr = glGetUniformLocation(shaderProgram, "viewTrans");
    glm::mat4x4 viewTrans(1.0f);
    //float zoom = 1.0;
    if (viewTransAttr == -1) {
        fprintf(stderr, "warn: Failed to get location of resource viewTrans\n");
    }

    glUseProgram(shaderProgram);

    glViewport(0, 0, 800, 800);

    SDL_Event event;
    const struct timespec sleeptime = { 0, 16'666'666 };
    int quit = 0;
    while (!quit) {
        SDL_WaitEvent(NULL);
        while (SDL_PollEvent(&event)) {
            GLfloat tmp;
            glm::vec3 tmpVec;
            switch (event.type) {
            case SDL_MOUSEMOTION:
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    viewTrans = glm::translate(
                        viewTrans,
                        // TODO unhardcode window dimensions
                        (2.0f/800)*glm::vec3(-event.motion.xrel, event.motion.yrel, 0.0)
                    );
                }
                break;
            case SDL_MOUSEWHEEL:
                tmp = powf(2.0f, -0.5*event.wheel.y);
                tmpVec = fromWinCoord(event.wheel.mouseX, event.wheel.mouseY);
                viewTrans = glm::translate(
                    glm::scale(
                        glm::translate(viewTrans, tmpVec),
                        glm::vec3(tmp)
                    ),
                    -tmpVec
                );
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_q:
                case SDLK_ESCAPE:
                    quit = 1;
                    break;
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            }
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniformMatrix4fv(viewTransAttr, 1, GL_FALSE, glm::value_ptr(viewTrans));
        glDrawArrays(GL_TRIANGLES, 0, nverts);

        nanosleep(&sleeptime, NULL);

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    glDeleteProgram(shaderProgram);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

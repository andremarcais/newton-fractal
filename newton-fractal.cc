#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
//#include <glm/gtx/io.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "embed.h"

typedef GLdouble real;
typedef glm::dmat2x2 complex;
typedef glm::dvec3 vec3;
typedef glm::dvec4 vec4;
typedef glm::dmat4x4 mat4;

static const complex U = complex(1, 0, 0, 1);
static const complex I = complex(0, 1, -1, 0);
static const size_t max_deg = 4; // keep in sync with fragment shader

static int width = 800, height = 800;
static std::vector<complex> roots = { U+I, -U+I, -U-I, U-I };
static std::vector<complex> poly; // This gets initialized by uploadPolynomial()
static SDL_Window *window;
static int selected = -1; // selected root
static glm::dmat4x4 viewTrans(1.0d);
static int quit = 0;

static int
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

static int
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

static void
compileLinkShaders(GLuint shaderProgram)
{
    struct shader {
        const GLenum type;
        const char* path;
        const GLchar* source;
        const GLint length;
        GLuint id;
    } shaders[] = {
        { GL_VERTEX_SHADER, "vertex.glsl", vertex_glsl, vertex_glsl_len },
        { GL_FRAGMENT_SHADER, "fragment.glsl", fragment_glsl, fragment_glsl_len },
        { 0, NULL },
    };

    for (struct shader *shader = shaders; shader->path; ++shader) {
        shader->id = glCreateShader(shader->type);
        if (shader->source) {
            glShaderSource(shader->id, 1, &shader->source, &shader->length);
        } else {
            shaderSourcePath(shader->id, shader->path);
        }
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

// Mouse displacement in pixels to displacement in fractal coordinates
static vec3
fromMouseCoord(int x, int y)
{
    vec3 tmp(
        x/static_cast<real>(width),
        -y/static_cast<real>(height),
        0.0d // Not projective coordinates. 3D cartesian
    );
    // 2.0 because gl coordinates from -1 to 1
    return 2.0d*tmp;
}

// Mouse position in pixels to position in fractal coordinates
static vec3
fromWinCoord(int x, int y)
{
    //vec4 tmp(
    vec3 tmp(
        x/static_cast<real>(width) - 0.5d,
        -y/static_cast<real>(height) + 0.5d,
        0.0d //, 1.0d // projective 3D coordinates
    );
    // 2.0 because gl coordinates from -1 to 1
    return 2.0d*tmp;
    //vec4 tmp2 = viewTrans*tmp;
    //return vec3(tmp2.x, tmp2.y, tmp2.z)/tmp2.w;
}

static GLint
getUniformLocationOrWarn(GLuint shaderProgram, const GLchar *name)
{
    GLint loc = glGetUniformLocation(shaderProgram, name);
    if (loc == -1) {
        fprintf(stderr, "warn: Failed to get location of resource %s\n", name);
    }
    return loc;
}

static void
uploadPolynomial(GLint attr) {
    std::vector<complex> poly1 = { U };
    for (auto& root : roots) {
        std::vector<complex> poly2;
        poly2.push_back(-root*poly1[0]);
        for (size_t k = 1; k < poly1.size(); ++k) {
            poly2.push_back(poly1[k-1] - root*poly1[k]);
        }
        poly2.push_back(poly1.back());
        poly1 = poly2;
    }
    std::vector<real> values;
    for (const auto& term : poly1) {
        for (size_t i = 0; i < 4; ++i) {
            values.push_back(*(glm::value_ptr(term) + i));
        }
    }
    poly = poly1;
    glUniformMatrix2dv(attr, poly1.size(), GL_FALSE, values.data());
}

//static complex
//newton(complex z)
//{
//    for (int i = 0; i < 20; ++i) {
//        complex val = static_cast<real>(0)*U;
//        complex deriv = static_cast<real>(0)*U;
//        for (int k = poly.size() - 1; k >= 0; --k) val = poly[k] + z*val;
//        for (int k = poly.size() - 1; k >= 1; --k) deriv = static_cast<real>(k)*poly[k] + z*deriv;
//        complex deriv_conj = glm::transpose(deriv);
//        z = z - val * deriv_conj / (deriv * deriv_conj)[0][0];
//    }
//    return z;
//}

template<typename T>
static int
nearest_root_to_cursor(T mouse)
{
    auto tmp = fromWinCoord(mouse.x, mouse.y);
    //complex target = newton(tmp[0]*U + tmp[1]*I);
    complex target = tmp[0]*U + tmp[1]*I;
    int best = 0;
    real bestDist = std::numeric_limits<real>::max();
    std::cerr << "target:" << std::endl;
    std::cerr << tmp[0] << '\t' << tmp[1] << '\t' << std::endl;
    std::cerr << "roots:" << std::endl;
    for (size_t i = 0; i < roots.size(); ++i) {
        std::cerr << roots[i][0][0] << '\t' << roots[i][0][1] << std::endl;
        complex diff = target - roots[i];
        real dist = sqrt((diff * glm::transpose(diff))[0][0]);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    std::cerr << "selected: " << best << std::endl;
    return best;
}

int
main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if ((window = SDL_CreateWindow(
                    "SDL OpenGL Window",
                    SDL_WINDOWPOS_CENTERED,
                    SDL_WINDOWPOS_CENTERED,
                    width, height,
                    SDL_WINDOW_OPENGL
                    )
                ) == NULL) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "Failed to initialize GL context: %s\n", SDL_GetError());
        return 1;
    }

    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW: %s\n",
                glewGetErrorString(glewInitResult));
        return 1;
    }

    GLuint shaderProgram = glCreateProgram();
    compileLinkShaders(shaderProgram);

    const char *attrNames[] = { "vertPos" };
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

    GLint viewTransAttr = getUniformLocationOrWarn(shaderProgram, "viewTrans");

    GLint modeAttr = getUniformLocationOrWarn(shaderProgram, "mode");
    GLint polyAttr = getUniformLocationOrWarn(shaderProgram, "poly");
    GLint mode = 1;

    glUseProgram(shaderProgram);

    glViewport(0, 0, width, height);

    SDL_Event event;
    //const struct timespec sleeptime = { 0, 16'666'666 };
    while (!quit) {
        SDL_WaitEvent(NULL);
        while (SDL_PollEvent(&event)) {
            real tmp;
            glm::dvec3 tmpVec;
            switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    selected = nearest_root_to_cursor(event.button);
                    //std::cerr << selected << '\t' << bestDist << target << roots[selected] << std::endl;
                    //std::cerr << selected << std::endl;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                selected = -1;
                break;
            case SDL_MOUSEMOTION:
                if (event.motion.state & SDL_BUTTON_MMASK) {
                    tmpVec = fromMouseCoord(event.motion.xrel, event.motion.yrel);
                    viewTrans = glm::translate(viewTrans, -tmpVec);
                }
                if (event.motion.state & SDL_BUTTON_LMASK && selected >= 0) {
                    tmpVec = fromMouseCoord(event.motion.xrel, event.motion.yrel);
                    roots[selected] = roots[selected] + tmpVec[0]*U + tmpVec[1]*I;
                    //std::cerr << roots[selected][0][0] << '\t' << roots[selected][1][0] << std::endl;
                }
                break;
            case SDL_MOUSEWHEEL:
                tmp = pow(2.0, -0.5*event.wheel.y);
                tmpVec = fromWinCoord(event.wheel.mouseX, event.wheel.mouseY);
                viewTrans = glm::translate(
                    glm::scale(
                        glm::translate(viewTrans, tmpVec),
                        glm::dvec3(tmp)
                    ),
                    -tmpVec
                );
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    width = event.window.data1;
                    height = event.window.data2;
                    glViewport(0, 0, width, height);
                    break;
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_0: mode = 0; break;
                case SDLK_1: mode = 1; break;
                case SDLK_2: mode = 2; break;
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
            //std::cerr << viewTrans << '\n' << viewTrans[0][0] << std::endl;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniformMatrix4dv(viewTransAttr, 1, GL_FALSE, glm::value_ptr(viewTrans));
        glUniform1i(modeAttr, mode);
        uploadPolynomial(polyAttr);
        glDrawArrays(GL_TRIANGLES, 0, nverts);

        //nanosleep(&sleeptime, NULL);

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    glDeleteProgram(shaderProgram);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

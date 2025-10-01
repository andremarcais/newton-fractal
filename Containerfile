FROM docker.io/library/alpine:3.20 as build

RUN apk add sdl2-dev glew-dev xxd pkgconf g++ glm-dev sdl2 glew glm

RUN mkdir /newton-fractal
WORKDIR /newton-fractal

ADD build.sh xxdfor fragment.glsl vertex.glsl newton-fractal.cc .

RUN ./build.sh


FROM docker.io/library/alpine:3.20 as exe

RUN apk add sdl2 glew glm libstdc++ mesa-egl mesa-dri-gallium
COPY --from=build /newton-fractal/newton-fractal /usr/local/bin
ENTRYPOINT newton-fractal

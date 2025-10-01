FROM docker.io/library/alpine:3.20 as buildsys

RUN apk add git gcc
RUN git clone --depth=1 https://github.com/gittup/tup.git
RUN apk add pkgconf fuse3 fuse3-dev musl-dev
RUN apk add pcre2 pcre2-dev
RUN cd tup && unshare -pfr --user --mount ./bootstrap.sh


FROM docker.io/library/alpine:3.20 as build

RUN apk add sdl2-dev glew-dev xxd pkgconf g++ glm-dev sdl2 glew glm

RUN apk add pcre2 fuse3
COPY --from=buildsys /tup/tup /usr/local/bin

RUN mkdir /newton-fractal
WORKDIR /newton-fractal

ADD Tupfile Tupfile.ini xxdfor fragment.glsl vertex.glsl newton-fractal.cc .

RUN unshare -pfr --user --mount tup


FROM docker.io/library/alpine:3.20 as exe

RUN apk add sdl2 glew glm libstdc++ mesa-egl mesa-dri-gallium
COPY --from=build /newton-fractal/newton-fractal /usr/local/bin
ENTRYPOINT newton-fractal

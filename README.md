# Newton fractal

A simple Newton fractal/Mandelbrot set program in GLSL.

## Build/dependencies

[Tup](http://gittup.org/tup/index.html) is used to build this project. After
installing tup, just run `tup`. Otherwise, you can run `./build.sh`.

You must have SDL2, GLEW, and GLM installed.

## Running in a container

This program can be run with `podman`. To build:

    podman build -t newton-fractal .

Then to run it, giving container access to GPU and X display:

    podman run -it --privileged -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix newton-fractal

## Controls

- Button 1 (Left) to move zeros of polynomial for newton fractal
- Button 2 (Wheel) to pan view
- Button 4/5 (Scroll) to zoom in/out


<!--  vim: set ts=4 sw=4 tw=0 et spell spelllang=en : -->

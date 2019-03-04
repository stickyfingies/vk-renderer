#!/usr/bin/env bash

glslangValidator -V Fullscreen.vert -o Compiled/Fullscreen.vert.spv
glslangValidator -V Fullscreen.frag -o Compiled/Fullscreen.frag.spv
glslangValidator -V Raytracer.comp  -o Compiled/Raytracer.comp.spv
glslangValidator -V Tracer.comp     -o Compiled/Tracer.comp.spv

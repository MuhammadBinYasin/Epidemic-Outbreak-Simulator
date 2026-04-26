#pragma once
#ifndef VERTEX_SHADER_H
#define VERTEX_SHADER_H

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 vColor;\n"
"uniform float uZoom;\n"
"uniform vec2  uPan;\n"
"void main()\n"
"{\n"
"  vColor = aColor;\n"
"  vec2 pos = (aPos.xy - uPan) * uZoom;\n"
"  gl_Position = vec4(pos.x, pos.y, aPos.z, 1.0);\n"
"}\0";

#endif // !VERTEX_SHADER_H

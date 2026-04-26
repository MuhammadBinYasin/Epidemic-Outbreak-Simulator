#pragma once
#ifndef FRAGMENT_SHADER_H
#define FRAGEMENT_SHADER_H

const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 vColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"  FragColor = vec4(vColor, 1.0);\n"
"}\0";

#endif // !FRAGMENT_SHADER_H


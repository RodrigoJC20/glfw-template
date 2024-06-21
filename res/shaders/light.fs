#version 330 core
out vec4 FragColor;

uniform vec3 CubeColor;

void main()
{
    FragColor = vec4(CubeColor, 1.0);
}

#version 330 core
uniform vec3 u_color;
out vec4 FragColor;
in vec2  UV;
uniform sampler2D texture_diffuse;

void main()
{
    FragColor = texture(texture_diffuse, UV);
}
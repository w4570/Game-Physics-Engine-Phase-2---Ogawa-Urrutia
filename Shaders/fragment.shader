#version 330 core

uniform vec3 u_color;

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 particlecolor;

// Ouput data
out vec4 color;

uniform sampler2D texture_diffuse;

void main() {
    
    color = texture(texture_diffuse, UV) * particlecolor;

}
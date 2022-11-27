//#version 330 core
//
//// Interpolated values from the vertex shaders
//in vec2 UV;
//in vec4 particlecolor;
//
//// Ouput data
//out vec4 color;
//
//uniform sampler2D myTextureSampler;
//
//void main() {
//	// Output color = color of the texture at the specified UV
//	color = texture(myTextureSampler, UV) * particlecolor;
//
//}

#version 330 core
uniform vec3 u_color;
out vec4 FragColor;
in vec2  UV;
uniform sampler2D texture_diffuse;

void main()
{
    //FragColor = vec4(vec3(1.0-gl_FragCoord.z), 1.0)* texture(texture_diffuse, UV);
    FragColor = texture(texture_diffuse, UV);
    //FragColor = vec4(vec3(1.0-gl_FragCoord.z), 1.0);
    //FragColor = vec3(u_color);
}
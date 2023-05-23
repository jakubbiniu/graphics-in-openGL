#version 330

//Uniform variables
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

//Attributes
in vec2 texCoord0;
in vec4 vertex; //Vertex coordinates in model space
in vec4 color; //vertex color
in vec4 normal; //Vertex normal in model space

//Varying variables
out vec2 iTexCoord0;
out vec2 iTexCoord1;
out vec4 ic;
out vec4 l;
out vec4 n;
out vec4 v;

void main(void) {
    vec4 lp = vec4(0, 0, -6, 1); //light position, world space
    l = normalize(V * lp - V * M * vertex); //vector towards the light in eye space
    v = normalize(vec4(0, 0, 0, 1) - V * M * vertex); //vector towards the viewer in eye space
    n = normalize(V * M * normal); //normal vector in eye space

	iTexCoord0 = texCoord0;
    iTexCoord1 = (n.xy+1)/2;
    ic = color;

    gl_Position = P * V * M * vertex;
}
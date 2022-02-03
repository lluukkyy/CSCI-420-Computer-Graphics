#version 150

in vec3 position;
in vec3 position_smooth;
in vec4 color;
in vec4 color_smooth;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform int mode;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  if(mode == 0){
    gl_Position =  projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    col = color;
  } else if (mode == 1) {
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position_smooth, 1.0f);
    col = color_smooth;
  } 
  
}


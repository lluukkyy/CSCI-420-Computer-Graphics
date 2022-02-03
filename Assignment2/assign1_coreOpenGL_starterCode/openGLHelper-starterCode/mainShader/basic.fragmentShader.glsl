#version 150

in vec3 viewPosition;
in vec3 viewNormal;
out vec4 c; // output color

uniform vec4 La = vec4(.1,.1,.1,.1); // light ambient
uniform vec4 Ld = vec4(1,1,1,1); // light diffuse
uniform vec4 Ls = vec4(.3,.3,.3,.3); // light specular
uniform vec3 viewLightDirection;
uniform vec4 ka = vec4(.1,.1,.1,.1); // mesh ambient
uniform vec4 kd = vec4(.1,.1,.1,.1); // mesh diffuse
uniform vec4 ks = vec4(.1,.1,.1,.1); // mesh specular
uniform float alpha = 1; // shininess

void main()
{
  // camera is at (0,0,0) after the modelview transformation
  vec3 eyedir = normalize(vec3(0, 0, 0) - viewPosition);
  // reflected light direction
  vec3 reflectDir = -reflect(viewLightDirection, viewNormal);
  // Phong lighting
  float d = max(dot(viewLightDirection, viewNormal), 0.0f);
  float s = max(dot(reflectDir, eyedir), 0.0f);
  // compute the final color
  c = ka * La + d * kd * Ld + pow(s, alpha) * ks * Ls;
}

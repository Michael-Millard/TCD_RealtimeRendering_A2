#version 330 core

layout(location = 0) in vec3 aPos;     // Vertex position
layout(location = 1) in vec3 aNormal;  // Vertex normal

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 inverseProjection;

out vec3 V; // View direction
out vec3 N; // Normal vector

void main() 
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec3 viewPos = vec3(inverse(view) * vec4(0.0, 0.0, 0.0, 1.0)); // Camera position
    V = normalize(viewPos - worldPos.xyz); // Compute view direction

    vec3 I = -V; // Incident light direction
    N = normalize(mat3(transpose(inverse(model))) * aNormal); // Correct normal transformation
    
    gl_Position = projection * view * worldPos;
}

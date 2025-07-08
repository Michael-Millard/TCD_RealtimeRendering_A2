#version 330 core

in vec3 V; // View direction
in vec3 N; // Normal at the fragment

out vec4 FragColor;

uniform samplerCube skybox; // Environment map

// Dispersion values for RGB
uniform float etaR;   
uniform float etaG; 
uniform float etaB;  
uniform float F0; // Base reflectance for dielectrics

// Fresnel-Schlick approximation
float fresnelSchlick(float cosTheta) 
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() 
{
    // Compute reflection direction
    vec3 reflectedDir = reflect(-V, N);
    
    // Compute chromatic dispersion refraction directions
    vec3 refractedDirR = refract(-V, N, etaR);
    vec3 refractedDirG = refract(-V, N, etaG);
    vec3 refractedDirB = refract(-V, N, etaB);

    // Compute Fresnel term
    float cosTheta = clamp(dot(V, N), 0.0, 1.0);
    float fresnel = fresnelSchlick(cosTheta);

    // Sample skybox for reflection and refraction
    vec3 reflectedColor = texture(skybox, reflectedDir).rgb;
    vec3 refractedColor = vec3(
        texture(skybox, refractedDirR).r,
        texture(skybox, refractedDirG).g,
        texture(skybox, refractedDirB).b
    );

    // Blend using Fresnel term
    vec3 finalColor = mix(refractedColor, reflectedColor, fresnel);

    FragColor = vec4(finalColor, 1.0);
}

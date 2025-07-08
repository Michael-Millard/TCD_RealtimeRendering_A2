#ifndef MY_MESH_H
#define MY_MESH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <my_shader.h>

#include <string>
#include <vector>

struct Vertex 
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture 
{
    unsigned int id;
    std::string path;
};

// Enum for 6 DoF pose indexing
enum
{
    tX = 0,
    tY = 1,
    tZ = 2,
    rX = 3,
    rY = 4,
    rZ = 5
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    glm::mat4 meshMatrix;
    std::string meshName;
    float mesh6DoF[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float initRad = 0.0f;
    float initRot = 0.0f;

    // Init the mesh
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, const std::vector<Texture>& textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        setupMesh();

        // Init mesh matrix to identity
        this->meshMatrix = glm::mat4(1);
    }

    // Update mesh matrix
    void updateModelMatrix()
    {
        meshMatrix = glm::mat4(1.0f); // Start with identity matrix
        meshMatrix = glm::translate(meshMatrix, glm::vec3(mesh6DoF[tX], mesh6DoF[tY], mesh6DoF[tZ]));   // Apply translation
        meshMatrix = glm::rotate(meshMatrix, mesh6DoF[rX], glm::vec3(1.0f, 0.0f, 0.0f));                // Rotate around X-axis
        meshMatrix = glm::rotate(meshMatrix, mesh6DoF[rY], glm::vec3(0.0f, 1.0f, 0.0f));                // Rotate around Y-axis
        meshMatrix = glm::rotate(meshMatrix, mesh6DoF[rZ], glm::vec3(0.0f, 0.0f, 1.0f));                // Rotate around Z-axis    
    }

    // Draw the mesh
    void draw(Shader& shader)
    {
        // If multiple textures for this mesh, loop through
        for (unsigned int i = 0; i < static_cast<unsigned int>(textures.size()); i++)
        {
            // Active proper texture unit before binding
            glActiveTexture(GL_TEXTURE0 + i); 
 
            // Set the sampler to the correct texture unit
            shader.setInt("textureDiffuse" + std::to_string(i), i);

            // Bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // Draw
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set active back to 0
        glActiveTexture(GL_TEXTURE0);
    }

    // Draw the mesh with hierarchy
    void drawHierarchy(Shader& shader, glm::mat4& modelMat, float& rotZ)
    {
        glm::mat4 model = modelMat;
        model = glm::rotate(model, glm::radians(rotZ), glm::vec3(0.0f, 0.0f, 1.0f));
        shader.setMat4("model", model);

        // If multiple textures for this mesh, loop through
        for (unsigned int i = 0; i < static_cast<unsigned int>(textures.size()); i++)
        {
            // Active proper texture unit before binding
            glActiveTexture(GL_TEXTURE0 + i);

            // Set the sampler to the correct texture unit
            shader.setInt("textureDiffuse" + std::to_string(i), i);

            // Bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // Draw
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Set active back to 0
        glActiveTexture(GL_TEXTURE0);

        // Reset model matrix
        shader.setMat4("model", modelMat);
    }

private:
    unsigned int VAO, VBO, EBO;

    // Setup
    void setupMesh()
    {
        // Create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Bind VAO
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        
        // Vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};
#endif

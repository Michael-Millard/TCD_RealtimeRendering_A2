#ifndef MY_MODEL_H
#define MY_MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <my_mesh.h>
#include <my_shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

// Forward declare
unsigned int loadTexture(const char* texturePath);

class Model
{
public:
    // Public for wall constraints
    std::vector<Mesh> meshes;

    // Constructor (expects a filepath to a 3D model)
    Model(std::string const& objPath)
    {
        loadModel(objPath);
    }

    // Draw the model (all its meshes)
    void draw(Shader& shader)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(meshes.size()); i++)
            meshes[i].draw(shader);
    }

    // Draw the model (all its meshes) hierarchicaly
    void drawHierarchy(Shader& shader, glm::mat4& modelMat, float& rotZ)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(meshes.size()); i++)
        {
            if (meshes[i].meshName == "Prop")
                meshes[i].drawHierarchy(shader, modelMat, rotZ);
            else
                meshes[i].draw(shader);
        }
    }

private:
    // Load a 3D model specified by path
    void loadModel(std::string const& path)
    {
        // Read file
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        
        // Check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        // Process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // Processes a node recursively
    void processNode(aiNode* node, const aiScene* scene)
    {
        // Process each mesh located at current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // Recursively process children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        // Data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Loop through mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            // Positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // Normals (if it has)
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // Texture (if it has)
            if (mesh->mTextureCoords[0])
            {
                // Texture coordinates
                glm::vec2 texVec;
                texVec.x = mesh->mTextureCoords[0][i].x;
                texVec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = texVec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        // Loop through mesh's faces and retrieve the corresponding vertex indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            // Retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // Only using diffuse textures
        std::vector<Texture> textureMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
        textures.insert(textures.end(), textureMaps.begin(), textureMaps.end());

        // Return a mesh object created from the extracted mesh data
        Mesh tempMesh(vertices, indices, textures);

        // Set name if present
        std::string meshName = std::string(mesh->mName.C_Str());
        if (!meshName.empty())
            tempMesh.meshName = meshName;

        return tempMesh;
    }

    // Load materials
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            Texture texture;
            texture.id = loadTexture(str.C_Str());
            texture.path = str.C_Str();
            textures.push_back(texture);
        }
        return textures;
    }
};

unsigned int loadTexture(const char* texturePath)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    stbi_set_flip_vertically_on_load(false);
    int width, height, numChannels;
    unsigned char* data = stbi_load(texturePath, &width, &height, &numChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (numChannels == 1)
            format = GL_RED;
        else if (numChannels == 3)
            format = GL_RGB;
        else if (numChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
        std::cout << "Texture failed to load at path: " << texturePath << std::endl;

    stbi_image_free(data);

    return textureID;
}
#endif // MY_MODEL_H

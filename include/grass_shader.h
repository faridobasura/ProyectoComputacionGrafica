#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"  // Tu clase Shader base

class GrassShader {
private:
    Shader shader;
    
public:
    // Constructores
    GrassShader();  // Constructor por defecto
    GrassShader(const char* vertexPath, const char* fragmentPath);  // Constructor con paths
    
    void use();
    void setup(const glm::mat4& projection, const glm::mat4& view, 
               const glm::mat4& model, float currentTime,
               const glm::vec3& cameraPos,
               const glm::vec3& windDirection = glm::vec3(0.8f, 0.0f, 0.6f),
               float windStrength = 0.3f);
    
    void setTextures(GLuint albedoID);
    void setWindDirection(const glm::vec3& direction);
    void setWindStrength(float strength);
};
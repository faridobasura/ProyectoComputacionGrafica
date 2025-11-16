#include "grass_shader.h"
#include <iostream>

// Constructor por defecto - usa paths por defecto
GrassShader::GrassShader() 
    : shader("shaders/grass/grass.vs", "shaders/grass/grass.fs") {
    std::cout << "GrassShader loaded with default paths" << std::endl;
}

// Constructor con paths personalizados
GrassShader::GrassShader(const char* vertexPath, const char* fragmentPath)
    : shader(vertexPath, fragmentPath) {
    std::cout << "GrassShader loaded with custom paths" << std::endl;
}

// El resto de métodos permanece igual...
void GrassShader::use() {
    shader.use();
}

void GrassShader::setup(const glm::mat4& projection, const glm::mat4& view,
                       const glm::mat4& model, float currentTime,
                       const glm::vec3& cameraPos,
                       const glm::vec3& windDirection, float windStrength) {
    use();
    
    // Matrices
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setMat4("model", model);
    
    // Tiempo y viento
    shader.setFloat("time", currentTime);
    shader.setVec3("windDirection", windDirection);
    shader.setFloat("windStrength", windStrength);
    
    // Posición de cámara
    shader.setVec3("viewPos", cameraPos);
    
    // Luz fija (sol)
    shader.setVec3("lightPos", glm::vec3(0.0f, 100.0f, 0.0f));
    shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
}

void GrassShader::setTextures(GLuint albedoID) {
    use();
    shader.setInt("grassAlbedo", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoID);
}
#include "MissionManager.h"

// Incluir GLAD primero
#include <glad/glad.h>

// Luego GLM con todos los headers necesarios
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Para glm::translate, glm::perspective, etc.
#include <glm/gtc/type_ptr.hpp>         // Para glm::value_ptr
#include <glm/gtc/constants.hpp>        // Para glm::pi

// Después los demás headers
#include <shader_m.h>
#include <iostream>
#include <cmath>                        // Para std::cos, std::sin

MissionManager::MissionManager()
    : currentMissionIndex(0), markerShader(nullptr), cylinderVAO(0), cylinderVBO(0) {
}

MissionManager::~MissionManager() {
    if (cylinderVAO != 0) {
        glDeleteVertexArrays(1, &cylinderVAO);
    }
    if (cylinderVBO != 0) {
        glDeleteBuffers(1, &cylinderVBO);
    }
    if (markerShader) {
        delete markerShader;
    }
}

bool MissionManager::Initialize() {
    // Cargar shader
    markerShader = new Shader("shaders/mission_marker.vs", "shaders/mission_marker.fs");
    if (markerShader == nullptr || markerShader->ID == 0) {
        std::cout << "Error cargando shader de misiones" << std::endl;
        return false;
    }

    // Crear geometría
    CreateCylinderGeometry();

    // Activar primera misión si existe
    if (!markers.empty()) {
        markers[0].isActive = true;
    }

    return true;
}

void MissionManager::CreateCylinderGeometry() {
    const int segments = 16;
    const float height = 3.0f;
    const float radius = 1.0f;

    std::vector<float> vertices;

    for (int i = 0; i <= segments; ++i) {
        // Usar M_PI de cmath si glm::pi no está disponible
        float angle = 2.0f * 3.14159265358979323846f * i / segments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;

        // Vértice inferior
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        // Vértice superior
        vertices.push_back(x);
        vertices.push_back(height);
        vertices.push_back(z);
    }

    glGenVertexArrays(1, &cylinderVAO);
    glGenBuffers(1, &cylinderVBO);

    glBindVertexArray(cylinderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void MissionManager::AddMission(glm::vec3 position, float radius) {
    markers.push_back(MissionMarker(position, radius));
}

void MissionManager::Update(const glm::vec3& playerPosition) {
    if (currentMissionIndex >= static_cast<int>(markers.size())) return;

    MissionMarker& currentMarker = markers[currentMissionIndex];
    if (!currentMarker.isActive) return;

    float distance = glm::distance(playerPosition, currentMarker.position);
    if (distance < currentMarker.radius) {
        CompleteCurrentMission();
    }
}

void MissionManager::Render(glm::mat4 projection, glm::mat4 view) {
    if (!markerShader) return;

    markerShader->use();
    markerShader->setMat4("projection", projection);
    markerShader->setMat4("view", view);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    for (const auto& marker : markers) {
        if (!marker.isActive) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, marker.position);
        markerShader->setMat4("model", model);
        markerShader->setVec4("color", glm::vec4(1.0f, 1.0f, 0.0f, 0.3f));

        glBindVertexArray(cylinderVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 34); // 2 * (16 + 1) = 34 vértices
    }

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void MissionManager::CompleteCurrentMission() {
    if (currentMissionIndex < static_cast<int>(markers.size())) {
        markers[currentMissionIndex].isActive = false;
        currentMissionIndex++;

        if (currentMissionIndex < static_cast<int>(markers.size())) {
            markers[currentMissionIndex].isActive = true;
            std::cout << "Misión " << currentMissionIndex + 1 << " activada" << std::endl;
        }
        else {
            std::cout << "¡Todas las misiones completadas!" << std::endl;
        }
    }
}

bool MissionManager::AllMissionsCompleted() const {
    return currentMissionIndex >= static_cast<int>(markers.size());
}
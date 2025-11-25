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

void MissionManager::AddMission(glm::vec3 position, float radius, std::string name) {
    MissionMarker newMission = MissionMarker(position, radius, name);
    newMission.isActive = true;
    markers.push_back(newMission);
}

void MissionManager::Update(const glm::vec3& playerPosition) {
    if (currentMissionIndex >= static_cast<int>(markers.size())) {
        return;
    }

    MissionMarker& currentMarker = markers[currentMissionIndex];
    if (!currentMarker.isActive) {
        return;
    }

    //float distance = glm::distance(playerPosition, currentMarker.position);

    //if (distance < currentMarker.radius) {
    //    std::cout << "¡Misión completada por proximidad: " << currentMarker.name << "!" << std::endl;
    //    CompleteCurrentMission();
    //}
}
std::string MissionManager::GetCurrentMissionName() const {
    if (currentMissionIndex < static_cast<int>(markers.size())) {
        return markers[currentMissionIndex].name;
    }
    return "Todas las misiones completadas";
}

void MissionManager::Render(glm::mat4 projection, glm::mat4 view) {
    if (!markerShader) return;

    // GUARDAR ESTADOS ACTUALES
    GLboolean wasDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint prevDepthMask;
    glGetIntegerv(GL_DEPTH_WRITEMASK, &prevDepthMask);

    markerShader->use();
    markerShader->setMat4("projection", projection);
    markerShader->setMat4("view", view);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    for (const auto& marker : markers) {
        if (!marker.isActive) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, marker.position);
        markerShader->setMat4("model", model);
        markerShader->setVec4("color", glm::vec4(1.0f, 1.0f, 0.0f, 0.5f));

        glBindVertexArray(cylinderVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 34);
    }

    glBindVertexArray(0);

    // RESTAURAR ESTADOS ORIGINALES
    glDepthMask(prevDepthMask);

    if (wasCullEnabled) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    if (!wasBlendEnabled) glDisable(GL_BLEND);

    if (wasDepthEnabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}
void MissionManager::CompleteMission(std::string missionName) {
    // Buscar la misión por nombre
    for (int i = 0; i < markers.size(); ++i) {
        if (markers[i].name == missionName && !markers[i].isCompleted) {
            // Completar esta misión
            markers[i].isCompleted = true;
            markers[i].isActive = false;

            std::cout << "Misión completada por nombre: " << missionName << std::endl;

            // Si era la misión actual, activar la siguiente
            if (i == currentMissionIndex) {
                currentMissionIndex++;
                if (currentMissionIndex < static_cast<int>(markers.size())) {
                    markers[currentMissionIndex].isActive = true;
                    std::cout << "Nueva misión activada: " << markers[currentMissionIndex].name << std::endl;
                }
                else {
                    std::cout << "¡Todas las misiones completadas!" << std::endl;
                }
            }
            return; // Salir después de encontrar y completar la misión
        }
    }

    // Si no se encontró la misión
    std::cout << "Misión no encontrada o ya completada: " << missionName << std::endl;
}

bool MissionManager::AllMissionsCompleted() const {
    return currentMissionIndex >= static_cast<int>(markers.size());
}

int MissionManager::GetCompletedMissions() const {
    int completed = 0;
    for (const auto& marker : markers) {
        if (marker.isCompleted) completed++;
    }
    return completed;
}

bool MissionManager::IsMissionActive(int index) const {
    if (index >= 0 && index < static_cast<int>(markers.size())) {
        return markers[index].isActive;
    }
    return false;
}

bool MissionManager::IsMissionCompleted(int index) const {
    if (index >= 0 && index < static_cast<int>(markers.size())) {
        return markers[index].isCompleted;
    }
    return false;
}

#ifndef MISSION_MANAGER_H
#define MISSION_MANAGER_H

#include <vector>
#include <glm/glm.hpp>
#include <shader_m.h>

class MissionManager {
private:
    struct MissionMarker {
        glm::vec3 position;
        float radius;
        bool isActive;
        bool isCompleted;
        std::string name;  // NUEVO: nombre de la misión

        MissionMarker(glm::vec3 pos, float rad = 2.0f, std::string missionName = "")
            : position(pos), radius(rad), isActive(false),
            isCompleted(false), name(missionName) {
        }
    };

    std::vector<MissionMarker> markers;
    int currentMissionIndex;
    Shader* markerShader;
    GLuint cylinderVAO, cylinderVBO;

    void CreateCylinderGeometry();

public:
    MissionManager();
    ~MissionManager();

    bool Initialize();
    void AddMission(glm::vec3 position, float radius = 2.0f, std::string name = "");
    void Update(const glm::vec3& playerPosition);
    void Render(glm::mat4 projection, glm::mat4 view);
    void CompleteMission(std::string);
    bool AllMissionsCompleted() const;
    int GetCurrentMissionIndex() const { return currentMissionIndex; }
    int GetTotalMissions() const { return markers.size(); }
    int GetCompletedMissions() const;
    bool IsMissionActive(int index) const;
    bool IsMissionCompleted(int index) const;
    std::string GetCurrentMissionName() const;
};
#endif
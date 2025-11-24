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

        MissionMarker(glm::vec3 pos, float rad = 2.0f)
            : position(pos), radius(rad), isActive(false) {
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
    void AddMission(glm::vec3 position, float radius = 2.0f);
    void Update(const glm::vec3& playerPosition);
    void Render(glm::mat4 projection, glm::mat4 view);
    void CompleteCurrentMission();
    bool AllMissionsCompleted() const;
    int GetCurrentMissionIndex() const { return currentMissionIndex; }
    int GetTotalMissions() const { return markers.size(); }
};

#endif
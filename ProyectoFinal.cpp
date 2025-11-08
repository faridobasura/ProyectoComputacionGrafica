/*
* * Proyecto Final
*/

#include <iostream>
#include <stdlib.h>
#include <sstream> // Necesario para SetLightUniform...
#include <vector>  // Para usar std::vector
#include <string>  // Para usar std::string

// GLAD: Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator
// https://glad.dav1d.de/
#include <glad/glad.h>

// GLFW: https://www.glfw.org/
#include <GLFW/glfw3.h>

// GLM: OpenGL Math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Model loading classes
#include <shader_m.h>
#include <camera.h>
#include <model.h>
#include <animatedmodel.h>
#include <material.h>
#include <light.h>
#include <cubemap.h>

#include <irrKlang.h>
#include <algorithm> 


using namespace irrklang;

//Cajas de colisiones
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}
};

//Objetos colisionables
struct CollidableObject {
    Model* model;
    glm::vec3 position;
    AABB boundingBox;
    glm::vec3 scale;
    float rotationY;
};

struct InteractiveObject {
    Model* model;            // Puntero al modelo cargado
    glm::vec3   position;         // Posición en el mundo
    float       triggerRadius;    // Radio de cercanía
    std::string name;             // Nombre para depuración

    float       inspectRotationY; // Rotación en Y (ahora automática)
    float       inspectRotationX; // Rotación en X (ya no se usa, pero se queda para reset)
    bool        isAutoRotatingY;  // Bandera para rotación automática

    glm::vec3   inspectTargetOffset; // Punto de mira (relativo a la posición del objeto)
    glm::vec3   inspectCamPos;       // Posición de la cámara (relativa a la posición del objeto)

    InteractiveObject(Model* m, glm::vec3 pos, float radius, std::string n, 
                      glm::vec3 targetOffset, glm::vec3 camPos) : 
        model(m), position(pos), triggerRadius(radius), name(n),
        inspectRotationY(0.0f), inspectRotationX(0.0f), isAutoRotatingY(false),
        inspectTargetOffset(targetOffset), inspectCamPos(camPos) 
    { } 
};

// Functions
bool Start();
bool Update();
bool CheckCollision(const AABB& a, const AABB& b);
AABB CalculateWorldAABB(const CollidableObject& obj);
AABB GetCharacterBoundingBox();
bool CheckCharacterCollision();
bool CheckCollisionAtPosition(const glm::vec3& position);
void InitializeCollidableObjects();
void UpdateFirstPersonCamera();
void UpdateThirdPersonCamera();
void UpdateCameras(); 

// Definición de callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Gobals
GLFWwindow* window;

// Tamaño en pixeles de la ventana
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Variables del personaje
glm::vec3 character_position = glm::vec3(0.0f, 0.0f, 20.0f);
glm::vec3 forwardView(0.0f, 0.0f, -1.0f);
glm::vec3 rightView = glm::normalize(glm::cross(forwardView, glm::vec3(0.0f, 1.0f, 0.0f))); //camarad e hombro

float       thirdpersonOffset = 1.5f;
float       rotateCharacter = 180.0f;
bool character_run = false;
float walkSpeed = 0.05f;
float runSpeed = 0.2f;
float       scaleV = walkSpeed;

float characterHeight = 2.0f;      // Altura del personaje
float characterRadius = 0.5f;      // Radio para colisión cilíndrica
float collisionOffset = 1.0f;      // Margen de seguridad

// Definición de cámara (posición en XYZ)
Camera camera_float(glm::vec3(0.0f, 2.0f, 10.0f));
Camera camera1st = character_position + glm::vec3(0.0f, 1.0f, 0.0f);
Camera camera3rd = character_position + glm::vec3(0.0f, 1.3f, -0.05f);

// Controladores para el movimiento del mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Variables para la velocidad de reproducción
// de la animación
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float elapsedTime = 0.0f;

//Variables para las colisiones
std::vector<CollidableObject> g_collidableObjects;
AABB characterBoundingBox;

// Variables para debug de colisiones
unsigned int debugVAO, debugVBO;
bool showCollisionBoxes = false; // Presiona C para mostrar/ocultar

glm::vec3 position(0.0f, 0.0f, 0.0f);
glm::vec3 position_origin(0.0f, 0.0f, 0.0f);

glm::vec3 estatuaPos = position_origin;
glm::vec3 piramidePos = glm::vec3(0.0f, 0.0f, -25.0f);
glm::vec3 PiedraSolPos = glm::vec3(0.0f, 0.0f, -77.6f);
glm::vec3 CoatlicuePos = glm::vec3(-48.47f, 0.0f, -97.635f);
glm::vec3 PlatoAntiguoPos = glm::vec3(-42.28f, 0.0f, -73.26f);
glm::vec3 CraneoPos = glm::vec3(-25.69f, 0.22f, -118.21f);
glm::vec3 IncenciarioPos = glm::vec3(36.63f, 0.0f, -112.64f);
glm::vec3 XochipilliPos = glm::vec3(51.11f, 0.06f, -93.76f);
glm::vec3 BraceroPos = glm::vec3(37.94f, 0.10f, -70.74f);


// Shaders
Shader* mLightsShader;
Shader* proceduralShader;
Shader* wavesShader;
Shader* debugShader;

Shader* cubemapShader;
Shader* dynamicShader;

// Carga la información de los modelo
Model* museo; // Entorno

Model* Xiucoatl;
Model* piramide;
Model* PiedraDelSol;
Model* Coatlicue;
Model* PlatoAntiguo;
Model* Craneo;
Model* Incenciario;
Model* Xochipilli;
Model* Bracero;

// Modelos animados
AnimatedModel* character01;

// Cubemap
CubeMap* mainCubeMap;

// Light gLight;
std::vector<Light> gLights;

// Materiales
Material material01;

float proceduralTime = 0.0f;
float wavesTime = 0.0f;

// Audio
ISoundEngine* SoundEngine = createIrrKlangDevice();

// selección de cámara
int activeCamera = 1;


// --- VARIABLES GLOBALES DE INTERACCIÓN ---
std::vector<InteractiveObject> g_interactiveObjects; // Lista de exhibiciones
InteractiveObject* g_nearbyObject = nullptr;      // Exhibición más cercana (para "Presiona F")
InteractiveObject* g_interactingObject = nullptr; // Exhibición con la que estamos interactuando
bool g_f_keyPressed = false; // Para detectar una sola pulsación de 'F'
bool g_y_keyPressed = false; // Para detectar una sola pulsación de 'Y'


// --- VARIABLES CÁMARA DE INSPECCIÓN ---
float g_inspectZoom = 45.0f; // --- ¡NUEVO! Variable para el zoom (inicia en 45 grados de FOV) ---
// --- FIN DE VARIABLES ---

// Entrada a función principal
int main()
{
    if (!Start())
        return -1;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        if (!Update())
            break;
    }

    glfwTerminate();
    return 0;

}

void CreateDebugCube() {
    float vertices[] = {
        // Cara frontal
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // Cara trasera
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // Cara izquierda
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        // Cara derecha
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,

         // Cara inferior
         -0.5f, -0.5f, -0.5f,
          0.5f, -0.5f, -0.5f,
          0.5f, -0.5f,  0.5f,
          0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f, -0.5f,

         // Cara superior
         -0.5f,  0.5f, -0.5f,
         -0.5f,  0.5f,  0.5f,
          0.5f,  0.5f,  0.5f,
          0.5f,  0.5f,  0.5f,
          0.5f,  0.5f, -0.5f,
         -0.5f,  0.5f, -0.5f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    debugVAO = VAO;
}

void DrawBoundingBox(const AABB& box, const glm::vec3& color, glm::mat4 projection, glm::mat4 view) {
    if (!showCollisionBoxes) return;

    debugShader->use();

    // Calcular centro y tamaño
    glm::vec3 center = (box.min + box.max) * 0.5f;
    glm::vec3 size = box.max - box.min;

    // Matriz de transformación
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, center);
    model = glm::scale(model, size);

    debugShader->setMat4("model", model);
    debugShader->setMat4("view", view);
    debugShader->setMat4("projection", projection);
    debugShader->setVec3("color", color);

    glBindVertexArray(debugVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void UpdateFirstPersonCamera() {
    camera1st.Position = character_position + glm::vec3(0.0f, 1.8f, 0.0f);
    camera1st.Front = forwardView;
    camera1st.Right = rightView;
    camera1st.Up = glm::vec3(0.0f, 1.0f, 0.0f);
}

void UpdateThirdPersonCamera() {
    camera3rd.Front = forwardView;
    camera3rd.Position = character_position
        - (thirdpersonOffset * forwardView)
        + (0.5f * rightView)
        + glm::vec3(0.0f, 1.3f, 0.0f);
}

void UpdateCameras() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    forwardView = glm::normalize(glm::vec3(viewVector));
    rightView = glm::normalize(glm::cross(forwardView, glm::vec3(0.0f, 1.0f, 0.0f)));

    UpdateFirstPersonCamera();
    UpdateThirdPersonCamera();
}

bool Start() {
    // Inicialización de GLFW

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Creación de la ventana con GLFW
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Animation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: Cargar todos los apuntadores
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Activación de buffer de profundidad
    glEnable(GL_DEPTH_TEST);

    // Compilación y enlace de shaders
    mLightsShader = new Shader("shaders/11_PhongShaderMultLights.vs", "shaders/11_PhongShaderMultLights.fs");
    proceduralShader = new Shader("shaders/12_ProceduralAnimation.vs", "shaders/12_ProceduralAnimation.fs");
    wavesShader = new Shader("shaders/13_wavesAnimation.vs", "shaders/13_wavesAnimation.fs");
    cubemapShader = new Shader("shaders/10_vertex_cubemap.vs", "shaders/10_fragment_cubemap.fs");
    dynamicShader = new Shader("shaders/10_vertex_skinning-IT.vs", "shaders/10_fragment_skinning-IT.fs");
    debugShader = new Shader("shaders/shader_debug.vs", "shaders/shader_debug.fs");

    CreateDebugCube();

    // Máximo número de huesos: 100
    dynamicShader->setBonesIDs(MAX_RIGGING_BONES);

    // --- Carga de modelos modulares ---
    museo = new Model("models/IllumModels/proyectofinal/Entorno.fbx");
    Xiucoatl = new Model("models/IllumModels/estatua.fbx");
    piramide = new Model("models/IllumModels/proyectofinal/piramides.fbx");
    PiedraDelSol = new Model("models/IllumModels/proyectofinal/PiedraDelSol.fbx");
    Coatlicue = new Model("models/IllumModels/proyectofinal/Coatlicue.fbx");
    PlatoAntiguo = new Model("models/IllumModels/proyectofinal/PlatoAntiguo.fbx");
    Craneo = new Model("models/IllumModels/proyectofinal/Craneo.fbx");
    Incenciario = new Model("models/IllumModels/proyectofinal/Incenciario.fbx");
    Xochipilli = new Model("models/IllumModels/proyectofinal/Xochipilli.fbx");
    Bracero = new Model("models/IllumModels/proyectofinal/Bracero.fbx");
    character01 = new AnimatedModel("models/character.fbx");

    
    // --- Configuración de cámaras de inspección ---
    // Formato: (Modelo, Posición, RadioTrigger, Nombre, OffsetObjetivo, OffsetCámara)
    
    // Objeto: Xiucoatl (Estatua) - Cámara frontal estándar
    g_interactiveObjects.emplace_back(Xiucoatl, estatuaPos, 6.0f, "Estatua",
        glm::vec3(0.0f, 5.0f, 1.0f),  
        glm::vec3(0.0f, 5.0f, 11.0f));

    // Objeto: Pirámide - Cámara un poco más alta y lejana
    g_interactiveObjects.emplace_back(piramide, piramidePos, 4.0f, "Piramides",
        glm::vec3(0.0f, 2.0f, 0.0f),  // Mirar 1m arriba
        glm::vec3(0.0f, 3.0f, 10.0f)); // Pararse 2m arriba, 5m enfrente

    // Objeto: PiedraDelSol - Cámara frontal
    g_interactiveObjects.emplace_back(PiedraDelSol, PiedraSolPos, 15.0f, "PiedraSol",
        glm::vec3(0.0f, 7.0f, 0.0f),  // Mirar al centro (1.5m)
        glm::vec3(0.0f, 7.0f, 15.0f)); // Pararse 3m enfrente

    // Objeto: Coatlicue - Vista lateral izquierda
    g_interactiveObjects.emplace_back(Coatlicue, CoatlicuePos, 3.0f, "Coatlicue",
        glm::vec3(0.0f, 1.5f, 0.0f),  // Mirar al centro (1.5m)
        glm::vec3(-4.0f, 1.5f, 0.0f)); // Pararse 4m a la IZQUIERDA (X negativo)

    // Objeto: PlatoAntiguo - Vista superior (Top-Down)
    g_interactiveObjects.emplace_back(PlatoAntiguo, PlatoAntiguoPos, 3.0f, "PlatoAntiguo",
        glm::vec3(0.0f, 0.5f, 0.0f),  // Mirar justo encima del plato
        glm::vec3(0.0f, 3.0f, 0.5f)); // Pararse 3m ARRIBA, un poco enfrente

    // Objeto: Craneo - Vista frontal estándar
    g_interactiveObjects.emplace_back(Craneo, CraneoPos, 3.0f, "Craneo",
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 3.0f));

    // Objeto: Incenciario - Vista lateral derecha
    g_interactiveObjects.emplace_back(Incenciario, IncenciarioPos, 3.0f, "Incenciario",
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(3.0f, 1.0f, 0.0f)); // Pararse 3m a la DERECHA (X positivo)

    // Objeto: Xochipilli - Vista frontal estándar
    g_interactiveObjects.emplace_back(Xochipilli, XochipilliPos, 3.0f, "Xochipilli",
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 3.0f));

    // Objeto: Bracero - Vista frontal estándar
    g_interactiveObjects.emplace_back(Bracero, BraceroPos, 3.0f, "Bracero",
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 3.0f));
    // --- Fin de Carga de modelos ---


    // Cubemap
    vector<std::string> faces
    {
        "textures/cubemap/01/posx.png",
        "textures/cubemap/01/negx.png",
        "textures/cubemap/01/posy.png",
        "textures/cubemap/01/negy.png",
        "textures/cubemap/01/posz.png",
        "textures/cubemap/01/negz.png"
    };
    mainCubeMap = new CubeMap();
    mainCubeMap->loadCubemap(faces);

    UpdateCameras();

    // Lights configuration
    Light light01;
    light01.Position = glm::vec3(5.0f, 2.0f, 5.0f);
    light01.Color = glm::vec4(0.2f, 0.0f, 0.0f, 1.0f);
    gLights.push_back(light01);

    Light light02;
    light02.Position = glm::vec3(-5.0f, 2.0f, 5.0f);
    light02.Color = glm::vec4(0.0f, 0.2f, 0.0f, 1.0f);
    gLights.push_back(light02);

    Light light03;
    light03.Position = glm::vec3(5.0f, 2.0f, -5.0f);
    light03.Color = glm::vec4(0.0f, 0.0f, 0.2f, 1.0f);
    gLights.push_back(light03);

    Light light04;
    light04.Position = glm::vec3(-5.0f, 2.0f, -5.0f);
    light04.Color = glm::vec4(0.2f, 0.2f, 0.0f, 1.0f);
    gLights.push_back(light04);

    // SoundEngine->play2D("sound/EternalGarden.mp3", true);

     // --- INICIALIZAR COLISIONES ---
    InitializeCollidableObjects();

    return true;
}


void SetLightUniformInt(Shader* shader, const char* propertyName, size_t lightIndex, int value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader->setInt(uniformName.c_str(), value);
}
void SetLightUniformFloat(Shader* shader, const char* propertyName, size_t lightIndex, float value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader->setFloat(uniformName.c_str(), value);
}
void SetLightUniformVec4(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec4 value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader->setVec4(uniformName.c_str(), value);
}
void SetLightUniformVec3(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec3 value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader->setVec3(uniformName.c_str(), value);
}


bool Update() {
    // Cálculo del framerate
    float currentFrame = (float)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Procesa la entrada del teclado o mouse
    processInput(window);

    // --- LÓGICA DE INTERACCIÓN (Proximidad) ---
    g_nearbyObject = nullptr; // Reiniciar cada frame

    // 1. SOLO si NO estamos interactuando, buscamos objetos cercanos
    if (g_interactingObject == nullptr) {
        bool foundNearby = false;
        for (auto& obj : g_interactiveObjects) {
            float distance = glm::distance(position_origin, obj.position); // Distancia del personaje al objeto
            if (distance < obj.triggerRadius) {
                g_nearbyObject = &obj;
                foundNearby = true;
                break; // Encontramos uno, dejamos de buscar
            }
        }

        // --- LÓGICA DEL TEXTO (simulada en consola) ---
        if (foundNearby) {
            // \r mueve el cursor al inicio de la línea (evita spam)
            std::cout << "Presiona F para interactuar. Presiona Y para rotar.    \r";
        }
        else {
            // Limpia la línea
            std::cout << "                                                             \r";
        }
    }
    // --- FIN DE LÓGICA DE INTERACCIÓN ---

    // --- LÓGICA DE AUTO-ROTACIÓN ---
    if (g_interactingObject != nullptr && g_interactingObject->isAutoRotatingY) {
        float rotationSpeed = 1.0f; // 1 radián por segundo
        g_interactingObject->inspectRotationY += rotationSpeed * deltaTime;
    }
    // --- FIN DE LÓGICA DE AUTO-ROTACIÓN ---


    // Renderizado R - G - B - A
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- CÁLCULO DE CÁMARA MODIFICADO ---
    glm::mat4 projection;
    glm::mat4 view;

    // SI ESTAMOS EN MODO INSPECCIÓN (CÁMARA FIJA)
    if (g_interactingObject != nullptr)
    {
        // 1. Definimos el centro del objetivo usando el offset del objeto
        glm::vec3 targetCenter = g_interactingObject->position + g_interactingObject->inspectTargetOffset;

        // 2. Posicionamos la cámara usando el offset de cámara del objeto
        glm::vec3 camPos = g_interactingObject->position + g_interactingObject->inspectCamPos;
        
        // 3. La cámara mira al 'targetCenter'
        view = glm::lookAt(camPos, targetCenter, glm::vec3(0.0f, 1.0f, 0.0f));
        
        // --- ¡CAMBIO AQUÍ! ---
        // 4. Usamos g_inspectZoom (controlado por el scroll) para la proyección
        projection = glm::perspective(glm::radians(g_inspectZoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        // --- FIN DE CAMBIO ---
    }
    // SI ESTAMOS EN MODO NORMAL (explorando)
    else
    {
        if (activeCamera == 0) {
            // Cámara flotante
            projection = glm::perspective(glm::radians(camera_float.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
            view = camera_float.GetViewMatrix();
        }
        else if (activeCamera == 1) {
            // Cámara en tercera persona
            projection = glm::perspective(glm::radians(camera3rd.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
            view = camera3rd.GetViewMatrix();
        }
        else if (activeCamera == 2) {
            // Cámara flotante
            projection = glm::perspective(glm::radians(camera1st.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
            view = camera1st.GetViewMatrix();
        }
    }
    // --- FIN DE CÁLCULO DE CÁMARA ---

    // Cubemap (fondo)
    {
        mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
    }
    {
        mLightsShader->use();
        if (mLightsShader->ID != 0) {

            // Activamos para objetos transparentes
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            mLightsShader->setMat4("projection", projection);
            mLightsShader->setMat4("view", view);

            // Configuramos propiedades de fuentes de luz
            mLightsShader->setInt("numLights", (int)gLights.size());
            for (size_t i = 0; i < gLights.size(); ++i) {
                SetLightUniformVec3(mLightsShader, "Position", i, gLights[i].Position);
                SetLightUniformVec3(mLightsShader, "Direction", i, gLights[i].Direction);
                SetLightUniformVec4(mLightsShader, "Color", i, gLights[i].Color);
                SetLightUniformVec4(mLightsShader, "Power", i, gLights[i].Power);
                SetLightUniformInt(mLightsShader, "alphaIndex", i, gLights[i].alphaIndex);
                SetLightUniformFloat(mLightsShader, "distance", i, gLights[i].distance);
            }

            mLightsShader->setVec3("eye", camera1st.Position);

            // Aplicamos propiedades materiales
            mLightsShader->setVec4("MaterialAmbientColor", material01.ambient);
            mLightsShader->setVec4("MaterialDiffuseColor", material01.diffuse);
            mLightsShader->setVec4("MaterialSpecularColor", material01.specular);
            mLightsShader->setFloat("transparency", material01.transparency);

            // --- BUCLE DE DIBUJADO MODIFICADO ---

            // 1. Dibujar el entorno (el museo)
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            mLightsShader->setMat4("model", model);
            museo->Draw(*mLightsShader);

            // 2. Dibujar TODOS los objetos interactivos
            for (auto& obj : g_interactiveObjects) {
                model = glm::mat4(1.0f);
                // 1. Mover al lugar correcto
                model = glm::translate(model, obj.position);

                // Aplicamos las rotaciones de inspección guardadas en el objeto
                // 2. Rotar en Y (controlado por auto-rotación)
                model = glm::rotate(model, obj.inspectRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
                // 3. Rotar en X (ya no se usa)
                model = glm::rotate(model, obj.inspectRotationX, glm::vec3(1.0f, 0.0f, 0.0f));
                // --- FIN DE NUEVA LÓGICA ---

                // 4. Aplicar rotación base de FBX (la que tenías)
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

                mLightsShader->setMat4("model", model);
                obj.model->Draw(*mLightsShader); // Dibuja el modelo del objeto
            }
            // --- FIN DE BUCLE DE DIBUJADO ---
        }

        glUseProgram(0);

    }
    //Dibujo de cajas para debug
    if (showCollisionBoxes) {
        if (!debugShader || debugShader->ID == 0) {
            std::cout << "Shader debug no disponible" << std::endl;
            showCollisionBoxes = false;
        }
        else {
            // Solo dibujar si el shader está listo
            for (const auto& obj : g_collidableObjects) {
                AABB worldAABB = CalculateWorldAABB(obj);
                DrawBoundingBox(worldAABB, glm::vec3(1.0f, 0.0f, 0.0f), projection, view);
            }

            AABB charAABB = GetCharacterBoundingBox();
            DrawBoundingBox(charAABB, glm::vec3(0.0f, 1.0f, 0.0f), projection, view);

            for (const auto& obj : g_interactiveObjects) {
                AABB triggerBox;
                triggerBox.min = obj.position - glm::vec3(obj.triggerRadius, 0.1f, obj.triggerRadius);
                triggerBox.max = obj.position + glm::vec3(obj.triggerRadius, 2.0f, obj.triggerRadius);
                DrawBoundingBox(triggerBox, glm::vec3(0.0f, 0.0f, 1.0f), projection, view);
            }
        }
    }

    // Objeto animado
    {
        character01->UpdateAnimation(deltaTime);

        // Activación del shader del personaje
        dynamicShader->use();

        // Aplicamos transformaciones de proyección y cámara (si las hubiera)
        dynamicShader->setMat4("projection", projection);
        dynamicShader->setMat4("view", view);

        // Aplicamos transformaciones del modelo
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, character_position); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0, 1.0f, 0.0f));

        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));  // it's a bit too big for our scene, so scale it down

        dynamicShader->setMat4("model", model);

        dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, character01->gBones);

        // Dibujamos el modelo
        character01->Draw(*dynamicShader);
    }
    

    glUseProgram(0);

    // glfw: swap buffers 
    glfwSwapBuffers(window);
    glfwPollEvents();


    return true;
}


bool CheckCollision(const AABB& a, const AABB& b) {
    bool collisionX = (a.min.x <= b.max.x && a.max.x >= b.min.x);
    bool collisionY = (a.min.y <= b.max.y && a.max.y >= b.min.y);
    bool collisionZ = (a.min.z <= b.max.z && a.max.z >= b.min.z);
    bool collision = collisionX && collisionY && collisionZ;

    if (collision) {
        std::cout << "=== COLISIÓN DETECTADA ===" << std::endl;
        std::cout << "Eje X: " << (collisionX ? "SÍ" : "NO") << std::endl;
        std::cout << "Eje Y: " << (collisionY ? "SÍ" : "NO") << std::endl;
        std::cout << "Eje Z: " << (collisionZ ? "SÍ" : "NO") << std::endl;
        std::cout << "Personaje - Min: (" << a.min.x << ", " << a.min.z << ") Max: (" << a.max.x << ", " << a.max.z << ")" << std::endl;
        std::cout << "Objeto    - Min: (" << b.min.x << ", " << b.min.z << ") Max: (" << b.max.x << ", " << b.max.z << ")" << std::endl;
    }

    return collision;
}

AABB CalculateWorldAABB(const CollidableObject& obj) {
    AABB worldAABB;
    worldAABB.min = obj.position + obj.boundingBox.min * obj.scale;
    worldAABB.max = obj.position + obj.boundingBox.max * obj.scale;
    return worldAABB;
}

AABB GetCharacterBoundingBox() {
    AABB charAABB;
    charAABB.min = character_position - glm::vec3(characterRadius, 0.0f, characterRadius);
    charAABB.max = character_position + glm::vec3(characterRadius, characterHeight, characterRadius);
    return charAABB;
}

bool CheckCharacterCollision() {

    AABB charAABB = GetCharacterBoundingBox();

    for (int i = 0; i < g_collidableObjects.size(); i++) {
        AABB objAABB = CalculateWorldAABB(g_collidableObjects[i]);
        bool collision = CheckCollision(charAABB, objAABB);

        if(collision) {
            return true;
        }

    }
    return false;
}
bool CheckCollisionAtPosition(const glm::vec3& position) {
    glm::vec3 oldPosition = character_position;

    character_position = position;
    bool collision = CheckCharacterCollision();

    character_position = oldPosition;

    return collision;
}

void UpdateInteractionsWithCollision() {
    g_nearbyObject = nullptr;

    if (g_interactingObject == nullptr) {
        for (auto& obj : g_interactiveObjects) {
            float distance = glm::distance(position_origin, obj.position);
            if (distance < obj.triggerRadius) {
                g_nearbyObject = &obj;
                std::cout << "Presiona F para interactuar. Presiona Y para rotar.    \r";
                break;
            }
        }
    }
}

void InitializeCollidableObjects() {
    // Museo
    //g_collidableObjects.push_back({museo, glm::vec3(0.0f, 0.0f, 0.0f), 
    //    AABB(glm::vec3(-0.5f, 0.0f, -0.5f),   // ← MUCHO más pequeño
    //        glm::vec3(0.5f, 2.0f, 0.5f)),    // ← MUCHO más pequeño
    //        glm::vec3(1.0f), 0.0f
    //});

    // Estatuas y objetos interactivos
    g_collidableObjects.push_back({ Xiucoatl, estatuaPos,
                                  AABB(glm::vec3(-4.0f, 0.0f, -4.0f),   // min (x, y, z)
                                        glm::vec3(4.0f, 10.0f, 4.0f)),  // max (x, y, z),    // ← Más pequeño
                                        glm::vec3(1.0f), 0.0f });
    /*
    g_collidableObjects.push_back({ piramide, piramidePos,
                                  AABB(glm::vec3(-3.0f, 0.0f, -3.0f),
                                       glm::vec3(3.0f, 4.0f, 3.0f)),
                                  glm::vec3(1.0f), 0.0f });

    // Agregar más objetos aquí...
    g_collidableObjects.push_back({ PiedraDelSol, PiedraSolPos,
                                  AABB(glm::vec3(-4.0f, 0.0f, -4.0f),
                                       glm::vec3(4.0f, 2.0f, 4.0f)),
                                  glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Coatlicue, CoatlicuePos,
                                  AABB(glm::vec3(-1.5f, 0.0f, -1.5f),
                                       glm::vec3(1.5f, 3.0f, 1.5f)),
                                  glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ PlatoAntiguo, PlatoAntiguoPos,
                                  AABB(glm::vec3(-1.0f, 0.0f, -1.0f),
                                       glm::vec3(1.0f, 0.5f, 1.0f)),
                                  glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Craneo, CraneoPos,
                                  AABB(glm::vec3(-0.5f, 0.0f, -0.5f),
                                       glm::vec3(0.5f, 0.5f, 0.5f)),
                                  glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Incenciario, IncenciarioPos,
                                  AABB(glm::vec3(-1.0f, 0.0f, -1.0f),
                                       glm::vec3(1.0f, 2.0f, 1.0f)),
                                  glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Xochipilli, XochipilliPos,
                                  AABB(glm::vec3(-1.0f, 0.0f, -1.0f),
                                       glm::vec3(1.0f, 2.0f, 1.0f)),
                                  glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Bracero, BraceroPos,
                                  AABB(glm::vec3(-1.0f, 0.0f, -1.0f),
                                       glm::vec3(1.0f, 1.5f, 1.0f)),
                                  glm::vec3(1.0f), 0.0f });*/
}

// Implementaciones (después de las otras funciones, antes de processInput)

void processInput(GLFWwindow* window)
{
    static bool f1Pressed = false, f2Pressed = false, f3Pressed = false;
    static bool keyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (character_run == true) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera_float.ProcessKeyboard(FORWARD, deltaTime * 4.0f);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera_float.ProcessKeyboard(BACKWARD, deltaTime * 4.0f);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera_float.ProcessKeyboard(LEFT, deltaTime * 4.0f);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera_float.ProcessKeyboard(RIGHT, deltaTime * 4.0f);
    }
    else {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera_float.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera_float.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera_float.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera_float.ProcessKeyboard(RIGHT, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!keyPressed) {
            showCollisionBoxes = !showCollisionBoxes;
            if (showCollisionBoxes && (!debugShader || debugShader->ID == 0)) {
                std::cout << "No se puede activar debug - shader no disponible" << std::endl;
                showCollisionBoxes = false;
            }
            else {
                std::cout << "Debug colisiones: " << (showCollisionBoxes ? "ACTIVADO" : "DESACTIVADO") << std::endl;
            }
            keyPressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        keyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        if (!character_run) {
            character_run = true;
            scaleV = runSpeed;
            std::cout << "Modo CARRERA activado - Velocidad: " << scaleV << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        if (character_run) {
            character_run = false;
            scaleV = walkSpeed;
            std::cout << "Modo CAMINATA activado - Velocidad: " << scaleV << std::endl;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!g_f_keyPressed) {
            
            // CASO 1: Empezar a interactuar
            if (g_interactingObject == nullptr && g_nearbyObject != nullptr) {
                g_interactingObject = g_nearbyObject;
                g_nearbyObject = nullptr; 
                std::cout << "                                                      \r"; 
                std::cout << "Interactuando con " << g_interactingObject->name << ". Presiona F para salir. Presiona Y para rotar/detener.\n";

                g_inspectZoom = 45.0f; 
            }
            // CASO 2: Dejar de interactuar
            else if (g_interactingObject != nullptr) {
                
                // Reseteamos el estado del objeto a su original al salir
                g_interactingObject->inspectRotationY = 0.0f;
                g_interactingObject->inspectRotationX = 0.0f;
                g_interactingObject->isAutoRotatingY = false; // Detenemos la rotación

                g_interactingObject = nullptr;
            }
        }
        g_f_keyPressed = true; // Marcamos que la tecla está presionada
    }
    
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        g_f_keyPressed = false; // Reseteamos la bandera cuando se suelta
    }

    if (g_interactingObject != nullptr && glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        if (!g_y_keyPressed) {
            g_interactingObject->isAutoRotatingY = !g_interactingObject->isAutoRotatingY;
            std::cout << "                                                      \r"; 
            if (g_interactingObject->isAutoRotatingY) {
                std::cout << "Rotacion activada. Presiona Y para detener. Presiona F para salir.\n";
            } else {
                std::cout << "Rotacion detenida. Presiona Y para activar. Presiona F para salir.\n";
            }
        }
        g_y_keyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_RELEASE) {
        g_y_keyPressed = false;
    }

    // --- CONGELAR MOVIMIENTO MIENTRAS SE INTERACTÚA ---
    // Solo permitimos mover al personaje si NO estamos en modo interacción
    if (g_interactingObject == nullptr) {

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            glm::vec3 newPosition = character_position + scaleV * forwardView;
            glm::vec3 oldPosition = character_position;

            if (!CheckCollisionAtPosition(newPosition)) {
                camera3rd.ProcessKeyboard(FORWARD, deltaTime);
                character_position = newPosition;

                UpdateCameras();
            }
            else {
                std::cout << "COLISIÓN - Movimiento bloqueado" << std::endl;

                character_position = oldPosition;
                UpdateCameras();

            }

        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            glm::vec3 newPosition = character_position - scaleV * forwardView;  
            glm::vec3 oldPosition = character_position;

            if (!CheckCollisionAtPosition(newPosition)) {
                camera3rd.ProcessKeyboard(FORWARD, deltaTime);
                character_position = newPosition;

                UpdateCameras();
            }
            else {
                std::cout << "COLISIÓN - Movimiento bloqueado" << std::endl;

                character_position = oldPosition;
                UpdateCameras();

            }
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            rotateCharacter += 0.2f;
            if (character_run) {
                rotateCharacter += 2.0f;
            }
            UpdateCameras();
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            rotateCharacter -= 0.2f;
            if (character_run) {
                rotateCharacter -= 2.0f;
            }

            UpdateCameras();
        }
    } 
    // --- FIN DE CONGELAR MOVIMIENTO ---

    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
        if (!f1Pressed) {
            activeCamera = 0;
            std::cout << "float" << std::endl;
            f1Pressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
        if (!f2Pressed) {
            activeCamera = 1;
            std::cout << "3rd person." << std::endl;
            f2Pressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE) {
        f2Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
        if (!f3Pressed) {
            activeCamera = 2;
            std::cout << "1st person." << std::endl;
            f3Pressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) {
        f3Pressed = false;
    }
}

// glfw: Actualizamos el puerto de vista si hay cambios del tamaño
// de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// glfw: Callback del movimiento y eventos del mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    // --- LÓGICA DE MOUSE MODIFICADA ---
    // SI ESTAMOS EN MODO INSPECCIÓN
    if (g_interactingObject != nullptr)
    {
        // El mouse ya no hace nada en este modo de inspección, 
        // la rotación es por teclado.
    }
    // SI ESTAMOS EN MODO NORMAL
    else
    {
        camera_float.ProcessMouseMovement(xoffset, yoffset);
    }
    // --- FIN DE LÓGICA MODIFICADA ---
}

// glfw: Complemento para el movimiento y eventos del mouse
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // --- LÓGICA DE SCROLL MODIFICADA ---
    // SI ESTAMOS EN MODO INSPECCIÓN
    if (g_interactingObject != nullptr)
    {
        // --- ¡CAMBIO AQUÍ! ---
        // El scroll ahora controla el FOV (zoom)
        g_inspectZoom -= (float)yoffset;
        if (g_inspectZoom < 1.0f)
            g_inspectZoom = 1.0f; // Límite de zoom (muy cerca)
        if (g_inspectZoom > 45.0f)
            g_inspectZoom = 45.0f; // Límite de zoom (normal)
    }
    // SI ESTAMOS EN MODO NORMAL
    else
    {
        camera_float.ProcessMouseScroll((float)yoffset);
    }
    // --- FIN DE LÓGICA MODIFICADA ---
}


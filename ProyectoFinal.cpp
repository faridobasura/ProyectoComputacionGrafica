/*
* * Proyecto Final
*/
#include <iostream>
#include <sstream> // Necesario para SetLightUniform...
#include <vector>  // Para usar std::vector
#include <string>  // Para usar std::string

// GLAD: Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator
// https://glad.dav1d.de/
#include <glad/glad.h>

// Cargar antes glew
#include <text_render.h>
#include <TextManager.h> // <-- Declarado pero no usado, lo dejo por si acaso


// GLFW: https://www.glfw.org/
#include <GLFW/glfw3.h>

#include <stdlib.h>

// GLM: OpenGL Math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp> // <-- ¡ENCABEZADO NECESARIO PARA glm::lerp!

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

// --- ¡STRUCT MODIFICADO! ---
struct InteractiveObject {
    Model* model;
    glm::vec3   position;
    float       triggerRadius;
    std::string name;

    float       inspectRotationY;
    float       inspectRotationX;
    bool        isAutoRotatingY;

    // Cámara para el objeto principal
    glm::vec3   mainInspectTargetOffset;
    glm::vec3   mainInspectCamPos;

    // Info para el letrero
    Model* infoStandModel;
    glm::vec3   infoStandPos; // Posición RELATIVA al objeto principal
    glm::vec3   infoInspectTargetOffset; // Objetivo de cámara RELATIVO al letrero
    glm::vec3   infoInspectCamPos;       // Posición de cámara RELATIVA al letrero


    InteractiveObject(Model* m, glm::vec3 pos, float radius, std::string n,
        glm::vec3 mainTargetOffset, glm::vec3 mainCamPos,
        Model* infoModel, glm::vec3 infoPos,
        glm::vec3 infoTargetOffset, glm::vec3 infoCamPos) :

        model(m), position(pos), triggerRadius(radius), name(n),
        inspectRotationY(0.0f), inspectRotationX(0.0f), isAutoRotatingY(false),
        mainInspectTargetOffset(mainTargetOffset), mainInspectCamPos(mainCamPos),
        infoStandModel(infoModel), infoStandPos(infoPos),
        infoInspectTargetOffset(infoTargetOffset), infoInspectCamPos(infoCamPos)
    {
    }
};
// --- FIN DE STRUCT ---

// --- ¡NUEVO! STRUCT PARA LAS PUERTAS ---
enum DoorState { CLOSED, OPENING, OPEN, CLOSING };

struct Door {
    Model* model;
    glm::vec3   initialPosition; // Posición cerrada
    glm::vec3   openPosition;    // Posición abierta
    glm::vec3   currentPosition; // Posición actual
    AABB        boundingBox;     // Caja de colisión local
    float       triggerRadius;
    DoorState   state;
    float       animSpeed;
    float       animProgress;    // 0.0 = cerrada, 1.0 = abierta

    Door(Model* m, glm::vec3 closed, glm::vec3 open, float radius, AABB box) :
        model(m), initialPosition(closed), openPosition(open), currentPosition(closed),
        triggerRadius(radius), boundingBox(box), state(CLOSED), animSpeed(1.0f), animProgress(0.0f)
    {
    }
};
// --- FIN DE STRUCT ---

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
GLuint textShaderID;

//Interaccion
std::vector<InteractiveObject> g_interactiveObjects; // Lista de exhibiciones
InteractiveObject* g_nearbyObject = nullptr;      // Exhibición más cercana (para "Presiona F")
InteractiveObject* g_interactingObject = nullptr; // Exhibición con la que estamos interactuando
bool g_f_keyPressed = false; // Para detectar una sola pulsación de 'F'
bool g_y_keyPressed = false; // Para detectar una sola pulsación de 'Y'

// --- ¡MODIFICADO! Variables de Puerta ---
std::vector<Door> g_doors;           // Lista de todas las puertas
bool g_isNearDoors = false;       // <-- Usamos una simple bandera
bool g_e_keyPressed = false;      // Para la tecla "E" (Usar)
// --- FIN DE MODIFICADO ---

// --- ¡NUEVO! Variables de Inspección ---
bool g_isInspectingInfoStand = false; // ¿Estamos viendo el letrero?
bool g_g_keyPressed = false;          // Para la tecla 'G'
// --- FIN DE NUEVO ---


// --- VARIABLES CÁMARA DE INSPECCIÓN ---
float g_inspectZoom = 45.0f;

TextRenderer textRenderer; // <-- Corregido
TextManager textManager(textRenderer, 1024, 768); // <-- Declarado, pero no usado

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
float walkSpeed = 0.1f;
float runSpeed = 0.3f;
float       scaleV = walkSpeed;

float characterHeight = 2.0f;      // Altura del personaje
float characterRadius = 0.5f;      // Radio para colisión cilíndrica
float collisionOffset = 1.0f;      // Margen de seguridad

// Definición de cámara (posición en XYZ)
Camera camera_float(glm::vec3(0.0f, 2.0f, 10.0f));
Camera camera1st = character_position + glm::vec3(0.0f, 1.0f, 0.0f);
Camera camera3rd = character_position + glm::vec3(0.0f, 1.3f, -0.05f);

// Controladores para el movimiento del mouse
float lastX = SCR_WIDTH / 4.0f;
float lastY = SCR_HEIGHT / 4.0f;
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
bool showHelp = false; // Variable para mostrar/ocultar ayuda

glm::vec3 position(0.0f, 0.0f, 0.0f);
glm::vec3 position_origin(0.0f, 0.0f, 0.0f);

// --- POSICIONES DE OBJETOS ---
glm::vec3 estatuaPos = position_origin;
glm::vec3 piramidePos = glm::vec3(0.0f, 0.0f, -25.0f);
glm::vec3 PiedraSolPos = glm::vec3(0.0f, 0.0f, -77.3f);
glm::vec3 CoatlicuePos = glm::vec3(-47.55f, 0.0f, -95.0f);
glm::vec3 PlatoAntiguoPos = glm::vec3(-41.6f, -1.9f, -71.74f);
glm::vec3 CraneoPos = glm::vec3(-25.39f, -2.18f, -117.27f);
glm::vec3 IncenciarioPos = glm::vec3(36.63f, 0.0f, -112.64f);
glm::vec3 XochipilliPos = glm::vec3(51.0f, 0.02f, -93.0f);
glm::vec3 BraceroPos = glm::vec3(37.94f, 0.10f, -70.74f);

glm::vec3 pisoPos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 estante1Pos = glm::vec3(28.64f, 0.0f, -12.3f);
glm::vec3 estante2Pos = glm::vec3(-29.181f, 0.0f, -14.0f);
glm::vec3 estante3Pos = glm::vec3(19.64f, 0.0f, -42.3f);
glm::vec3 estante4Pos = glm::vec3(-19.181f, 0.0f, -42.93f);
glm::vec3 estante5Pos = glm::vec3(65.0f, 0.0f, -105.93f);
glm::vec3 estante6Pos = glm::vec3(4.0f, 0.0f, -115.3f);
glm::vec3 estante7Pos = glm::vec3(-60.64f, 0.0f, -80.93f);

glm::vec3 paredes_estantesPos = glm::vec3(0.0f, 0.0f, -45.00f);

glm::vec3 caracolPos = glm::vec3(65.0f, 3.7f, -81.00f);

glm::vec3 cuadroPos = glm::vec3(-65.0f, 2.0f, -107.00f);




Shader* mLightsShader;
Shader* proceduralShader;
Shader* wavesShader;
Shader* debugShader;
Shader* cubemapShader;
Shader* dynamicShader;
Shader* textShader;
Shader* glassShader; // <-- ¡NUEVO!

// Carga la información de los modelo
Model* museo; // Entorno

// --- PUNTEROS PARA OBJETOS ---
Model* piso;
Model* estante1;
Model* estante2;
Model* estante3;
Model* estante4;
Model* estante5;
Model* estante6;
Model* estante7;

Model* paredes_estantes;

Model* puertaModel;
Model* infoStandModel; // <-- ¡NUEVO!

// --- ¡NUEVO! Punteros para tus 10 objetos ---
Model* CuadroInformativo1;
Model* CuadroCraneo;
Model* CuadroPiramide;
Model* CuadroPlato;
Model* CuadroCoatlicue;
Model* CuadroPiedra;
Model* CuadroXochipilli;
Model* CuadroIncenciario;
Model* CuadroBracero;

// --- FIN DE NUEVO ---

Model* Xiucoatl;
Model* piramide;
Model* PiedraDelSol;
Model* Coatlicue;
Model* PlatoAntiguo;
Model* Craneo;
Model* Incenciario;
Model* Xochipilli;
Model* Bracero;
Model* caracol;
Model* cuadro;

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

static void CreateDebugCube() {
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

static void DrawBoundingBox(const AABB& box, const glm::vec3& color, glm::mat4 projection, glm::mat4 view) {
    if (!showCollisionBoxes) return;

    debugShader->use();

    // Calcular centro y tamaño
    glm::vec3 center = (box.min + box.max) * 0.5f;
    glm::vec3 size = box.max - box.min;

    // Matriz de transformación
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, center);
    model = glm::scale(model, size);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    debugShader->setMat4("model", model);
    debugShader->setMat4("view", view);
    debugShader->setMat4("projection", projection);
    debugShader->setVec3("color", color);

    // Transparencia (por ejemplo, 50%)
    debugShader->setFloat("alpha", 0.5f);
    debugShader->setBool("useAlpha", true);

    glBindVertexArray(debugVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

}
static void DrawLightDebug(glm::mat4 projection, glm::mat4 view) {
    if (!showCollisionBoxes) return;

    for (const auto& light : gLights) {
        AABB lightVolume;
        float lightSize = 0.5f; // Tamaño visual de la luz
        lightVolume.min = light.Position - glm::vec3(lightSize);
        lightVolume.max = light.Position + glm::vec3(lightSize);

        glm::vec3 lightColor = glm::vec3(light.Color.r, light.Color.g, light.Color.b);

        DrawBoundingBox(lightVolume, lightColor, projection, view);
    }
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
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proyecto Final", NULL, NULL);
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

    mLightsShader = new Shader("shaders/11_PhongShaderMultLights.vs", "shaders/11_PhongShaderMultLights.fs");
    proceduralShader = new Shader("shaders/12_ProceduralAnimation.vs", "shaders/12_ProceduralAnimation.fs");
    wavesShader = new Shader("shaders/13_wavesAnimation.vs", "shaders/13_wavesAnimation.fs");
    cubemapShader = new Shader("shaders/10_vertex_cubemap.vs", "shaders/10_fragment_cubemap.fs");
    dynamicShader = new Shader("shaders/10_vertex_skinning-IT.vs", "shaders/10_fragment_skinning-IT.fs");
    debugShader = new Shader("shaders/shader_debug.vs", "shaders/shader_debug.fs");
    textShader = new Shader("shaders/text_shader.vs", "shaders/text_shader.fs");
    glassShader = new Shader("shaders/glass.vs", "shaders/glass.fs"); // <-- ¡NUEVO!

    textShaderID = textShader->ID;

    // textManager.textShaderID = textShaderID; // Esto es de TextManager, no de TextRenderer

    if (!glfwGetCurrentContext()) {
        std::cerr << "Error: No hay un contexto de OpenGL activo antes de inicializar TextRenderer\n";
    }
    else {
        try {
            // Usa el objeto global 'textRenderer' que declaraste
            textRenderer.Init(textShaderID, SCR_WIDTH, SCR_HEIGHT, "fonts/cambriab.ttf");
        }
        catch (const std::exception& e) {
            std::cerr << "Error al inicializar texto: " << e.what() << std::endl;
        }
    }

    CreateDebugCube();

    // Máximo número de huesos: 100
    dynamicShader->setBonesIDs(MAX_RIGGING_BONES);

    // --- Carga de modelos modulares ---
    museo = new Model("models/IllumModels/proyectofinal/Entorno1.fbx");
    piso = new Model("models/IllumModels/proyectofinal/Piso.fbx");
    puertaModel = new Model("models/IllumModels/proyectofinal/puerta.fbx");

    // Estantes
    estante1 = new Model("models/IllumModels/proyectofinal/estante1.fbx");
    estante2 = new Model("models/IllumModels/proyectofinal/estante2.fbx");
    estante3 = new Model("models/IllumModels/proyectofinal/estante3.fbx");
    estante4 = new Model("models/IllumModels/proyectofinal/estante4.fbx");
    estante5 = new Model("models/IllumModels/proyectofinal/estante5.fbx");
    estante6 = new Model("models/IllumModels/proyectofinal/estante6.fbx");
    estante7 = new Model("models/IllumModels/proyectofinal/estante7.fbx");
    paredes_estantes = new Model("models/IllumModels/proyectofinal/paredes_estantes.fbx");

    // Otros objetos estáticos
    caracol = new Model("models/IllumModels/proyectofinal/caracol.fbx");
    cuadro = new Model("models/IllumModels/proyectofinal/cuadro.fbx");

    // --- ¡NUEVO! Cargar tus 10 modelos estáticos ---
    // (Usa los modelos existentes como placeholders, cámbialos por tus archivos .fbx)
    CuadroInformativo1 = new Model("models/IllumModels/proyectofinal/CuadroInformativo.fbx"); // ¡CAMBIAR!
    CuadroCraneo = new Model("models/IllumModels/proyectofinal/CuadroCraneo.fbx");  // ¡CAMBIAR!
    CuadroPiramide = new Model("models/IllumModels/proyectofinal/CuadroPiramide.fbx"); // ¡CAMBIAR!
    CuadroPlato = new Model("models/IllumModels/proyectofinal/CuadroPlato.fbx");  // ¡CAMBIAR!
    CuadroCoatlicue = new Model("models/IllumModels/proyectofinal/CuadroCoatlicue.fbx"); // ¡CAMBIAR!
    CuadroPiedra = new Model("models/IllumModels/proyectofinal/CuadroPiedra.fbx");  // ¡CAMBIAR!
    CuadroXochipilli = new Model("models/IllumModels/proyectofinal/CuadroXochipilli.fbx"); // ¡CAMBIAR!
    CuadroIncenciario = new Model("models/IllumModels/proyectofinal/CuadroIncenciario.fbx");  // ¡CAMBIAR!
    CuadroBracero = new Model("models/IllumModels/proyectofinal/CuadroBracero.fbx"); // ¡CAMBIAR!
    

    // Modelos Interactivos
    Xiucoatl = new Model("models/IllumModels/proyectofinal/xiucoatl.fbx");
    piramide = new Model("models/IllumModels/proyectofinal/piramides.fbx");
    PiedraDelSol = new Model("models/IllumModels/proyectofinal/PiedraDelSol.fbx");
    Coatlicue = new Model("models/IllumModels/proyectofinal/Coatlicue.fbx");
    PlatoAntiguo = new Model("models/IllumModels/proyectofinal/PlatoAntiguo.fbx");
    Craneo = new Model("models/IllumModels/proyectofinal/Craneo.fbx");
    Incenciario = new Model("models/IllumModels/proyectofinal/Incenciario.fbx");
    Xochipilli = new Model("models/IllumModels/proyectofinal/Xochipilli.fbx");
    Bracero = new Model("models/IllumModels/proyectofinal/Bracero.fbx");
    character01 = new AnimatedModel("models/IllumModels/proyectofinal/personaje2.fbx");

    // --- ¡MODIFICADO! Configuración de cámaras de inspección ---
    // Formato: (Modelo, Pos, Radio, Nombre, 
    //           TargetObj, CamPosObj,         <-- Cámara para el objeto
    //           ModeloLetrero, PosLetrero,    <-- Modelo y Posición del letrero
    //           TargetLetrero, CamPosLetrero) <-- Cámara para el letrero

    g_interactiveObjects.emplace_back(Xiucoatl, estatuaPos, 6.0f, "Estatua",
        glm::vec3(0.0f, 5.0f, 1.0f),  // Target Objeto
        glm::vec3(0.0f, 5.0f, 11.0f), // CamPos Objeto
        CuadroPiedra,             // ¡Modelo de letrero! (Cambia si es específico)
        glm::vec3(5.0f, 0.0f, 3.0f),  // Posición del letrero (relativa a la estatua) ¡AJUSTA ESTO!
        glm::vec3(-5.0f, 3.5f, 0.0f),  // Target Letrero (relativo al letrero)
        glm::vec3(7.0f, 4.0f, 0.0f)   // CamPos Letrero (relativo al letrero)
    );

    g_interactiveObjects.emplace_back(piramide, piramidePos, 6.0f, "Piramides",
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(0.0f, 5.0f, 12.0f),
        CuadroPiramide,
        glm::vec3(6.0f, 0.0f, 3.0f),  // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(-5.0f, 4.0f, 0.0f),
        glm::vec3(8.0f, 4.0f, 0.0f)
    );

    g_interactiveObjects.emplace_back(PiedraDelSol, PiedraSolPos, 19.0f, "PiedraSol",
        glm::vec3(0.0f, 7.0f, 0.0f),
        glm::vec3(0.0f, 7.0f, 15.0f),
        CuadroInformativo1,
        glm::vec3(-9.32f, -0.16f, 16.0), // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(0.0f, 4.0f, 0.0f),
        glm::vec3(0.0f, 5.0f, 7.0f)
    );

    g_interactiveObjects.emplace_back(Coatlicue, CoatlicuePos, 4.0f, "Coatlicue",
        glm::vec3(-5.0f, 6.0f, 0.0f),
        glm::vec3(16.0f, 6.0f, 0.0f),
        CuadroCoatlicue,
        glm::vec3(1.0f, 0.0f, 6.0f), // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(-5.0f, 3.5f, 0.0f),
        glm::vec3(8.0f, 4.0f, 0.0f)
    );

    g_interactiveObjects.emplace_back(PlatoAntiguo, PlatoAntiguoPos, 4.0f, "PlatoAntiguo",
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 6.0f, -7.0f),
        CuadroPlato,
        glm::vec3(4.0f, 2.0f, 1.0f), // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(0.0f, 3.5f, 0.0f),
        glm::vec3(0.0f, 4.0f, -8.0f)
    );

    g_interactiveObjects.emplace_back(Craneo, CraneoPos, 4.0f, "Craneo",
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 6.0f, 3.0f),
        CuadroCraneo,
        glm::vec3(-3.5f, 2.5f, 2.0f), // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(0.0f, 3.5f, 0.0f),
        glm::vec3(0.0f, 4.0f, 5.0f)
    );

    g_interactiveObjects.emplace_back(Incenciario, IncenciarioPos, 5.0f, "Incenciario",
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 6.0f, 8.0f),
        CuadroIncenciario,
        glm::vec3(5.5f, 0.0f, 2.0f), // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(0.0f, 3.5f, 0.0f),
        glm::vec3(0.0f, 4.0f, 8.0f)
    );

    g_interactiveObjects.emplace_back(Xochipilli, XochipilliPos, 5.0f, "Xochipilli",
        glm::vec3(5.0f, 5.0f, 0.0f),
        glm::vec3(-8.0f, 6.0f, 0.0f),
        CuadroXochipilli,
        glm::vec3(-1.5f, 0.0f, 4.0f), // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(5.0f, 3.5f, 0.0f),
        glm::vec3(-7.0f, 4.0f, 0.0f)
    );

    g_interactiveObjects.emplace_back(Bracero, BraceroPos, 6.0f, "Bracero",
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 5.0f, -7.0f),
        CuadroBracero,
        glm::vec3(4.5f, 0.0f, -2.0f), // Posición del letrero ¡AJUSTA ESTO!
        glm::vec3(0.0f, 3.5f, 0.0f),
        glm::vec3(0.0f, 4.0f, -7.0f)
    );
    // --- Fin de Carga de modelos ---

    // --- ¡MODIFICADO! Definir las puertas ---
    // Basado en tu imagen, X: -5.1117, Y(Blender): -31.297 -> Z(OpenGL): 31.297

    // 1. Define las posiciones CERRADAS
    glm::vec3 puerta1_closedPos = glm::vec3(-5.0f, 0.0f, 31.297f);
    glm::vec3 puerta2_closedPos = glm::vec3(5.0f, 0.0f, 31.297f); // Asumimos simetría

    // 2. Define cuánto se deslizan (¡ajusta este valor!)
    float slideDistance = 7.0f; // La puerta se moverá 7 unidades
    glm::vec3 puerta1_openPos = puerta1_closedPos + glm::vec3(-slideDistance, 0.0f, 0.0f); // Se mueve a la izquierda
    glm::vec3 puerta2_openPos = puerta2_closedPos + glm::vec3(slideDistance, 0.0f, 0.0f); // Se mueve a la derecha

    // 3. Define la caja de colisión LOCAL (relativa al centro de la puerta)
    float puertaAncho = 10.0f;
    float puertaAlto = 10.0f;
    float puertaGrosor = 0.4f;
    AABB puerta_box = AABB(glm::vec3(-puertaAncho / 2.0f, 0.0f, -puertaGrosor / 2.0f),
        glm::vec3(puertaAncho / 2.0f, puertaAlto, puertaGrosor / 2.0f));

    // 4. Añade las puertas a la lista
    g_doors.emplace_back(puertaModel, puerta1_closedPos, puerta1_openPos, 6.0f, puerta_box);
    g_doors.emplace_back(puertaModel, puerta2_closedPos, puerta2_openPos, 6.0f, puerta_box);
    // --- FIN DE MODIFICACIÓN ---


    // Cubemap
    vector<std::string> faces
    {
        "textures/cubemap/01/posx.jpg",
        "textures/cubemap/01/negx.jpg",
        "textures/cubemap/01/posy.jpg",
        "textures/cubemap/01/negy.jpg",
        "textures/cubemap/01/posz.jpg",
        "textures/cubemap/01/negz.jpg"
    };
    mainCubeMap = new CubeMap();
    mainCubeMap->loadCubemap(faces);

    UpdateCameras();

    //Entrada_derecha
    Light light01;
    light01.Position = glm::vec3(11.5f, 20.3f, -1.7f);
    light01.Color = glm::vec4(0.09f, 0.07f, 0.06f, 1.0f);
    light01.Power = glm::vec4(50.0f);

    gLights.push_back(light01);

    //Entrada_izq
    Light light02;
    light02.Position = glm::vec3(-10.0f, 20.3f, -1.7f);
    light02.Color = glm::vec4(0.09f, 0.07f, 0.06f, 1.0f);
    light02.Power = glm::vec4(50.0f);
    gLights.push_back(light02);

    Light light03;
    light03.Position = glm::vec3(22.3f, 22.0f, -88.7f);
    light03.Color = glm::vec4(0.09f, 0.07f, 0.06f, 1.0f);
    light03.Power = glm::vec4(60.0f); // más fuerte
    gLights.push_back(light03);

    Light light04;
    light04.Position = glm::vec3(-16.7f, 22.0f, -88.7f);
    light04.Color = glm::vec4(0.09f, 0.07f, 0.06f, 1.0f);
    light04.Power = glm::vec4(60.0f);

    gLights.push_back(light04);


    // SoundEngine->play2D("sound/EternalGarden.mp3", true);

    InitializeCollidableObjects();


    return true;
}


static void SetLightUniformInt(Shader* shader, const char* propertyName, size_t lightIndex, int value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader->setInt(uniformName.c_str(), value);
}
static void SetLightUniformFloat(Shader* shader, const char* propertyName, size_t lightIndex, float value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader->setFloat(uniformName.c_str(), value);
}
static void SetLightUniformVec4(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec4 value) {
    std::ostringstream ss;
    ss << "allLights[" << lightIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader->setVec4(uniformName.c_str(), value);
}
static void SetLightUniformVec3(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec3 value) {
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
    processInput(window); // <-- MOVIDO AL INICIO

    // --- LÓGICA DE INTERACCIÓN (Proximidad) ---
    g_nearbyObject = nullptr; // Reiniciar cada frame
    g_isNearDoors = false;   // --- ¡MODIFICADO! Reiniciar bandera
    bool foundNearby_flag = false; // Bandera local para saber si se muestra *algún* texto

    // 1. SOLO si NO estamos interactuando, buscamos objetos cercanos
    if (g_interactingObject == nullptr) {

        for (auto& obj : g_interactiveObjects) {
            // --- ¡¡CORREGIDO!! Usa la posición del personaje ---
            float distance = glm::distance(character_position, obj.position);
            if (distance < obj.triggerRadius) {
                g_nearbyObject = &obj;
                foundNearby_flag = true;
                break; // Encontramos uno, dejamos de buscar
            }
        }

        // Si no encontramos un objeto F, buscar una puerta E
        if (g_nearbyObject == nullptr) {
            for (auto& door : g_doors) {
                // Comprueba la distancia a la POSICIÓN CERRADA (o un punto medio)
                float distance = glm::distance(character_position, door.initialPosition);
                if (distance < door.triggerRadius) {
                    g_isNearDoors = true; // --- ¡MODIFICADO!
                    foundNearby_flag = true;
                    break;
                }
            }
        }
    }
    // --- FIN DE LÓGICA DE INTERACCIÓN ---

    // --- ¡NUEVO! LÓGICA DE ANIMACIÓN DE PUERTAS ---
    for (auto& door : g_doors) {
        if (door.state == OPENING) {
            door.animProgress += door.animSpeed * deltaTime;
            if (door.animProgress >= 1.0f) {
                door.animProgress = 1.0f;
                door.state = OPEN;
            }
            door.currentPosition = glm::lerp(door.initialPosition, door.openPosition, door.animProgress);
        }
        else if (door.state == CLOSING) {
            door.animProgress -= door.animSpeed * deltaTime;
            if (door.animProgress <= 0.0f) {
                door.animProgress = 0.0f;
                door.state = CLOSED;
            }
            door.currentPosition = glm::lerp(door.initialPosition, door.openPosition, door.animProgress);
        }
    }
    // --- FIN DE LÓGICA DE PUERTAS ---

    // --- LÓGICA DE AUTO-ROTACIÓN ---
    if (g_interactingObject != nullptr && g_interactingObject->isAutoRotatingY) {
        float rotationSpeed = 1.0f; // 1 radián por segundo
        g_interactingObject->inspectRotationY += rotationSpeed * deltaTime;
    }
    // --- FIN DE LÓGICA DE AUTO-ROTACIÓN ---

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- CÁLCULO DE CÁMARA MODIFICADO ---
    glm::mat4 projection;
    glm::mat4 view;

    // SI ESTAMOS EN MODO INSPECCIÓN (CÁMARA FIJA)
    if (g_interactingObject != nullptr)
    {
        // --- ¡MODIFICADO! Comprueba si vemos el letrero o el objeto ---
        glm::vec3 targetCenter;
        glm::vec3 camPos;

        if (g_isInspectingInfoStand) {
            // --- Focus en el Letrero ---
            // 1. Calcula la pos absoluta del letrero
            glm::vec3 standPos = g_interactingObject->position + g_interactingObject->infoStandPos;
            // 2. Calcula el objetivo de la cámara
            targetCenter = standPos + g_interactingObject->infoInspectTargetOffset;
            // 3. Calcula la pos de la cámara
            camPos = standPos + g_interactingObject->infoInspectCamPos;
        }
        else {
            // --- Focus en el Objeto Principal ---
            targetCenter = g_interactingObject->position + g_interactingObject->mainInspectTargetOffset;
            camPos = g_interactingObject->position + g_interactingObject->mainInspectCamPos;
        }
        // --- FIN DE MODIFICACIÓN ---

        view = glm::lookAt(camPos, targetCenter, glm::vec3(0.0f, 1.0f, 0.0f));
        projection = glm::perspective(glm::radians(g_inspectZoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
    }
    // SI ESTAMOS EN MODO NORMAL (explorando)
    else
    {
        if (activeCamera == 0) {
            // Cámara flotante
            projection = glm::perspective(glm::radians(camera_float.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
            view = camera_float.GetViewMatrix();
        }
        else if (activeCamera == 1) {
            // Cámara en tercera persona
            projection = glm::perspective(glm::radians(camera3rd.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
            view = camera3rd.GetViewMatrix();
        }
        else if (activeCamera == 2) {
            // Cámara en primera persona
            projection = glm::perspective(glm::radians(camera1st.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f); // <-- CORREGIDO
            view = camera1st.GetViewMatrix();
        }

    }
    // --- FIN DE CÁLCULO DE CÁMARA ---

    //Skybbox
    {
        mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
    }

    // --- PASO 1: DIBUJAR TODOS LOS OBJETOS OPACOS ---
    {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        mLightsShader->use();
        if (mLightsShader->ID != 0) {

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

            if (g_interactingObject != nullptr) {
                // --- ¡MODIFICADO! Usa la cámara correcta ---
                glm::vec3 camPos;
                if (g_isInspectingInfoStand) {
                    glm::vec3 standPos = g_interactingObject->position + g_interactingObject->infoStandPos;
                    camPos = standPos + g_interactingObject->infoInspectCamPos;
                }
                else {
                    camPos = g_interactingObject->position + g_interactingObject->mainInspectCamPos;
                }
                mLightsShader->setVec3("eye", camPos);
                // --- FIN DE MODIFICACIÓN ---
            }
            else if (activeCamera == 0) {
                mLightsShader->setVec3("eye", camera_float.Position);
            }
            else if (activeCamera == 1) {
                mLightsShader->setVec3("eye", camera3rd.Position);
            }
            else if (activeCamera == 2) {
                mLightsShader->setVec3("eye", camera1st.Position);
            }

            // Aplicamos propiedades materiales
            mLightsShader->setVec4("MaterialAmbientColor", material01.ambient);
            mLightsShader->setVec4("MaterialDiffuseColor", material01.diffuse);
            mLightsShader->setVec4("MaterialSpecularColor", material01.specular);
            mLightsShader->setFloat("transparency", material01.transparency);


            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            mLightsShader->setMat4("model", model);
            museo->Draw(*mLightsShader);

            model = glm::mat4(1.0f);
            model = glm::translate(model, pisoPos);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            mLightsShader->setMat4("model", model);
            piso->Draw(*mLightsShader);

            // Dibujar ESTANTE 1
            model = glm::mat4(1.0f);
            model = glm::translate(model, estante1Pos);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            mLightsShader->setMat4("model", model);
            estante1->Draw(*mLightsShader);

            // Dibujar ESTANTE 2
            model = glm::mat4(1.0f);
            model = glm::translate(model, estante2Pos);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            mLightsShader->setMat4("model", model);
            estante2->Draw(*mLightsShader);

            // --- ¡NUEVO! DIBUJAR OBJETOS ESTÁTICOS ---
            model = glm::mat4(1.0f); model = glm::translate(model, estante3Pos); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); estante3->Draw(*mLightsShader);
            model = glm::mat4(1.0f); model = glm::translate(model, estante4Pos); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); estante4->Draw(*mLightsShader);
            model = glm::mat4(1.0f); model = glm::translate(model, estante5Pos); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); estante5->Draw(*mLightsShader);
            model = glm::mat4(1.0f); model = glm::translate(model, estante6Pos); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); estante6->Draw(*mLightsShader);
            model = glm::mat4(1.0f); model = glm::translate(model, estante7Pos); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); estante7->Draw(*mLightsShader);

            model = glm::mat4(1.0f); model = glm::translate(model, paredes_estantesPos); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); paredes_estantes->Draw(*mLightsShader);
            model = glm::mat4(1.0f); model = glm::translate(model, caracolPos); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); caracol->Draw(*mLightsShader);
            model = glm::mat4(1.0f); model = glm::translate(model, cuadroPos); model = glm::scale(model, glm::vec3(3.0f, 3.5f, 3.0f)); model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); mLightsShader->setMat4("model", model); cuadro->Draw(*mLightsShader);

           

            // 2. Dibujar TODOS los objetos interactivos (OPacos)
            for (auto& obj : g_interactiveObjects) {
                // Dibuja el objeto principal
                model = glm::mat4(1.0f);
                model = glm::translate(model, obj.position);
                model = glm::rotate(model, obj.inspectRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, obj.inspectRotationX, glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                mLightsShader->setMat4("model", model);
                obj.model->Draw(*mLightsShader);

                // --- ¡NUEVO! Dibuja el letrero asociado ---
                if (obj.infoStandModel != nullptr) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, obj.position + obj.infoStandPos); // Posición absoluta
                    // (Añade rotación para el letrero si es necesario)
                    // model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); 
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    mLightsShader->setMat4("model", model);
                    obj.infoStandModel->Draw(*mLightsShader);
                }
                // --- FIN DE NUEVO ---
            }
        }
    }

    //Dibujo de cajas para debug
    if (showCollisionBoxes) {
        if (!debugShader || debugShader->ID == 0) {
            std::cout << "Shader debug no disponible" << std::endl;
            showCollisionBoxes = false;
        }
        else {
            for (const auto& obj : g_collidableObjects) {
                AABB worldAABB = CalculateWorldAABB(obj);
                DrawBoundingBox(worldAABB, glm::vec3(1.0f, 0.0f, 0.0f), projection, view);
            }
            for (auto& door : g_doors) {
                AABB worldDoorBox;
                worldDoorBox.min = door.currentPosition + door.boundingBox.min;
                worldDoorBox.max = door.currentPosition + door.boundingBox.max;
                DrawBoundingBox(worldDoorBox, glm::vec3(1.0f, 0.0f, 1.0f), projection, view);
            }
            AABB charAABB = GetCharacterBoundingBox();
            DrawBoundingBox(charAABB, glm::vec3(0.0f, 1.0f, 0.0f), projection, view);
            for (const auto& obj : g_interactiveObjects) {
                AABB triggerBox;
                triggerBox.min = obj.position - glm::vec3(obj.triggerRadius, 0.1f, obj.triggerRadius);
                triggerBox.max = obj.position + glm::vec3(obj.triggerRadius, 2.0f, obj.triggerRadius);
                DrawBoundingBox(triggerBox, glm::vec3(0.0f, 0.0f, 1.0f), projection, view);
            }
            for (const auto& door : g_doors) {
                AABB triggerBox;
                triggerBox.min = door.initialPosition - glm::vec3(door.triggerRadius, 0.1f, door.triggerRadius);
                triggerBox.max = door.initialPosition + glm::vec3(door.triggerRadius, 2.0f, door.triggerRadius);
                DrawBoundingBox(triggerBox, glm::vec3(0.0f, 1.0f, 1.0f), projection, view);
            }
            DrawLightDebug(projection, view);
        }
    }

    // Objeto animado
    if (activeCamera != 2)
    {
        character01->UpdateAnimation(deltaTime);
        dynamicShader->use();
        dynamicShader->setMat4("projection", projection);
        dynamicShader->setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, character_position);
        model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.0095f, 0.0095f, 0.0095f));
        dynamicShader->setMat4("model", model);
        dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, character01->gBones);
        character01->Draw(*dynamicShader);
    }

    // --- ¡NUEVO! DIBUJAR OBJETOS TRANSPARENTES (VIDRIO) ---
    // (Debe ir DESPUÉS del personaje y ANTES del texto)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glassShader->use();
        glassShader->setMat4("projection", projection);
        glassShader->setMat4("view", view);

        // Configurar posición del ojo para el shader de vidrio
        if (g_interactingObject != nullptr) {
            // --- ¡MODIFICADO! Usa la cámara correcta ---
            glm::vec3 camPos;
            if (g_isInspectingInfoStand) {
                glm::vec3 standPos = g_interactingObject->position + g_interactingObject->infoStandPos;
                camPos = standPos + g_interactingObject->infoInspectCamPos;
            }
            else {
                camPos = g_interactingObject->position + g_interactingObject->mainInspectCamPos;
            }
            glassShader->setVec3("eye", camPos);
            // --- FIN DE MODIFICACIÓN ---
        }
        else if (activeCamera == 0) {
            glassShader->setVec3("eye", camera_float.Position);
        }
        else if (activeCamera == 1) {
            glassShader->setVec3("eye", camera3rd.Position);
        }
        else if (activeCamera == 2) {
            glassShader->setVec3("eye", camera1st.Position);
        }

        // ¡IMPORTANTE! Bindear la textura del Cubemap
        glActiveTexture(GL_TEXTURE0);
        glassShader->setInt("skybox", 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainCubeMap->textureID);

        glassShader->setFloat("transparency", 0.4f); // <-- Ajusta la transparencia aquí

        // Dibujar las puertas
        for (auto& door : g_doors) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, door.currentPosition); // <-- Usa la posición actual
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            glassShader->setMat4("model", model);
            door.model->Draw(*glassShader);
        }
    }
    // --- FIN DEL BLOQUE DE VIDRIO ---


    glUseProgram(0);

    // --- PASO 3: DIBUJAR LA INTERFAZ 2D (TEXTO) ---
    // (Debe ir AL FINAL de todo el dibujado, antes de SwapBuffers)
    {
        // --- ¡LA SOLUCIÓN ESTÁ AQUÍ! ---
        glDisable(GL_DEPTH_TEST); // Deshabilitar prueba de profundidad
        glEnable(GL_BLEND);       // Habilitar blending para el texto
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // --- FIN DE LA SOLUCIÓN ---

        if (showHelp) {
            std::string helpText =
                "F1 - CAMARA FLOTANTE\n"
                "F2 - CAMARA 3ERA PERSONA\n"
                "F3 - CAMARA 1ERA PERSONA\n"
                "C - MODO DEBUG\n"
                "SHIFT - CORRER\n"
                "FLECHAS - DESPLAZARTE (PERSONAJE)\n"
                "WASD - DESPLAZARTE (CAMARA FLOTANTE)\n"
                "H - MOSTRAR/OCULTAR AYUDA";

            textRenderer.RenderText(helpText,
                (float)SCR_WIDTH * 0.01f,
                (float)SCR_HEIGHT * 0.9f, // Arriba
                0.4f,
                glm::vec3(1.0f, 0.9f, 0.1f));
        }

        if (g_interactingObject != nullptr) {
            // --- ¡MODIFICADO! Lógica de texto de inspección ---
            if (g_isInspectingInfoStand) {
                textRenderer.RenderText("Presiona G para volver al objeto.", (float)SCR_WIDTH * 0.30f, (float)SCR_HEIGHT * 0.2f, 0.4f, glm::vec3(1.0f, 0.9f, 0.1f));
                textRenderer.RenderText("Presiona F para salir.", (float)SCR_WIDTH * 0.30f, (float)SCR_HEIGHT * 0.15f, 0.4f, glm::vec3(1.0f, 0.9f, 0.1f));
            }
            else {
                // --- ¡¡ESTE ES EL TEXTO QUE PEDISTE CAMBIAR!! ---
                textRenderer.RenderText("Presiona G para leer descripcion de objeto.", (float)SCR_WIDTH * 0.30f, (float)SCR_HEIGHT * 0.2f, 0.4f, glm::vec3(1.0f, 0.9f, 0.1f));
                textRenderer.RenderText("Presiona F para salir.", (float)SCR_WIDTH * 0.30f, (float)SCR_HEIGHT * 0.15f, 0.4f, glm::vec3(1.0f, 0.9f, 0.1f));
                textRenderer.RenderText("Presiona Y para rotar.", (float)SCR_WIDTH * 0.30f, (float)SCR_HEIGHT * 0.1f, 0.4f, glm::vec3(1.0f, 0.9f, 0.1f));
            }
        }
        else if (g_nearbyObject != nullptr) {
            textRenderer.RenderText("Presiona F para inspeccionar", (float)SCR_WIDTH * 0.40f, (float)SCR_HEIGHT * 0.2f, 0.4f, glm::vec3(1.0f, 0.9f, 0.1f));
        }
        else if (g_isNearDoors) {
            textRenderer.RenderText("Presiona E para usar", (float)SCR_WIDTH * 0.43f, (float)SCR_HEIGHT * 0.2f, 0.4f, glm::vec3(1.0f, 0.9f, 0.1f));
        }

        // --- ¡IMPORTANTE! Restaurar estado ---
        glEnable(GL_DEPTH_TEST); // Volver a habilitar la profundidad para el próximo frame
        glDisable(GL_BLEND); // Deshabilitar blend
        // --- FIN DE RESTAURAR ---
    }
    // --- FIN DE LÓGICA DE TEXTO ---

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL Error: " << err << std::endl;
    }

    // glfw: swap buffers 
    glfwSwapBuffers(window);
    glfwPollEvents();

    // processInput(window); // <-- Eliminado de aquí, ya se llama al inicio de Update()


    return true;
}


bool CheckCollision(const AABB& a, const AABB& b) {
    bool collisionX = (a.min.x <= b.max.x && a.max.x >= b.min.x);
    bool collisionY = (a.min.y <= b.max.y && a.max.y >= b.min.y);
    bool collisionZ = (a.min.z <= b.max.z && a.max.z >= b.min.z);
    bool collision = collisionX && collisionY && collisionZ;

    if (collision) {
        // Comentado para reducir spam
        // std::cout << "=== COLISIÓN DETECTADA ===" << std::endl;
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

    // Comprobar colisiones con objetos estáticos
    for (int i = 0; i < g_collidableObjects.size(); i++) {
        AABB objAABB = CalculateWorldAABB(g_collidableObjects[i]);
        if (CheckCollision(charAABB, objAABB)) {
            return true;
        }
    }

    // --- ¡NUEVO! Comprobar colisiones con puertas dinámicas ---
    for (auto& door : g_doors) {
        AABB worldDoorBox;
        worldDoorBox.min = door.currentPosition + door.boundingBox.min;
        worldDoorBox.max = door.currentPosition + door.boundingBox.max;
        if (CheckCollision(charAABB, worldDoorBox)) {
            return true;
        }
    }
    // --- FIN DE NUEVO ---

    return false;
}
bool CheckCollisionAtPosition(const glm::vec3& position) {
    glm::vec3 oldPosition = character_position;

    character_position = position;
    bool collision = CheckCharacterCollision();

    character_position = oldPosition;

    return collision;
}

static void UpdateInteractionsWithCollision() {
    // Esta función ya no se usa, la lógica está en Update()
}

void InitializeCollidableObjects() {

    //Pared trasera 
    g_collidableObjects.push_back({ museo, glm::vec3(0.0f, 0.0f, -117.0f),
         AABB(glm::vec3(-70.0f, 0.0f, -0.2f),
               glm::vec3(50.0f, 20.0f, 0.2f)),
         glm::vec3(1.0f), 0.0f
        });

    //Pared izquierda_01
    g_collidableObjects.push_back({ museo, glm::vec3(-32.0f, 0.0f, 0.0f),
         AABB(glm::vec3(-0.5f, 0.0f, -70.0f),
               glm::vec3(0.5f, 20.0f, 33.0f)),
         glm::vec3(1.0f), 0.0f
        });
    //Pared izquierda_02
    g_collidableObjects.push_back({ museo, glm::vec3(-66.0f, 0.0f, 0.0f),
         AABB(glm::vec3(-0.5f, 0.0f, -120.0f),
               glm::vec3(0.5f, 20.0f, -70.0f)),
         glm::vec3(1.0f), 0.0f
        });
    //Pared izquierda_03
    g_collidableObjects.push_back({ museo, glm::vec3(0.0f, 0.0f, -68.0f),
        AABB(glm::vec3(-66.0f, 0.0f, -0.5f),
             glm::vec3(-30.0f, 20.0f, 0.5f)),
        glm::vec3(1.0f), 0.0f
        });
    //Pared derecha_01
    g_collidableObjects.push_back({ museo, glm::vec3(32.0f, 0.0f, 0.0f),
         AABB(glm::vec3(-0.5f, 0.0f, -70.0f),
               glm::vec3(0.5f, 20.0f, 33.0f)),
         glm::vec3(1.0f), 0.0f
        });
    //Pared derecha_02
    g_collidableObjects.push_back({ museo, glm::vec3(66.0f, 0.0f, 0.0f),
         AABB(glm::vec3(-0.5f, 0.0f, -115.0f),
               glm::vec3(0.5f, 20.0f, -70.0f)),
         glm::vec3(1.0f), 0.0f
        });
    //Pared derecha_03
    g_collidableObjects.push_back({ museo, glm::vec3(0.0f, 0.0f, -68.0f),
        AABB(glm::vec3(29.0f, 0.0f, -0.5f),
             glm::vec3(66.0f, 20.0f, 0.5f)),
        glm::vec3(1.0f), 0.0f
        });
    //Pared frontal_izq
    g_collidableObjects.push_back({ museo, glm::vec3(0.0f, 0.0f, 32.0f),
         AABB(glm::vec3(-32.0f, 0.0f, -0.5f),
               glm::vec3(-10.0f, 20.0f, 0.5f)),
         glm::vec3(1.0f), 0.0f
        });
    //Pared frontal_der
    g_collidableObjects.push_back({ museo, glm::vec3(0.0f, 0.0f, 32.0f),
         AABB(glm::vec3(10.0f, 0.0f, -0.5f),
               glm::vec3(32.0f, 20.0f, 0.5f)),
         glm::vec3(1.0f), 0.0f
        });

    // --- AÑADIR ESTANTES (PERO NO EL PISO) ---
    g_collidableObjects.push_back({ estante1, estante1Pos,
                                     AABB(glm::vec3(-2.0f, 0.0f, -7.0f), // Caja de ejemplo
                                          glm::vec3(2.0f, 7.0f, 7.0f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ estante2, estante2Pos,
                                     AABB(glm::vec3(-2.0f, 0.0f, -5.0f),
                                          glm::vec3(2.0f, 7.0f, 5.0f)),
                                     glm::vec3(1.0f), 0.0f });

    // --- ¡¡AQUÍ EMPIEZA LO QUE FALTABA!! ---
    // (¡Ajusta las posiciones y las cajas!)
    g_collidableObjects.push_back({ estante3, estante3Pos,
                                     AABB(glm::vec3(-4.0f, 0.0f, -9.0f),
                                          glm::vec3(4.0f, 7.0f, 9.0f)),
                                     glm::vec3(1.0f), 0.0f });
    g_collidableObjects.push_back({ estante4, estante4Pos,
                                     AABB(glm::vec3(-4.0f, 0.0f, -10.0f),
                                          glm::vec3(4.0f, 7.0f, 10.0f)),
                                     glm::vec3(1.0f), 0.0f });
    g_collidableObjects.push_back({ estante5, estante5Pos,
                                     AABB(glm::vec3(-2.0f, 0.0f, -5.0f),
                                          glm::vec3(2.0f, 9.0f, 4.0f)),
                                     glm::vec3(1.0f), 0.0f });
    g_collidableObjects.push_back({ estante6, estante6Pos,
                                     AABB(glm::vec3(-13.0f, 0.0f, -4.0f),
                                          glm::vec3(7.0f, 10.0f, 4.0f)),
                                     glm::vec3(1.0f), 0.0f });
    g_collidableObjects.push_back({ estante7, estante7Pos,
                                     AABB(glm::vec3(-3.0f, 0.0f, -4.0f),
                                          glm::vec3(3.0f, 9.0f, 4.0f)),
                                     glm::vec3(1.0f), 0.0f });
    g_collidableObjects.push_back({ caracol, caracolPos,
                                     AABB(glm::vec3(-1.0f, 0.0f, -1.0f), // Caja de ejemplo
                                          glm::vec3(1.0f, 5.0f, 1.0f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ cuadro, cuadroPos,
                                     AABB(glm::vec3(-2.0f, 0.0f, -3.0f), // Caja de ejemplo
                                          glm::vec3(2.0f, 8.0f, 3.0f)),
                                     glm::vec3(1.0f), 0.0f });

   


    // --- OBJETOS INTERACTIVOS ---
    g_collidableObjects.push_back({ Xiucoatl, estatuaPos,
                                     AABB(glm::vec3(-4.5f, 0.0f, -4.5f),
                                          glm::vec3(4.5f, 8.0f, 4.5f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ piramide, piramidePos,
                                     AABB(glm::vec3(-4.0f, 0.0f, -4.0f),
                                          glm::vec3(4.0f, 4.0f, 4.0f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ PiedraDelSol, PiedraSolPos,
                                     AABB(glm::vec3(-13.0f, 0.0f, -2.5f),
                                          glm::vec3(13.0f, 10.0f, 17.0f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Coatlicue, CoatlicuePos,
                                     AABB(glm::vec3(-3.5f, 0.0f, -3.5f),
                                          glm::vec3(2.8f, 15.0f, 3.5f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ PlatoAntiguo, PlatoAntiguoPos,
                                     AABB(glm::vec3(-1.8f, 0.0f, -2.0f),
                                          glm::vec3(1.8f, 5.0f, 2.0f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Craneo, CraneoPos,
                                     AABB(glm::vec3(-1.0f, 0.0f, -1.0f),
                                          glm::vec3(1.0f, 5.0f, 1.0f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Incenciario, IncenciarioPos,
                                     AABB(glm::vec3(-3.0f, 0.0f, -3.0f),
                                          glm::vec3(3.0f, 8.0f, 3.3f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Xochipilli, XochipilliPos,
                                     AABB(glm::vec3(-2.0f, 0.0f, -1.5f),
                                          glm::vec3(1.5f, 8.0f, 2.5f)),
                                     glm::vec3(1.0f), 0.0f });

    g_collidableObjects.push_back({ Bracero, BraceroPos,
                                     AABB(glm::vec3(-2.0f, 0.0f, -2.0f),
                                          glm::vec3(2.0f, 8.0f, 2.0f)),
                                     glm::vec3(1.0f), 0.0f });

}


// --- ¡¡FUNCIÓN 'processInput' CORREGIDA!! ---
void processInput(GLFWwindow* window)
{
    static bool f1Pressed = false, f2Pressed = false, f3Pressed = false;
    static bool c_keyPressed = false; // Tecla para 'C'
    static bool h_keyPressed = false; // Tecla para 'H'

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- Controles de cámara flotante (F1) ---
    if (activeCamera == 0) { // Solo controla la cámara flotante si está activa
        if (character_run == true) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera_float.ProcessKeyboard(FORWARD, deltaTime * 8.0f);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera_float.ProcessKeyboard(BACKWARD, deltaTime * 8.0f);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camera_float.ProcessKeyboard(LEFT, deltaTime * 8.0f);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camera_float.ProcessKeyboard(RIGHT, deltaTime * 8.0f);
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                camera_float.ProcessKeyboard(UP, deltaTime * 8.0f);
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // La 'E' solo mueve la cámara flotante
                camera_float.ProcessKeyboard(DOWN, deltaTime * 8.0f);
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
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                camera_float.ProcessKeyboard(UP, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // La 'E' solo mueve la cámara flotante
                camera_float.ProcessKeyboard(DOWN, deltaTime);
        }
    }
    // --- Fin controles cámara flotante ---


    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!c_keyPressed) {
            showCollisionBoxes = !showCollisionBoxes;
            if (showCollisionBoxes && (!debugShader || debugShader->ID == 0)) {
                std::cout << "No se puede activar debug - shader no disponible" << std::endl;
            }
            else {
                std::cout << "Debug colisiones: " << (showCollisionBoxes ? "ACTIVADO" : "DESACTIVADO") << std::endl;
            }
            c_keyPressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        c_keyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        if (!h_keyPressed) {
            showHelp = !showHelp; // Alterna la ayuda
            h_keyPressed = true;
            std::cout << "Ayuda: " << (showHelp ? "ACTIVADA" : "DESACTIVADA") << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE) {
        h_keyPressed = false;
    }


    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        if (!character_run) {
            character_run = true;
            scaleV = runSpeed;
            // std::cout << "Modo CARRERA activado - Velocidad: " << scaleV << std::endl; // Reducir spam
        }
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        if (character_run) {
            character_run = false;
            scaleV = walkSpeed;
            // std::cout << "Modo CAMINATA activado - Velocidad: " << scaleV << std::endl; // Reducir spam
        }
    }

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!g_f_keyPressed) {

            // CASO 1: Empezar a interactuar
            if (g_interactingObject == nullptr && g_nearbyObject != nullptr) {
                g_interactingObject = g_nearbyObject;
                g_nearbyObject = nullptr;
                std::cout << "                                                      \r";
                std::cout << "Interactuando con " << g_interactingObject->name << ". Presiona F para salir. Presiona G para info.\n";

                g_inspectZoom = 45.0f;
                g_isInspectingInfoStand = false; // <-- ¡NUEVO! Resetear estado
            }
            // CASO 2: Dejar de interactuar
            else if (g_interactingObject != nullptr) {

                // Reseteamos el estado del objeto a su original al salir
                g_interactingObject->inspectRotationY = 0.0f;
                g_interactingObject->inspectRotationX = 0.0f;
                g_interactingObject->isAutoRotatingY = false; // Detenemos la rotación

                g_interactingObject = nullptr;
                g_isInspectingInfoStand = false; // <-- ¡NUEVO! Resetear estado
            }
        }
        g_f_keyPressed = true; // Marcamos que la tecla está presionada
    }

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        g_f_keyPressed = false; // Reseteamos la bandera cuando se suelta
    }

    // --- ¡MODIFICADO! Lógica para tecla 'E' (Usar Puerta) ---
    // (Solo si no estamos en modo flotante)
    if (activeCamera != 0 && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (!g_e_keyPressed && g_isNearDoors) { // <-- Comprueba la bandera
            g_e_keyPressed = true;

            // Determina el estado objetivo (si una está cerrada, ábrelas todas, si no, ciérralas todas)
            DoorState targetState = OPENING;
            // Comprueba el estado de la PRIMERA puerta para decidir qué hacer
            if (g_doors.size() > 0 && (g_doors[0].state == OPEN || g_doors[0].state == OPENING)) {
                targetState = CLOSING;
            }

            // Aplica el estado a TODAS las puertas
            for (auto& door : g_doors) {
                if (targetState == OPENING && door.state == CLOSED) {
                    door.state = OPENING;
                }
                else if (targetState == CLOSING && door.state == OPEN) {
                    door.state = CLOSING;
                }
            }

            // Toca el sonido una vez
            if (targetState == OPENING) {
                // SoundEngine->play2D("sound/door_open.wav"); 
            }
            else {
                // SoundEngine->play2D("sound/door_close.wav");
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
        g_e_keyPressed = false;
    }
    // --- FIN DE MODIFICADO ---

    // --- ¡NUEVO! Lógica para tecla 'G' (Cambiar Foco) ---
    if (g_interactingObject != nullptr && glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!g_g_keyPressed) {
            g_isInspectingInfoStand = !g_isInspectingInfoStand; // Alterna
            g_g_keyPressed = true;

            // Resetea el zoom al cambiar de vista
            g_inspectZoom = 45.0f;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) {
        g_g_keyPressed = false;
    }
    // --- FIN DE NUEVO ---


    if (g_interactingObject != nullptr && glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        if (!g_y_keyPressed) {
            g_interactingObject->isAutoRotatingY = !g_interactingObject->isAutoRotatingY;
            std::cout << "                                                      \r";
            if (g_interactingObject->isAutoRotatingY) {
                std::cout << "Rotacion activada. Presiona Y para detener. Presiona F para salir.\n";
            }
            else {
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
                // camera3rd.ProcessKeyboard(FORWARD, deltaTime); // <-- ¡ELIMINADA!
                character_position = newPosition;
                UpdateCameras();
            }
            else {
                // std::cout << "COLISIÓN - Movimiento bloqueado" << std::endl; // Reducir spam
                character_position = oldPosition;
                UpdateCameras();
            }

        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {

            glm::vec3 newPosition = character_position - scaleV * forwardView;
            glm::vec3 oldPosition = character_position;

            if (!CheckCollisionAtPosition(newPosition)) {
                // camera3rd.ProcessKeyboard(FORWARD, deltaTime); // <-- ¡ELIMINADA!
                character_position = newPosition;
                UpdateCameras();
            }
            else {
                // std::cout << "COLISIÓN - Movimiento bloqueado" << std::endl; // Reducir spam
                character_position = oldPosition;
                UpdateCameras();
            }
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            rotateCharacter += 0.8f;
            if (character_run) {
                rotateCharacter += 2.0f;
            }
            UpdateCameras();
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            rotateCharacter -= 0.8f;
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
// --- FIN DE LA FUNCIÓN CORREGIDA ---


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
    }
    // SI ESTAMOS EN MODO NORMAL
    else
    {
        // Solo controla la cámara flotante
        if (activeCamera == 0) {
            camera_float.ProcessMouseMovement(xoffset, yoffset);
        }
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
        // Solo controla la cámara flotante
        if (activeCamera == 0) {
            camera_float.ProcessMouseScroll((float)yoffset);
        }
    }
    // --- FIN DE LÓGICA MODIFICADA ---
}
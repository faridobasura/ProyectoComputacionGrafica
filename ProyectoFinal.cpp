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
using namespace irrklang;

// --- ESTRUCTURA DE OBJETO INTERACTIVO (MODIFICADA) ---
struct InteractiveObject {
	Model* model;            // Puntero al modelo cargado
	glm::vec3   position;         // Posición en el mundo
	float       triggerRadius;    // Radio de cercanía
	std::string name;             // Nombre para depuración

	// Variables de estado para la inspección
	float       inspectRotationY; // Rotación en Y (ahora automática)
	float       inspectRotationX; // Rotación en X (ya no se usa, pero se queda para reset)
	bool        isAutoRotatingY;  // Bandera para rotación automática

	// Constructor para facilitar la creación
	InteractiveObject(Model* m, glm::vec3 pos, float radius, std::string n) :
		model(m), position(pos), triggerRadius(radius), name(n),
		inspectRotationY(0.0f), inspectRotationX(0.0f), isAutoRotatingY(false) {
	} // Inicia en 0
};
// --- FIN DE ESTRUCTURA ---

// Functions
bool Start();
bool Update();

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

// Definición de cámara (posición en XYZ)
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
Camera camera3rd(glm::vec3(0.0f, 0.0f, 0.0f));

// Controladores para el movimiento del mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Variables para la velocidad de reproducción
// de la animación
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float elapsedTime = 0.0f;

// Variables del personaje
glm::vec3 position(0.0f, 0.0f, 0.0f);
glm::vec3 forwardView(0.0f, 0.0f, 1.0f);
float     trdpersonOffset = 1.5f;
float     scaleV = 0.025f;
float     rotateCharacter = 0.0f;

// Shaders
Shader* mLightsShader;
Shader* proceduralShader;
Shader* wavesShader;

Shader* cubemapShader;
Shader* dynamicShader;

// Carga la información del modelo
Model* museo; // Entorno

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
bool activeCamera = 1;


// --- VARIABLES GLOBALES DE INTERACCIÓN ---
std::vector<InteractiveObject> g_interactiveObjects; // Lista de exhibiciones
InteractiveObject* g_nearbyObject = nullptr;      // Exhibición más cercana (para "Presiona F")
InteractiveObject* g_interactingObject = nullptr; // Exhibición con la que estamos interactuando
bool g_f_keyPressed = false; // Para detectar una sola pulsación de 'F'
bool g_y_keyPressed = false; // Para detectar una sola pulsación de 'Y'

// --- VARIABLES CÁMARA DE INSPECCIÓN ---
float g_inspectRadius = 3.0f; // Distancia (zoom) de la cámara FIJA
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

	// Máximo número de huesos: 100
	dynamicShader->setBonesIDs(MAX_RIGGING_BONES);

	// --- Carga de modelos modulares ---
	museo = new Model("models/IllumModels/proyectofinal/Entorno.fbx");
	Model* Xiucoatl = new Model("models/IllumModels/estatua.fbx");
	Model* piramide = new Model("models/IllumModels/proyectofinal/piramides.fbx");
	Model* PiedraDelSol = new Model("models/IllumModels/proyectofinal/PiedraDelSol.fbx");
	Model* Coatlicue = new Model("models/IllumModels/proyectofinal/Coatlicue.fbx");
	Model* PlatoAntiguo = new Model("models/IllumModels/proyectofinal/PlatoAntiguo.fbx");
	Model* Craneo = new Model("models/IllumModels/proyectofinal/Craneo.fbx");
	Model* Incenciario = new Model("models/IllumModels/proyectofinal/Incenciario.fbx");
	Model* Xochipilli = new Model("models/IllumModels/proyectofinal/Xochipilli.fbx");
	Model* Bracero = new Model("models/IllumModels/proyectofinal/Bracero.fbx");
	character01 = new AnimatedModel("models/character.fbx");

	// Definimos las posiciones de los objetos (las que tenías en Update())
	glm::vec3 estatuaPos = glm::vec3(0.0f, 0.0f, 0.0f); // Origen
	glm::vec3 piramidePos = glm::vec3(0.0f, 0.0f, -25.0f);
	glm::vec3 PiedraSolPos = glm::vec3(0.0f, 0.0f, -77.6f);
	glm::vec3 CoatlicuePos = glm::vec3(-48.47f, 0.0f, -97.635f);
	glm::vec3 PlatoAntiguoPos = glm::vec3(-42.28f, 0.0f, -73.26f);
	glm::vec3 CraneoPos = glm::vec3(-25.69f, 0.22f, -118.21f);
	glm::vec3 IncenciarioPos = glm::vec3(36.63f, 0.0f, -112.64f);
	glm::vec3 XochipilliPos = glm::vec3(51.11f, 0.06f, -93.76f);
	glm::vec3 BraceroPos = glm::vec3(37.94f, 0.10f, -70.74f);

	// Creamos y añadimos los objetos interactivos a la lista global
	g_interactiveObjects.emplace_back(Xiucoatl, estatuaPos, 6.0f, "Estatua"); // Radio de 3 unidades
	g_interactiveObjects.emplace_back(piramide, piramidePos, 4.0f, "Piramides"); // Radio de 4
	g_interactiveObjects.emplace_back(PiedraDelSol, PiedraSolPos, 3.0f, "PiedraSol"); // Radio de 3
	g_interactiveObjects.emplace_back(Coatlicue, CoatlicuePos, 3.0f, "Coatlicue"); // Radio de 3
	g_interactiveObjects.emplace_back(PlatoAntiguo, PlatoAntiguoPos, 3.0f, "PlatoAntiguo"); // Radio de 3
	g_interactiveObjects.emplace_back(Craneo, CraneoPos, 3.0f, "Craneo");
	g_interactiveObjects.emplace_back(Incenciario, IncenciarioPos, 3.0f, "Incenciario");
	g_interactiveObjects.emplace_back(Xochipilli, XochipilliPos, 3.0f, "Xochipilli");
	g_interactiveObjects.emplace_back(Bracero, BraceroPos, 3.0f, "Bracero");
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

	camera3rd.Position = position;
	camera3rd.Position.y += 1.7f;
	camera3rd.Position -= trdpersonOffset * forwardView;
	camera3rd.Front = forwardView;

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
			float distance = glm::distance(position, obj.position); // Distancia del personaje al objeto
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
		// --- ¡CAMBIO AQUÍ! ---
		// 1. Definimos el centro del objetivo A 1.0 unidad POR ENCIMA del pivote del objeto
		// Esto asume que el pivote está en la base. Si tu modelo tiene el pivote más alto, ajústalo.
		glm::vec3 targetCenter = g_interactingObject->position + glm::vec3(0.0f, 5.0f, 9.0f);

		// 2. Posicionamos la cámara FIJA, a la misma altura que el targetCenter, y 'g_inspectRadius' hacia atrás
		// También elevamos la cámara un poco más si es necesario para tener una buena vista.
		glm::vec3 camPos = targetCenter + glm::vec3(0.0f, 0.5f, g_inspectRadius); // 0.5f para elevar la cámara un poco más

		// 3. La cámara mira al 'targetCenter'
		view = glm::lookAt(camPos, targetCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		// --- FIN DE CAMBIO ---

		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
	}
	// SI ESTAMOS EN MODO NORMAL (explorando)
	else
	{
		if (activeCamera) {
			// Cámara en primera persona
			projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
			view = camera.GetViewMatrix();
		}
		else {
			// cámara en tercera persona
			projection = glm::perspective(glm::radians(camera3rd.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
			view = camera3rd.GetViewMatrix();
		}
	}
	// --- FIN DE CÁLCULO DE CÁMARA ---

	// Cubemap (fondo)
	{
		mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
	}

	{
		mLightsShader->use();

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

		mLightsShader->setVec3("eye", camera.Position);

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

			// --- ¡CAMBIO AQUÍ! ---
			// Si estamos inspeccionando ESTE objeto, lo elevamos para que se vea bien
			if (&obj == g_interactingObject) {
				// Puedes ajustar 1.0f según el tamaño promedio de tus objetos
				// Para la estatua o pirámide, 1.0f-1.5f debería funcionar bien
				model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
			}
			// --- FIN DE CAMBIO ---

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


	// Actividad 5.2
	/*
	// ... (Código de proceduralShader sin cambios)
	*/

	// Actividad 5.3
	/*
	// ... (Código de wavesShader sin cambios)
	*/

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
		model = glm::translate(model, position); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));	// it's a bit too big for our scene, so scale it down

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

// Procesamos entradas del teclado
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// --- Controles de cámara y debug ---
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	// --- Fin controles ---


	// --- LÓGICA DE TECLA F (para pulsación única) ---
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		// Solo si NO la teníamos presionada desde el frame anterior
		if (!g_f_keyPressed) {

			// CASO 1: Empezar a interactuar
			if (g_interactingObject == nullptr && g_nearbyObject != nullptr) {
				g_interactingObject = g_nearbyObject;
				g_nearbyObject = nullptr;
				std::cout << "                                                      \r";
				std::cout << "Interactuando con " << g_interactingObject->name << ". Presiona F para salir. Presiona Y para rotar/detener.\n";

				// Reseteamos el zoom
				g_inspectRadius = 3.0f;

				// NO reseteamos la rotación aquí, para que pueda continuar donde se quedó
			}
			// CASO 2: Dejar de interactuar
			else if (g_interactingObject != nullptr) {

				// --- ¡CAMBIO AQUÍ! ---
				// Reseteamos el estado del objeto a su original al salir
				g_interactingObject->inspectRotationY = 0.0f;
				g_interactingObject->inspectRotationX = 0.0f;
				g_interactingObject->isAutoRotatingY = false; // Detenemos la rotación
				// --- FIN DE CAMBIO ---

				g_interactingObject = nullptr;
			}
		}
		g_f_keyPressed = true; // Marcamos que la tecla está presionada
	}

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		g_f_keyPressed = false; // Reseteamos la bandera cuando se suelta
	}
	// --- FIN DE LÓGICA F ---

	// --- LÓGICA DE TECLA Y (para auto-rotación) ---
	// Solo funciona si ya estamos en modo inspección
	if (g_interactingObject != nullptr && glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
		if (!g_y_keyPressed) {
			// Cambia (toggle) el estado de auto-rotación
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
	// --- FIN DE LÓGICA Y ---


	// --- CONGELAR MOVIMIENTO MIENTRAS SE INTERACTÚA ---
	// Solo permitimos mover al personaje si NO estamos en modo interacción
	if (g_interactingObject == nullptr) {
		// Character movement
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			position = position + scaleV * forwardView;
			camera3rd.Front = forwardView;
			camera3rd.ProcessKeyboard(FORWARD, deltaTime);
			camera3rd.Position = position;
			camera3rd.Position.y += 1.7f;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			position = position - scaleV * forwardView;
			camera3rd.Front = forwardView;
			camera3rd.ProcessKeyboard(BACKWARD, deltaTime);
			camera3rd.Position = position;
			camera3rd.Position.y += 1.7f;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			rotateCharacter += 0.5f;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
			forwardView = glm::vec3(viewVector);
			forwardView = glm::normalize(forwardView);

			camera3rd.Front = forwardView;
			camera3rd.Position = position;
			camera3rd.Position.y += 1.7f;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			rotateCharacter -= 0.5f;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
			forwardView = glm::vec3(viewVector);
			forwardView = glm::normalize(forwardView);

			camera3rd.Front = forwardView;
			camera3rd.Position = position;
			camera3rd.Position.y += 1.7f;
			camera3rd.Position -= trdpersonOffset * forwardView;
		}
	} // Fin de if (g_interactingObject == nullptr)
	// --- FIN DE CONGELAR MOVIMIENTO ---


	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
		activeCamera = 0;
	if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
		activeCamera = 1;

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
		camera.ProcessMouseMovement(xoffset, yoffset);
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
		// El scroll controla el zoom (distancia de la cámara fija)
		g_inspectRadius -= (float)yoffset;

		// Limitar el zoom
		if (g_inspectRadius < 1.0f)
			g_inspectRadius = 1.0f;
		if (g_inspectRadius > 10.0f)
			g_inspectRadius = 10.0f;
	}
	// SI ESTAMOS EN MODO NORMAL
	else
	{
		camera.ProcessMouseScroll((float)yoffset);
	}
	// --- FIN DE LÓGICA MODIFICADA ---
}
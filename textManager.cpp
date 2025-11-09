#include <textManager.h>
#include <text_render.h>
#include <iostream>
#include <stdexcept>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

TextManager::TextManager(TextRenderer& renderer, unsigned int width, unsigned int height)
    : textRenderer(renderer), scrWidth(width), scrHeight(height) {
}

void TextManager::setScreenSize(unsigned int width, unsigned int height) {
    scrWidth = width;
    scrHeight = height;
}

void TextManager::clearTextArea(float x, float y, float scale, const std::string& text) {
    // Implementación de limpieza: renderiza texto "vacío" (espacios) sobre el área previa
    std::string clearStr(text.length(), ' ');
    textRenderer.RenderText(clearStr, x, y, scale, glm::vec3(0.0f, 0.0f, 0.0f));
}

bool TextManager::isContext() {
    if (!glfwGetCurrentContext()) {
        std::cerr << "Error: No hay un contexto de OpenGL activo antes de inicializar TextRenderer\n";
        return false;
    }
    else {
        try {
            textRenderer.Init(textShaderID, SCR_WIDTH, SCR_HEIGHT, "fonts/cambriab.ttf");
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error al inicializar texto: " << e.what() << std::endl;
            return false;
        }
    }
}

bool TextManager::setHelpText(const std::string& helpText) {
    try {
        textRenderer.RenderText(
            helpText,
            static_cast<float>(SCR_WIDTH) * 0.01f,
            static_cast<float>(SCR_HEIGHT) * 0.95f,  // ajustado para que aparezca en la parte superior
            0.4f,
            glm::vec3(1.0f, 0.9f, 0.1f)
        );
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error en setHelpText: " << e.what() << std::endl;
        return false;
    }
}

bool TextManager::setTextByWorld(const std::string& text) {
    try {
        float x = static_cast<float>(scrWidth) * 0.3f;
        float y = static_cast<float>(scrHeight) * 0.2f;
        float scale = 0.4f;

        if (text != lastText && !lastText.empty()) {
            clearTextArea(x, y, scale, lastText);
        }

        if (!text.empty()) {
            textRenderer.RenderText(text, x, y, scale, glm::vec3(0.9f, 0.7f, 0.05f));
        }

        lastText = text;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error en setTextByWorld: " << e.what() << std::endl;
        return false;
    }
}

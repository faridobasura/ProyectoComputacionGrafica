#pragma once

#include <string>
#include <glm/glm.hpp>
#include <iostream>
#include <text_render.h>

class TextManager {
    public:
        TextManager(TextRenderer& renderer, unsigned int width, unsigned int height);
    
        bool isContext();
        bool setHelpText(const std::string& helpText);
        bool setTextByWorld(const std::string& text);
        void clearTextArea(float x, float y, float scale, const std::string& text);
        void setScreenSize(unsigned int width, unsigned int height);
    
        unsigned int textShaderID;  // <-- Agregar esto
    
    private:
        TextRenderer& textRenderer;
        unsigned int scrWidth;
        unsigned int scrHeight;
        std::string lastText;
};
    
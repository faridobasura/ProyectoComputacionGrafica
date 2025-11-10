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
        //void clearTextArea(float x, float y, float scale, const std::string& text);
        void setScreenSize(unsigned int width, unsigned int height);
        void clearTextArea(float x, float y, float width, float height, const glm::vec3& bgColor);
    
        unsigned int textShaderID;  
    
    private:
        TextRenderer& textRenderer;
        unsigned int scrWidth;
        unsigned int scrHeight;
        std::string lastText;
};
    
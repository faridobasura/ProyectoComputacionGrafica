#pragma once

#include <map>
#include <string>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H


struct Character {
    GLuint TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    GLuint Advance;
};


class TextRenderer {
public:
    std::map<GLchar, Character> Characters;
    GLuint VAO, VBO;
    GLuint shaderID;
    int screenWidth, screenHeight;

    TextRenderer() {}

    // ============================================================
    // INIT
    // ============================================================
    void Init(GLuint shader, int width, int height, const std::string& fontPath) {
        shaderID = shader;
        screenWidth = width;
        screenHeight = height;

        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            std::cerr << "Error al inicializar FreeType\n";
            return;
        }

        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            std::cerr << "Error al cargar la fuente: " << fontPath << "\n";
            FT_Done_FreeType(ft);
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 48);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            Character character = {
                tex,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                (GLuint)face->glyph->advance.x
            };
            Characters.insert(std::pair<GLchar, Character>(c, character));
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        // --- Crear VAO / VBO ---
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }


    // ============================================================
    // 🔥 FUNCIONES NUEVAS - MEDIR TEXTO ANTES DE RENDERIZAR
    // ============================================================

    float MeasureTextWidth(const std::string& text, float scale) {
        float width = 0.0f;

        for (char c : text) {
            if (c == '\n') break; // solo mide primera línea
            Character ch = Characters[c];
            width += (ch.Advance >> 6) * scale;
        }

        return width;
    }

    float MeasureTextHeight(float scale) {
        Character H = Characters['H'];
        return H.Size.y * scale * 1.35f;  // altura real evitando traslapes
    }

    glm::vec2 MeasureTextBlock(const std::string& text, float scale) {
        float maxWidth = 0.0f;
        float totalHeight = 0.0f;

        float currentWidth = 0.0f;
        float lineHeight = MeasureTextHeight(scale);

        for (char c : text) {
            if (c == '\n') {
                totalHeight += lineHeight;
                maxWidth = std::max(maxWidth, currentWidth);
                currentWidth = 0;
                continue;
            }

            Character ch = Characters[c];
            currentWidth += (ch.Advance >> 6) * scale;
        }

        totalHeight += lineHeight;
        maxWidth = std::max(maxWidth, currentWidth);

        return glm::vec2(maxWidth, totalHeight);
    }


    // ============================================================
    // 🔥 Ajustar automáticamente el tamaño de la ventana o layout
    // ============================================================
    void AutoScaleToText(const std::string& text, float scale, int& outW, int& outH) {
        glm::vec2 size = MeasureTextBlock(text, scale);
        outW = (int)size.x + 20;  // padding
        outH = (int)size.y + 20;  // padding
    }


    // ============================================================
    // RENDER DE TEXTO
    // ============================================================
    void RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
        glUseProgram(shaderID);
        glUniform3f(glGetUniformLocation(shaderID, "textColor"), color.x, color.y, color.z);

        glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        GLfloat startX = x;
        GLfloat currentY = y;

        float lineHeight = MeasureTextHeight(scale);

        for (auto c : text) {

            if (c == '\n') {
                x = startX;
                currentY -= lineHeight;
                continue;
            }

            Character ch = Characters[c];

            GLfloat xpos = x + ch.Bearing.x * scale;
            GLfloat ypos = currentY - (ch.Size.y - ch.Bearing.y) * scale;

            GLfloat w = ch.Size.x * scale;
            GLfloat h = ch.Size.y * scale;

            GLfloat vertices[6][4] = {
                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos,     ypos,     0.0f, 1.0f},
                {xpos + w, ypos,     1.0f, 1.0f},

                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos + w, ypos,     1.0f, 1.0f},
                {xpos + w, ypos + h, 1.0f, 0.0f}
            };

            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            x += (ch.Advance >> 6) * scale;
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

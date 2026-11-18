#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

#include <cstddef>
#include <filesystem>
#include <vector>

#include "basic_transform.h"
#include "glad/glad.h"  // must come before any other GL header
#include "texture_atlas.h"
#include "vector_util.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#define GL_SILENCE_DEPRECATION  // silences macOS OpenGL deprecation warnings

#define glCheck(expr)                                                                              \
    expr;                                                                                          \
    if (int e = glGetError()) {                                                                    \
        std::cerr << "OpenGL error: " << e << " in file " __FILE__ << ", line number " << __LINE__ \
                  << '\n';                                                                         \
        abort();                                                                                   \
    }

using SpriteID = size_t;

class SpriteManager {
public:
    SpriteManager(const sf::Vector2u& atlasSize, unsigned padding)
        : mVertices(sf::PrimitiveType::Triangles),
          mNumSprites{0},
          mAtlas(atlasSize, padding),
          mShader(std::filesystem::path{"main.vert"}, std::filesystem::path{"main.frag"}),
          mSfmlShader(std::filesystem::path{"main_sfml.vert"},
                      std::filesystem::path{"main.frag"}) {
    
        mSfmlShader.setUniform("texture", sf::Shader::CurrentTexture);

        const GLuint prog = mShader.getNativeHandle();

        mPositionAttribLocation = glGetAttribLocation(prog, "position");
        mTexCoordsAttribLocation = glGetAttribLocation(prog, "texCoords");
        mSpriteNumberAttribLocation = glGetAttribLocation(prog, "spriteNumber");

        mTextureUniformLocation = glGetUniformLocation(prog, "texture");
        mTransformsUniformLocation = glGetUniformLocation(prog, "transforms");
        mNumTransformsUniformLocation = glGetUniformLocation(prog, "numTransforms");

        glCheck(glGenBuffers(1, &mVerticesVBO));
        glCheck(glGenBuffers(1, &mSpriteNumberVBO));
        glCheck(glGenTextures(1, &mTransformsTexture));
    }


    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;

    ~SpriteManager() {
        glDeleteBuffers(1, &mVerticesVBO);
        glDeleteBuffers(1, &mSpriteNumberVBO);
        glDeleteTextures(1, &mTransformsTexture);
    }

    TextureID registerTexture(const std::string& filepath) {
        return mAtlas.registerTexture(filepath);
    }

    SpriteID createSprite(TextureID textureID) {
        const SpriteID id = mNumSprites++;
        mTransforms.emplace_back();  

        for (int i = 0; i < 6; ++i) {
            mVertices.append(sf::Vertex{});
            mSpriteNumbers.push_back(static_cast<float>(id));
        }
        writeQuad(id, textureID);
        return id;
    }

    void setTexture(SpriteID sprite, TextureID newTexture) { writeQuad(sprite, newTexture); }

  
    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        sf::VertexArray transformed = mVertices; 
        for (size_t s = 0; s < mNumSprites; ++s) {
            for (size_t k = 0; k < 6; ++k) {
                const size_t v = 6 * s + k;
                transformed[v].position = mTransforms[s].applyPoint(mVertices[v].position);
            }
        }
        states.shader = &mSfmlShader;
        states.texture = &mAtlas.getTexture();
        target.draw(transformed, states);
    }

    void drawGL() {
        if (mNumSprites == 0) return;
        const GLsizei vertexCount = static_cast<GLsizei>(mVertices.getVertexCount());

        glCheck(glUseProgram(mShader.getNativeHandle()));

       
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, mVerticesVBO));
        glCheck(glBufferData(GL_ARRAY_BUFFER,
                             static_cast<GLsizeiptr>(vertexCount * sizeof(sf::Vertex)),
                             &mVertices[0], GL_STREAM_DRAW));
        glCheck(glVertexAttribPointer(mPositionAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                      sizeof(sf::Vertex),
                                      reinterpret_cast<const void*>(offsetof(sf::Vertex, position))));
        glCheck(glEnableVertexAttribArray(mPositionAttribLocation));
        glCheck(glVertexAttribPointer(mTexCoordsAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                      sizeof(sf::Vertex),
                                      reinterpret_cast<const void*>(offsetof(sf::Vertex, texCoords))));
        glCheck(glEnableVertexAttribArray(mTexCoordsAttribLocation));


        glCheck(glBindBuffer(GL_ARRAY_BUFFER, mSpriteNumberVBO));
        glCheck(glBufferData(GL_ARRAY_BUFFER,
                             static_cast<GLsizeiptr>(mSpriteNumbers.size() * sizeof(float)),
                             mSpriteNumbers.data(), GL_STREAM_DRAW));
        glCheck(glVertexAttribPointer(mSpriteNumberAttribLocation, 1, GL_FLOAT, GL_FALSE, 0,
                                      reinterpret_cast<const void*>(0)));
        glCheck(glEnableVertexAttribArray(mSpriteNumberAttribLocation));

        glCheck(glActiveTexture(GL_TEXTURE0));
        glCheck(glBindTexture(GL_TEXTURE_2D, mAtlas.getTexture().getNativeHandle()));
        glCheck(glUniform1i(mTextureUniformLocation, 0));

        glCheck(glActiveTexture(GL_TEXTURE1));
        glCheck(glBindTexture(GL_TEXTURE_1D, mTransformsTexture));
        glCheck(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        glCheck(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        glCheck(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        glCheck(glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F_ARB,
                             static_cast<GLsizei>(mTransforms.size()), 0, GL_RGBA, GL_FLOAT,
                             mTransforms.data()));
        glCheck(glUniform1i(mTransformsUniformLocation, 1));
        glCheck(glUniform1i(mNumTransformsUniformLocation, static_cast<GLint>(mTransforms.size())));

        glCheck(glDrawArrays(GL_TRIANGLES, 0, vertexCount));


        glDisableVertexAttribArray(mPositionAttribLocation);
        glDisableVertexAttribArray(mTexCoordsAttribLocation);
        glDisableVertexAttribArray(mSpriteNumberAttribLocation);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glActiveTexture(GL_TEXTURE0);
        glUseProgram(0);
    }

    BasicTransform& getTransform(SpriteID id) { return mTransforms[id]; }

    const TextureAtlas& getAtlas() const { return mAtlas; }

private:
 
    void writeQuad(SpriteID id, TextureID tex) {
        const sf::Vector2f size(mAtlas.getSizePixels(tex));
        const sf::FloatRect uv = mAtlas.getBoundsUV(tex);  
        const sf::Vector2f bl{0.f, 0.f}, br{size.x, 0.f}, tr{size.x, size.y}, tl{0.f, size.y};
        const sf::Vector2f uvBL = uv.position;
        const sf::Vector2f uvBR{uv.position.x + uv.size.x, uv.position.y};
        const sf::Vector2f uvTR = uv.position + uv.size;
        const sf::Vector2f uvTL{uv.position.x, uv.position.y + uv.size.y};

        const sf::Vector2f pos[6] = {bl, br, tr, bl, tr, tl};
        const sf::Vector2f tc[6] = {uvBL, uvBR, uvTR, uvBL, uvTR, uvTL};
        for (size_t k = 0; k < 6; ++k) {
            sf::Vertex& v = mVertices[6 * id + k];
            v.position = pos[k];
            v.texCoords = tc[k];
            v.color = sf::Color::White;
        }
    }

    sf::VertexArray mVertices;
    std::vector<BasicTransform> mTransforms;
    size_t mNumSprites;
    TextureAtlas mAtlas;
    mutable sf::Shader mShader;      
    mutable sf::Shader mSfmlShader;  
 
    std::vector<float> mSpriteNumbers;

    GLint mTextureUniformLocation = -1;
    GLint mTransformsUniformLocation = -1;
    GLint mNumTransformsUniformLocation = -1;

    GLint mPositionAttribLocation = -1;
    GLint mTexCoordsAttribLocation = -1;
    GLint mSpriteNumberAttribLocation = -1;

    GLuint mVerticesVBO = 0;
    GLuint mSpriteNumberVBO = 0;
    GLuint mTransformsTexture = 0;
};

#endif  // SPRITE_MANAGER_H
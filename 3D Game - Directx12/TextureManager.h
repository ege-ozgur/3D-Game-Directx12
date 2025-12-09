#pragma once
#include <map>
#include <string>
#include "Texture.h"
#include "Core.h"

class TextureManager {
public:
    std::map<std::string, Texture*> textureCache;

    ~TextureManager() {
        for (auto const& [key, val] : textureCache) {
            delete val;
        }
    }

    void load(Core* core, std::string filename) {
        if (textureCache.find(filename) != textureCache.end()) {
            return;
        }

        Texture* newTex = new Texture();
        newTex->load(core, filename);

        textureCache[filename] = newTex;
    }


    int find(std::string filename) {
        if (textureCache.find(filename) != textureCache.end()) {
            return textureCache[filename]->heapOffset;
        }
        else {
            return -1;
        }
    }
};

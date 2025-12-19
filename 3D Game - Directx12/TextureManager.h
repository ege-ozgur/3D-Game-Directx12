#pragma once
#include <map>
#include <string>
#include "Texture.h"
#include "Core.h"

using namespace std;

class TextureManager { // my simple texture manager to load and manage textures
public:
    map<string, Texture*> textureCache;

	~TextureManager() { // destructor to release all loaded textures
        for (auto const& [key, val] : textureCache) {
            delete val;
        }
    }

	void load(Core* core, std::string filename) { // load a texture if not already loaded
        if (textureCache.find(filename) != textureCache.end()) {
            return;
        }

        Texture* newTex = new Texture();
        newTex->load(core, filename);

        textureCache[filename] = newTex;
    }


	int find(string filename) { // find a texture by its filename and return its heap offset or -1 if not found
        if (textureCache.find(filename) != textureCache.end()) {
            return textureCache[filename]->heapOffset;
        }
        else {
            return -1;
        }
    }
};

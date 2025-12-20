#include "Window.h"
#include "Core.h"
#include "PSOManager.h"
#include "ShaderManager.h"
#include "maths.h"
#include "Sphere.h"
#include "StaticMesh.h"
#include "AnimatedMesh.h"
#include "Animation.h"
#include "Player.h"
#include "TextureManager.h"
#include "PlayerAnimManager.h"
#include "EnemyManager.h"
#include "BulletManager.h"
#include "GroundPlane.h"
#include "Skybox.h"
#include "Crosshair.h"
#include "Grass.h"
#include <chrono>
#include <vector>
#include <map>
#include <string>

using namespace std;

extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

class Timer { // simple timer class to calculate delta time between frames
public:
    std::chrono::steady_clock::time_point last;
    std::chrono::steady_clock::time_point start;
    Timer() { last = std::chrono::steady_clock::now(); }

    void reset() { last = std::chrono::steady_clock::now(); }

    float dt() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> diff = now - last;
        last = now;
        return diff.count();
    }

    float totalTime() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> diff = now - start;
        return diff.count();
    }
};

struct RenderItem { // structure to hold renderable static mesh items along with their world transform and collider
    StaticMesh* mesh;
    Matrix transform;
    AABB collider;
};

// these are the global variables for the level and game
map<string, StaticMesh*> meshCache;
vector<RenderItem> staticRenderList;
vector<AABB> obstacles; // we store collidable obstacles in the level
EnemyManager enemyMgr;
BulletManager bulletMgr;
Matrix worldPlane;
string currentGroundModel = ""; 

void ClearLevel() { // function to clear the current level data
    staticRenderList.clear();
    obstacles.clear();
    enemyMgr.reset();
    bulletMgr.reset();
}

bool LoadLevel(string filename, Core* core, TextureManager* texMgr, PSOManager* psoMgr, ShaderManager* shaderMgr, GroundPlane* plane) { // function to load a level from a text file
    ifstream file(filename); // open the level file

    if (!file.is_open()) {
        return false;
    }
	ClearLevel(); // if we are loading the second level we need to clear the previous level data first

    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
			if (line.empty() || line[0] == '#') { // skip empty lines and comments
                continue;
            }
            stringstream ss(line);
            string type, path;
            Vec3 pos, rot, sc;

			ss >> type >> path >> pos.x >> pos.y >> pos.z >> rot.x >> rot.y >> rot.z >> sc.x >> sc.y >> sc.z; // this fits the format i defined for level objects

			Matrix S, RX, RY, T; // we create world matrix from scaling, rotation and translation matrices
            S.scaling(sc);
            RX.rotationX(rot.x);
            RY.rotAroundY(rot.y);
            Matrix R = RX * RY;
            T.translation(pos);

            Matrix worldMatrix = S * R * T; // correct multiplication for the math class. The multiplication is taken from Tom's

			if (type == "TREE" || type == "WALL") // these are the static mesh objects
            {
				if (meshCache.find(path) == meshCache.end()) // we load the static mesh only if not already loaded
                {
                    StaticMesh* newMesh = new StaticMesh();
                    newMesh->init(core, path, texMgr);
                    meshCache[path] = newMesh;
                }

                RenderItem item;
                item.mesh = meshCache[path];
                item.transform = worldMatrix;

				// we create a simple AABB collider for the static mesh based on its type and scale
                Vec3 colSize(1.0f, 1.0f, 1.0f);
                Vec3 centerOffset(0, 0, 0);

				if (type == "TREE") { // for trees we use a tall thin collider
                    colSize = Vec3(1.0f, 10.0f, 1.0f);
                    centerOffset = Vec3(0, 5.0f, 0);
                }
				else { // here else means type is WALL
					float baseThickness = 0.5f; // walls are thin. I find this value by testing it multiple times
					float baseHeight = 5.0f; // same for height
					float baseWidth = 3.8f; // and 3.8 was good for width this also found by testing

                    if (sc.y < 5.0f) { // if we use it as a cover we make the bounding box smaller
						baseWidth = 2.7f; // manually calcuated for the wall model by simply testing it multiple times
                    }

					float w = baseThickness * sc.x; // thickness is along x-axis
					float h = baseHeight * sc.y; // height is along y-axis
					float d = baseWidth * sc.z; // width is along z-axis
                     
					if (abs(rot.y) > 1.0f) { // if wall is rotated 90 degrees we swap width and thickness
                        std::swap(w, d);
                    }

                    colSize = Vec3(w, h, d);
                    centerOffset = Vec3(0, h * 0.5f, 0);
                }

                Vec3 halfSize = colSize * 0.5f;
                Vec3 centerPos = pos + centerOffset;
                item.collider.min = centerPos - halfSize;
                item.collider.max = centerPos + halfSize;

                staticRenderList.push_back(item);
                obstacles.push_back(item.collider);
            }
			else if (type == "ENEMY") // enemy type
            {
                enemyMgr.spawnEnemy(pos, sc);
            }
			else if (type == "PLANE") // ground plane type
            {
                worldPlane = worldMatrix;

				if (path != currentGroundModel) { // we only reload the ground model if its different from the current one. currently it is the same but it can easily be changed to have different ground models for different levels
                    for (auto m : plane->meshes) delete m;
                    plane->meshes.clear();
                    plane->init(core, path, texMgr);
                    currentGroundModel = path;
                }
            }
        }
        file.close();
    }
    return true;
}

// this is the main entry point of the program
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	// we create the main window, core rendering system, shader manager, PSO manager, texture manager, and game objects first
    Window win;
    Core core;
    Timer tim;

    ShaderManager shaderMgr;
    PSOManager psoMgr;
    TextureManager texMgr;

	// we have different game objects like ground plane, crosshair, enemy and character models, skybox, bullet sphere, player and animation managers
    GroundPlane planeModel;
    Crosshair crosshair;
    AnimatedMesh enemyModel;
    AnimatedMesh characterModel;
    Skybox skybox;
    Sphere bulletSphere;
    AnimationInstance characterAnim;
    Grass* grassSystem = nullptr;
    Player player;
    PlayerAnimManager playerAnimMgr;

	win.initialize(1024, 1024, "Game Scene"); // we initialize the window as 1024x1024 resolution named Game Scene
	core.initialize(win.hwnd, 1024, 1024); // we initialize the core rendering system with the window handle and size

	crosshair.init(&core); // we initialize the crosshair

    // we create and initialize the grass system with 15000 instances over a 50x50 area
	grassSystem = new Grass(); 
	grassSystem->init(&core, &shaderMgr, &psoMgr, &texMgr, 15000, "Models/Grass_04g.gem", 25.0f); // I used a grass model from the 8 GB Models pack available on Moodle

	enemyModel.load(&core, "Models/Soldier1.gem", &psoMgr, &shaderMgr, &texMgr); // we load the enemy and character models from GEM files
	characterModel.load(&core, "Models/AutomaticCarbine.gem", &psoMgr, &shaderMgr, &texMgr); // this is the assault rifle model which looks like CSGO type of player so we use it as the character model
	bulletSphere.init(&core, 12, 12, 1.0f); // we initialize the bullet sphere mesh with radius 1.0f
	skybox.init(&core, &shaderMgr, &psoMgr, &texMgr); // we initialize the skybox

	characterAnim.init(&characterModel.animation, 0); // we initialize the character animation instance with the character model's animation data

	bulletMgr.init(&bulletSphere); // we initialize the bullet manager with the bullet sphere mesh
	enemyMgr.init(&enemyModel); // we initialize the enemy manager with the enemy model
	playerAnimMgr.init(&characterAnim, &bulletMgr); // we initialize the player animation manager with the character animation instance and bullet manager

	player.init(Vec3(0, 0, -10)); // we initialize the player position at (0, 0, -10)

	ShowCursor(FALSE); // we hide the OS cursor as we have our own crosshair

	int currentLevel = 1; // we start from level 1
	LoadLevel("LevelData1.txt", &core, &texMgr, &psoMgr, &shaderMgr, &planeModel); // we load the first level from my predefined level text file

	tim.reset(); // we reset the timer to start measuring delta time

	float levelTransitionDelay = 3.0f; // 3 seconds delay before transitioning to the next level to have a better experience
	float currentLevelTimer = 0.0f; // timer for level transition
	bool isLevelFinished = false; // to check if the level is finished

	int frameCount = 0; // for FPS calculation
    float timeElapsed = 0.0f;

	while (true) // the main game loop starts here
    {
		core.beginFrame(); // we begin the frame rendering
		win.processMessages(); // we process window messages

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { // if escape key is pressed we exit the game no matter what
            ShowWindow(win.hwnd, SW_HIDE);
            break;
        }
        core.beginRenderPass();

		float dt = tim.dt(); // we get the delta time since last frame
		float totalTime = tim.totalTime(); // for the wind effect in grass we get the total elapsed time

		frameCount++; // we count frames for FPS calculation
        timeElapsed += dt;

        if (timeElapsed >= 1.0f) {
            float fps = (float)frameCount;          
            float mspf = 1000.0f / fps;           

            string fpsString = "Game Scene - FPS: " + to_string((int)fps) +
                " | Frame Time: " + to_string(mspf) + " ms";

            SetWindowTextA(win.hwnd, fpsString.c_str());

            frameCount = 0;
            timeElapsed = 0.0f;
        }

		player.update(dt, &win, obstacles, enemyMgr.getEnemies()); // we update the player with input, obstacles and enemies for collision detection
		playerAnimMgr.update(dt, player, obstacles, enemyMgr.getEnemies()); // we update the player animation manager with player state and collisions

		if (player.isReloading && playerAnimMgr.isCurrentActionFinished()) { // if player is reloading and reload animation is finished we complete the reload
            player.completeReload();
        }

		enemyMgr.update(dt, player.position); // we update the enemy manager with delta time and player position for enemy AI
		bulletMgr.update(dt, enemyMgr, obstacles); // we update the bullet manager with delta time, enemy manager and obstacles for bullet movement and collision

		bool allDead = true; // we check if all enemies are dead to finish the level
        auto& enemies = enemyMgr.getEnemies();

        if (enemies.empty()) {
            allDead = false;
        }

        for (auto e : enemies) { 
			if (!e->isDead) { // if any enemy is alive we set allDead to false which means that level is not finished yet
                allDead = false;
                break;
            }
        }

		if (allDead && !enemies.empty() && !isLevelFinished) { // if all enemies are dead and level is not already marked as finished we mark it and start the transition timer
            isLevelFinished = true;
            currentLevelTimer = levelTransitionDelay;
        }

        if (isLevelFinished) {
			currentLevelTimer -= dt; // we countdown the level transition timer

			if (currentLevelTimer <= 0.0f) { // if timer reaches zero we load the next level or exit if no more levels
                if (currentLevel == 2) {
                    ShowWindow(win.hwnd, SW_HIDE);
                    break;
                }

                currentLevel++;
				string nextLevel = "LevelData" + to_string(currentLevel) + ".txt"; // we load the LevelData2.txt for level 2 etc.

				if (LoadLevel(nextLevel, &core, &texMgr, &psoMgr, &shaderMgr, &planeModel)) { // if level loads successfully we reset player position and states
                    player.init(Vec3(0, 0, -10)); 
                    tim.reset();              
                    isLevelFinished = false;     
                }
				else { // if level fails to load we exit the game
                    ShowWindow(win.hwnd, SW_HIDE);
                    break;
                }
                isLevelFinished = false;
            }
        }

		float aspect = (float)win.width / (float)win.height; // we calculate aspect ratio for projection matrix
        Matrix p;
		p = p.perspectiveProjection(aspect, 60.0f, 0.01f, 10000.0f); // we create the projection matrix
		Matrix v = player.getViewMatrix(); // we get the view matrix from the player
		Matrix vp = v * p; // we calculate the view-projection matrix

		skybox.draw(&core, &psoMgr, v, p); // we draw the skybox first
		planeModel.draw(&core, worldPlane, vp, &texMgr); // we draw the ground plane

		if (grassSystem) { // we draw the grass system if it is initialized
            grassSystem->draw(&core, &psoMgr, &shaderMgr, &texMgr, vp, totalTime);
        }

		for (int i = 0; i < staticRenderList.size(); i++) { // we draw all static mesh render items in the level
            if (staticRenderList[i].mesh) {
                staticRenderList[i].mesh->draw(&core, staticRenderList[i].transform, vp, &texMgr);
            }
        }

		enemyMgr.draw(&core, &psoMgr, &shaderMgr, &texMgr, vp); // we draw all enemies
		bulletMgr.draw(&core, vp); // we draw all bullets

        Matrix identityView; 
		Matrix weaponVP = identityView * p; // for weapon rendering we use identity view matrix to keep it fixed on screen
		Matrix gunS; 
        gunS.scaling(Vec3(0.02f, 0.02f, 0.02f)); // we scale down the weapon model
        Matrix gunR; 
		gunR.rotAroundY(3.14159f); // we rotate the weapon model to face forward
        Matrix gunT; 
        gunT.translation(Vec3(0.05f, -0.07f, 0.15f));
        Matrix gunWorld = gunS * gunR * gunT;

        characterModel.draw(&core, &psoMgr, &shaderMgr, &texMgr, &characterAnim, weaponVP, gunWorld); // we draw the character
        crosshair.draw(&core); // and the crosshair

        core.finishFrame(); // everything is finished
    }

    core.flushGraphicsQueue();
    if (grassSystem) {  // if there is a grass system we delete it
        delete grassSystem; 
        grassSystem = nullptr;
    }
    for (auto const& [key, val] : meshCache) { // we delete all the meshes
        delete val;
    }
    ShowCursor(TRUE); // the cursor can be visible again

    return 0;
}
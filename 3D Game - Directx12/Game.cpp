#include "Window.h"
#include "Core.h"
#include "PSOManager.h"
#include "ShaderManager.h"
#include "maths.h"
#include "Vertex.h"
#include "Camera.h"
#include "Plane.h"
#include "Cube.h"
#include "Mesh.h"
#include "Sphere.h"
#include "GEMLoader.h"
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
#include <cmath>
#include <map>
#include <string>

using namespace std;

extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

class Timer {
public:
    std::chrono::steady_clock::time_point last;
    Timer() { last = std::chrono::steady_clock::now(); }

    void reset() { last = std::chrono::steady_clock::now(); }

    float dt() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> diff = now - last;
        last = now;
        return diff.count();
    }
};

struct RenderItem {
    StaticMesh* mesh;
    Matrix transform;
    AABB collider;
};

map<string, StaticMesh*> meshCache;
vector<RenderItem> staticRenderList;
vector<AABB> obstacles;
EnemyManager enemyMgr;
BulletManager bulletMgr;
Matrix worldPlane;
string currentGroundModel = ""; 

void ClearLevel() {
    staticRenderList.clear();
    obstacles.clear();
    enemyMgr.reset();
    bulletMgr.reset();
}

void LoadLevel(string filename, Core* core, TextureManager* texMgr, PSOManager* psoMgr, ShaderManager* shaderMgr, GroundPlane* plane) {
    ClearLevel();

    float mapLimit = 48.0f;
    float wallThick = 10.0f;
    float wallH = 100.0f;

    obstacles.push_back(AABB(Vec3(-50, 0, mapLimit), Vec3(50, wallH, mapLimit + wallThick)));
    obstacles.push_back(AABB(Vec3(-50, 0, -mapLimit - wallThick), Vec3(50, wallH, -mapLimit)));
    obstacles.push_back(AABB(Vec3(mapLimit, 0, -50), Vec3(mapLimit + wallThick, wallH, 50)));
    obstacles.push_back(AABB(Vec3(-mapLimit - wallThick, 0, -50), Vec3(-mapLimit, wallH, 50)));

    ifstream file(filename);
    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            if (line.empty() || line[0] == '#') continue;

            stringstream ss(line);
            string type, path;
            Vec3 pos, rot, sc;

            ss >> type >> path
                >> pos.x >> pos.y >> pos.z
                >> rot.x >> rot.y >> rot.z
                >> sc.x >> sc.y >> sc.z;

            Matrix S, RX, RY, T;
            S.scaling(sc);
            RX.rotationX(rot.x);
            RY.rotAroundY(rot.y);
            Matrix R = RX * RY;
            T.translation(pos);

            Matrix worldMatrix = S * R * T;

            if (type == "TREE" || type == "WALL")
            {
                if (meshCache.find(path) == meshCache.end())
                {
                    StaticMesh* newMesh = new StaticMesh();
                    newMesh->init(core, path, texMgr);
                    meshCache[path] = newMesh;
                }

                RenderItem item;
                item.mesh = meshCache[path];
                item.transform = worldMatrix;

                Vec3 colSize(1.0f, 1.0f, 1.0f);
                Vec3 centerOffset(0, 0, 0);

                if (type == "TREE") {
                    colSize = Vec3(1.0f, 10.0f, 1.0f);
                    centerOffset = Vec3(0, 5.0f, 0);
                }
                else {
                    float baseThickness = 0.5f;
                    float baseHeight = 5.0f;

                    float baseWidth = 3.8f;

                    if (sc.y < 5.0f) { // if we use it as a cover we make the bounding box smaller
						baseWidth = 2.7f; // manually calcuated for the wall model by simply testing it multiple times
                    }

                    float w = baseThickness * sc.x;
                    float h = baseHeight * sc.y;
                    float d = baseWidth * sc.z;

                    if (abs(rot.y) > 1.0f) {
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
            else if (type == "ENEMY")
            {
                enemyMgr.spawnEnemy(pos, sc);
            }
            else if (type == "PLANE")
            {
                worldPlane = worldMatrix;

                if (path != currentGroundModel) {
                    for (auto m : plane->meshes) delete m;
                    plane->meshes.clear();
                    plane->init(core, path, texMgr);
                    currentGroundModel = path;
                }
            }
        }
        file.close();
    }
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    Window win;
    Core core;
    Timer tim;

    ShaderManager shaderMgr;
    PSOManager psoMgr;
    TextureManager texMgr;

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

    win.initialize(1024, 1024, "Game Scene");
    core.initialize(win.hwnd, 1024, 1024);

    crosshair.init(&core);

    grassSystem = new Grass();
    grassSystem->init(&core, &shaderMgr, &psoMgr, &texMgr, 15000, "Models/Grass_04g.gem", 25.0f);

    enemyModel.load(&core, "Models/Soldier1.gem", &psoMgr, &shaderMgr, &texMgr);
    characterModel.load(&core, "Models/AutomaticCarbine.gem", &psoMgr, &shaderMgr, &texMgr);
    bulletSphere.init(&core, 12, 12, 1.0f);
    skybox.init(&core, &shaderMgr, &psoMgr, &texMgr);

    characterAnim.init(&characterModel.animation, 0);

    bulletMgr.init(&bulletSphere);
    enemyMgr.init(&enemyModel);
    playerAnimMgr.init(&characterAnim, &bulletMgr);

    player.init(Vec3(0, 0, -10));

    ShowCursor(FALSE);

    int currentLevel = 1;
    LoadLevel("LevelData1.txt", &core, &texMgr, &psoMgr, &shaderMgr, &planeModel);

    tim.reset();

    float levelTransitionDelay = 3.0f;
    float currentLevelTimer = 0.0f;
    bool isLevelFinished = false;

    while (true)
    {
        core.beginFrame();
        win.processMessages();

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            ShowWindow(win.hwnd, SW_HIDE);
            break;
        }
        core.beginRenderPass();

        float dt = tim.dt();

        player.update(dt, &win, obstacles, enemyMgr.getEnemies());
        playerAnimMgr.update(dt, player, obstacles, enemyMgr.getEnemies());

        if (player.isReloading && playerAnimMgr.isCurrentActionFinished()) {
            player.completeReload();
        }

        enemyMgr.update(dt, player.position);
        bulletMgr.update(dt, enemyMgr, obstacles);

        bool allDead = true;
        auto& enemies = enemyMgr.getEnemies();

        if (enemies.empty()) allDead = false;

        for (auto e : enemies) {
            if (!e->isDead) {
                allDead = false;
                break;
            }
        }

        if (allDead && !enemies.empty() && !isLevelFinished) {
            isLevelFinished = true;
            currentLevelTimer = levelTransitionDelay;
        }

        if (isLevelFinished) {
            currentLevelTimer -= dt;

            if (currentLevelTimer <= 0.0f) {
                if (currentLevel == 2) {
                    ShowWindow(win.hwnd, SW_HIDE);
                    break;
                }

                currentLevel++;
                string nextLevel = "LevelData" + to_string(currentLevel) + ".txt";

                ifstream check(nextLevel);
                if (check.good()) {
                    check.close();
                    player.init(Vec3(0, 0, -10));
                    LoadLevel(nextLevel, &core, &texMgr, &psoMgr, &shaderMgr, &planeModel);
                    tim.reset();                }
                else {
                    ShowWindow(win.hwnd, SW_HIDE);
                    break;
                }
                isLevelFinished = false;
            }
        }

        float aspect = (float)win.width / (float)win.height;
        Matrix p;
        p = p.perspectiveProjection(aspect, 60.0f, 0.01f, 10000.0f);
        Matrix v = player.getViewMatrix();
        Matrix vp = v * p;

        skybox.draw(&core, &psoMgr, v, p);
        planeModel.draw(&core, worldPlane, vp, &texMgr);

        if (grassSystem) grassSystem->draw(&core, &psoMgr, &shaderMgr, &texMgr, vp);

        for (int i = 0; i < staticRenderList.size(); i++) {
            if (staticRenderList[i].mesh) {
                staticRenderList[i].mesh->draw(&core, staticRenderList[i].transform, vp, &texMgr);
            }
        }

        enemyMgr.draw(&core, &psoMgr, &shaderMgr, &texMgr, vp);
        bulletMgr.draw(&core, vp);

        Matrix identityView;
        Matrix weaponVP = identityView * p;
        Matrix gunS; gunS.scaling(Vec3(0.02f, 0.02f, 0.02f));
        Matrix gunR; gunR.rotAroundY(3.14159f);
        Matrix gunT; gunT.translation(Vec3(0.05f, -0.07f, 0.15f));
        Matrix gunWorld = gunS * gunR * gunT;

        characterModel.draw(&core, &psoMgr, &shaderMgr, &texMgr, &characterAnim, weaponVP, gunWorld);
        crosshair.draw(&core);

        core.finishFrame();
    }

    core.flushGraphicsQueue();
    if (grassSystem) { delete grassSystem; grassSystem = nullptr; }
    for (auto const& [key, val] : meshCache) delete val;
    ShowCursor(TRUE);

    return 0;
}
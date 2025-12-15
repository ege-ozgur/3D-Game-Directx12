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
#include "Skybox.h"
#include "Crosshair.h" 
#include "Grass.h"
#include <chrono>
#include <vector>
#include <cmath>
#include <map> 

using namespace std;

extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

const string SHADER_PATH_VS = "vertexShader.hlsl";
const string SHADER_PATH_PS = "pixelShader.hlsl";

class Timer {
public:
    std::chrono::steady_clock::time_point last;
    Timer() { last = std::chrono::steady_clock::now(); }
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

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    Window win;
    Core core;
    Timer tim;

    ShaderManager shaderMgr;
    PSOManager psoMgr;
    TextureManager texMgr;

    StaticMesh planeModel;
    Crosshair crosshair;

    AnimatedMesh enemyModel;
    AnimatedMesh characterModel;

    Skybox skybox;
    Sphere bulletSphere;

    AnimationInstance characterAnim;
    Grass* grassSystem = nullptr;

    Player player;
    PlayerAnimManager playerAnimMgr;
    EnemyManager enemyMgr;
    BulletManager bulletMgr;

    map<string, StaticMesh*> meshCache;
    vector<RenderItem> staticRenderList;
    vector<AABB> obstacles;

    Matrix worldPlane;

    win.initialize(1024, 1024, "Game Scene");
    core.initialize(win.hwnd, 1024, 1024);

    planeModel.init(&core, "Models/Plane_01a.gem", &texMgr);
    crosshair.init(&core);

    grassSystem = new Grass();
    grassSystem->init(&core, &shaderMgr, &psoMgr, &texMgr, 10000, "Models/Grass_04g.gem");

    enemyModel.load(&core, "Models/Soldier1.gem", &psoMgr, &shaderMgr, &texMgr);
    characterModel.load(&core, "Models/AutomaticCarbine.gem", &psoMgr, &shaderMgr, &texMgr);

    bulletSphere.init(&core, 12, 12, 1.0f);

    characterAnim.init(&characterModel.animation, 0);

    skybox.init(&core, &shaderMgr, &psoMgr, &texMgr);

    bulletMgr.init(&bulletSphere);
    playerAnimMgr.init(&characterAnim, &bulletMgr);
    enemyMgr.init(&enemyModel);

    player.init(Vec3(0, 0, -10));

    ShowCursor(FALSE);

    worldPlane.scaling(Vec3(50.0f, 1.0f, 50.0f));
    worldPlane.translation(Vec3(0.0f, -0.1f, 0.0f));

    ifstream file("LevelData.txt");

    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

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

            if (type == "TREE")
            {
                if (meshCache.find(path) == meshCache.end())
                {
                    StaticMesh* newMesh = new StaticMesh();
                    newMesh->init(&core, path, &texMgr);
                    meshCache[path] = newMesh;
                }

                RenderItem item;
                item.mesh = meshCache[path];
                item.transform = worldMatrix;

                Vec3 colSize(1.0f, 10.0f, 1.0f);
                Vec3 halfSize = colSize * 0.5f;
                Vec3 centerPos = pos;
                centerPos.y += 5.0f;

                item.collider.min = centerPos - halfSize;
                item.collider.max = centerPos + halfSize;

                staticRenderList.push_back(item);
                obstacles.push_back(item.collider);
            }
            else if (type == "PLANE")
            {
                worldPlane = worldMatrix;
            }
            else if (type == "ENEMY")
            {
                enemyMgr.spawnEnemy(pos, sc);
            }
            else if (type == "WALL")
            {
                if (meshCache.find(path) == meshCache.end())
                {
                    StaticMesh* newMesh = new StaticMesh();
                    newMesh->init(&core, path, &texMgr);
                    meshCache[path] = newMesh;
                }

                RenderItem item;
                item.mesh = meshCache[path];
                item.transform = worldMatrix;

                float baseThickness = 2.0f;
                float baseHeight = 5.0f;
                float baseWidth = 4.0f;

                float w = baseThickness * sc.x;
                float h = baseHeight * sc.y;
                float d = baseWidth * sc.z;

                if (abs(rot.y) > 0.1f)
                    std::swap(w, d);

                Vec3 colSize(w, h, d);
                Vec3 halfSize = colSize * 0.5f;

                Vec3 centerPos = pos;
                centerPos.y += halfSize.y;

                item.collider.min = centerPos - halfSize;
                item.collider.max = centerPos + halfSize;

                staticRenderList.push_back(item);
                obstacles.push_back(item.collider);
            }
        }
        file.close();
    }

    float mapLimit = 48.0f;
    float wallThick = 10.0f;
    float wallH = 100.0f;

    Vec3 sizeN(100.0f, wallH, wallThick);
    Vec3 posN(0.0f, 0.0f, mapLimit + 5.0f);
    AABB wN(posN - sizeN * 0.5f, posN + sizeN * 0.5f);
    obstacles.push_back(wN);

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

        float aspect = (float)win.width / (float)win.height;
        Matrix p;
        p = p.perspectiveProjection(aspect, 60.0f, 0.1f, 10000.0f);

        Matrix v = player.getViewMatrix();
        Matrix vp = v * p;

        skybox.draw(&core, &psoMgr, v, p);

        planeModel.draw(&core, worldPlane, vp, &texMgr);

        if (grassSystem)
            grassSystem->draw(&core, &psoMgr, &shaderMgr, &texMgr, vp);

        for (int i = 0; i < staticRenderList.size(); i++)
            staticRenderList[i].mesh->draw(&core, staticRenderList[i].transform, vp, &texMgr);

        enemyMgr.draw(&core, &psoMgr, &shaderMgr, &texMgr, vp);
        bulletMgr.draw(&core, vp);

        Matrix identityView;
        Matrix weaponVP = identityView * p;

        Matrix gunS;
        gunS.scaling(Vec3(0.02f, 0.02f, 0.02f));
        Matrix gunR;
        gunR.rotAroundY(3.14159f);
        Matrix gunT;
        gunT.translation(Vec3(0.05f, -0.07f, 0.15f));
        Matrix gunWorld = gunS * gunR * gunT;

        characterModel.draw(&core, &psoMgr, &shaderMgr, &texMgr, &characterAnim, weaponVP, gunWorld);

        crosshair.draw(&core);

        core.finishFrame();
    }

    core.flushGraphicsQueue();

    if (grassSystem) {
        delete grassSystem;
        grassSystem = nullptr;
    }

    for (auto const& [key, val] : meshCache)
        delete val;

    ShowCursor(TRUE);

    return 0;
}
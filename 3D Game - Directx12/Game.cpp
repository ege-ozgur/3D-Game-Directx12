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
#include <chrono>
#include <vector>
#include <cmath>

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

    Plane planeModel;

    AnimatedMesh dinoModel;
    AnimatedMesh characterModel;
    AnimationInstance dinoAnim;
    AnimationInstance characterAnim;

    Player player;
    PlayerAnimManager playerAnimMgr;

    map<string, StaticMesh*> meshCache;
    vector<RenderItem> staticRenderList;
    vector<AABB> obstacles;
    vector<Matrix> wallMatrices;

    Matrix worldPlane;
    Matrix worldEnemy;

    win.initialize(1024, 1024, "Game Scene");
    core.initialize(win.hwnd, 1024, 1024);

    planeModel.init(&core);
    dinoModel.load(&core, "Models/TRex.gem", &psoMgr, &shaderMgr, &texMgr);
    characterModel.load(&core, "Models/AutomaticCarbine.gem", &psoMgr, &shaderMgr, &texMgr);

    dinoAnim.init(&dinoModel.animation, 0);
    dinoAnim.usingAnimation = "run";
    dinoAnim.t = 0;

    characterAnim.init(&characterModel.animation, 0);
    playerAnimMgr.init(&characterAnim); 

    player.init(Vec3(0, 0, -10));

    ShowCursor(FALSE);

    worldPlane.scaling(Vec3(50.0f, 1.0f, 50.0f));
    worldPlane.translation(Vec3(0.0f, -0.1f, 0.0f));

    Matrix eS, eR, eT;
    eS.scaling(Vec3(0.01f, 0.01f, 0.01f));
    eR.rotationX(0);
    eT.translation(Vec3(0.0f, 0.0f, 10.0f));
    worldEnemy = eS * eR * eT;

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
                    newMesh->init(&core, path);
                    meshCache[path] = newMesh;
                }

                RenderItem item;
                item.mesh = meshCache[path];
                item.transform = worldMatrix;

                item.collider.position = pos;
                item.collider.position.y += 5.0f;
                item.collider.size = Vec3(1.0f, 10.0f, 1.0f);

                staticRenderList.push_back(item);
                obstacles.push_back(item.collider);
            }
            else if (type == "PLANE")
            {
                worldPlane = worldMatrix;
            }
            else if (type == "DINO")
            {
                worldEnemy = worldMatrix;
            }
            else if (type == "WALL")
            {
                wallMatrices.push_back(worldMatrix);

                AABB wallCollider;
                wallCollider.position = pos;

                if (abs(rot.y) < 0.1f)
                    wallCollider.size = Vec3(sc.x * 2.0f, sc.z * 2.0f, 1.0f);
                else
                    wallCollider.size = Vec3(1.0f, sc.z * 2.0f, sc.x * 2.0f);

                obstacles.push_back(wallCollider);
            }
        }
        file.close();
    }

    float mapLimit = 48.0f;
    float wallThick = 10.0f;
    float wallH = 100.0f;

    AABB wN = { Vec3(0, 0,  mapLimit + 5), Vec3(100, wallH, wallThick) };
    AABB wS = { Vec3(0, 0, -mapLimit - 5), Vec3(100, wallH, wallThick) };
    AABB wE = { Vec3(mapLimit + 5, 0, 0), Vec3(wallThick, wallH, 100) };
    AABB wW = { Vec3(-mapLimit - 5, 0, 0), Vec3(wallThick, wallH, 100) };

    obstacles.push_back(wN);
    obstacles.push_back(wS);
    obstacles.push_back(wE);
    obstacles.push_back(wW);

    while (true)
    {
        core.beginFrame();
        win.processMessages();

        if (win.keys[VK_ESCAPE])
            break;

        core.beginRenderPass();
        float dt = tim.dt();

        player.update(dt, &win, obstacles);

        playerAnimMgr.update(dt, player);

        if (player.isReloading && playerAnimMgr.isCurrentActionFinished()) {
            player.completeReload(); 
        }

        dinoAnim.update("run", dt);

        float aspect = (float)win.width / (float)win.height;
        Matrix p;
        p = p.perspectiveProjection(aspect, 60.0f, 0.1f, 5000.0f);

        Matrix v = player.getViewMatrix();
        Matrix vp = v * p;

        planeModel.draw(&core, worldPlane, vp);

        for (int i = 0; i < wallMatrices.size(); i++)
            planeModel.draw(&core, wallMatrices[i], vp);

        for (int i = 0; i < staticRenderList.size(); i++)
            staticRenderList[i].mesh->draw(&core, staticRenderList[i].transform, vp);

        dinoModel.draw(&core, &psoMgr, &shaderMgr, &texMgr, &dinoAnim, vp, worldEnemy);

        Matrix identityView;

        Matrix weaponVP = identityView * p;

        Matrix gunS;
        gunS.scaling(Vec3(0.02f, 0.02f, 0.02f));

        Matrix gunR;
        gunR.rotAroundY(3.14159f);

        Matrix gunT;
        gunT.translation(Vec3(0.05f, -0.07f, 0.15f));

        Matrix gunWorld = gunS * gunR * gunT;

        characterModel.draw(
            &core,
            &psoMgr,
            &shaderMgr,
            &texMgr,
            &characterAnim,
            weaponVP,
            gunWorld
        );

        core.finishFrame();
    }

    for (auto const& [key, val] : meshCache)
        delete val;

    ShowCursor(TRUE);
    core.flushGraphicsQueue();
    return 0;
}
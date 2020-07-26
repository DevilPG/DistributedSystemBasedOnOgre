#include "Ogre.h"
#include "OgreApplicationContext.h"
#include "OgreInput.h"
#include "OgreRTShaderSystem.h"
#include "OgreTrays.h"
#include <OgreManualObject.h>
#include <OgreMath.h>
#include <iostream>
#include <string>
#include <cmath>
#include <thread>
#include <vector>
#include <mutex>


using namespace Ogre;
using namespace OgreBites;
using namespace std;

struct Object
{
    std::vector<Vector3> v;
    std::vector<Vector3> vn;
    std::vector<Vector2> vt;
    std::vector<Vector3> f;
    std::vector<Vector3> ft;
    std::vector<bool> visible;
    string material;
};

static float near_distance = 5;
static float far_distance = 2000;

static string texture_name = "file";

static mutex mutex_lock;
static int main_flag(0);
static std::vector<int> t_flags;

static bool stop_flag = false;
static bool render_flag = false;
static int Num_Of_Threads = 4;
void renderFunction(int id);
static int movement_x = 0;
static int movement_y = 0;
static double scale_factor = 1.0;
Ogre::TexturePtr* lTextureWithRtt;
static string obj1 = "D:\\Test\\Virtual\\Project1\\Centurion_helmet.obj";
static string obj2 = "D:\\Test\\Virtual\\Project1\\Pasha_guard_head.obj";
static string path = "D:/ProgramFiles/OGRE/sdk/Media/materials/textures/filedata.PNG";

class MainApp
    : public ApplicationContext
    , public InputListener
    , public TrayListener
{
private:
    Root* root;
    SceneManager* scnMgr;
    SceneManager** Mgrs;
    Light* light;
    SceneNode* lightNode;
    SceneNode* camNode;
    SceneNode** threadLightNodes;
    SceneNode** threadCamNodes;
    SceneNode** helmet_Nodes;
    SceneNode** face_Nodes;
    SceneNode* ogreNode1;
    Camera* cam;
    RenderWindow* rw;
    Camera** myCameras;
    RenderWindow** myRenderWindows;
    RenderTarget::FrameStats FS;
    ManualObject* ShowPlane;
    Entity* plane;
    float width = 0;
    float hight = 0;
    int num_of_thread = 4;
    std::vector<Object> helmet_objs;
    std::vector<Object> face_objs;
public:
    MainApp();
    MainApp(int);
    virtual ~MainApp() {}

    void setup();
    bool keyPressed(const KeyboardEvent& evt);
    void read_data(string file, string mat, Object& object);
    std::vector<Real> split(string str);
    void createMaterial(string mat);
    Real getNear(Vector3 location, Object obj);
    Real getFar(Vector3 location, Object obj);
    Object getByViewport(SceneNode* camNode, Camera* cam, Object object);
    bool frameRenderingQueued(const FrameEvent& evt);
    void draw_obj(Object object, ManualObject* obj);
    void threadRenderScene(int id);
    void mergeImages();
    bool mouseWheelRolled(const MouseWheelEvent& evt);
    void translate(int mx, int my);
    void SaveImage(TexturePtr TextureToSave, String filename);
};

MainApp::MainApp()
    : ApplicationContext("MyFirstApp")
{
}

MainApp::MainApp(int n)
{
    num_of_thread = n;
}

void MainApp::setup()
{
    ApplicationContext::setup();
    addInputListener(this);

    rw = this->getRenderWindow();
    width = rw->getWidth();
    hight = rw->getHeight();
    
    root = getRoot();

    scnMgr = root->createSceneManager();
    RTShader::ShaderGenerator* shadergen = RTShader::ShaderGenerator::getSingletonPtr();
    shadergen->addSceneManager(scnMgr);
    cout << num_of_thread << endl;
    Mgrs = new SceneManager * [num_of_thread];
    threadLightNodes = new SceneNode * [num_of_thread];
    threadCamNodes = new SceneNode * [num_of_thread];
    helmet_Nodes = new SceneNode * [num_of_thread];
    face_Nodes = new SceneNode * [num_of_thread];
    myCameras = new Camera * [num_of_thread];
    myRenderWindows = new RenderWindow * [num_of_thread];
    for (int i = 0; i < num_of_thread; i++) {
        Mgrs[i] = root->createSceneManager();
        shadergen->addSceneManager(Mgrs[i]);
        threadLightNodes[i] = Mgrs[i]->getRootSceneNode()->createChildSceneNode();
        threadCamNodes[i] = Mgrs[i]->getRootSceneNode()->createChildSceneNode();
        helmet_Nodes[i] = Mgrs[i]->getRootSceneNode()->createChildSceneNode();
        face_Nodes[i] = Mgrs[i]->getRootSceneNode()->createChildSceneNode();
        string windowname = "threadWindow";
        windowname += (i + '0');
        myRenderWindows[i] = root->createRenderWindow(windowname, 640, 480, false);
        //myRenderWindows[i]->setHidden(true);
    }

    scnMgr->setAmbientLight(ColourValue(0.6, 0.6, 0.6));

    light = scnMgr->createLight("MainLight");
    lightNode = scnMgr->getRootSceneNode()->createChildSceneNode();
    lightNode->attachObject(light);
    //! [newlight]

    //! [lightpos]
    lightNode->setPosition(0, 200, 100);
    //! [lightpos]

    camNode = scnMgr->getRootSceneNode()->createChildSceneNode();
    
    cam = scnMgr->createCamera("myCam");
    cam->setNearClipDistance(5); // specific to this sample
    cam->setAutoAspectRatio(true);
    camNode->attachObject(cam);

    getRenderWindow()->addViewport(cam);
    camNode->setPosition(0, 0, 1000);

    //Draw the showing plane
    ShowPlane = scnMgr->createManualObject("ground");
    ShowPlane->begin("black", RenderOperation::OT_TRIANGLE_LIST);
    ShowPlane->position(465.073, 348.805, 157.91);
    ShowPlane->normal(0, 0, 1);
    ShowPlane->textureCoord(1, 0);

    ShowPlane->position(-465.073, 348.805, 157.91);
    ShowPlane->normal(0, 0, 1);
    ShowPlane->textureCoord(0, 0);

    ShowPlane->position(-465.073, -348.805, 157.91);
    ShowPlane->normal(0, 0, 1);
    ShowPlane->textureCoord(0, 1);

    ShowPlane->position(465.073, -348.805, 157.91);
    ShowPlane->normal(0, 0, 1);
    ShowPlane->textureCoord(1, 1);

    ShowPlane->quad(0, 1, 2, 3);
    ShowPlane->end();
    MeshPtr mp1 = ShowPlane->convertToMesh("ShowingPlane");

    plane = scnMgr->createEntity("blackboard", mp1);
    //plane->setCastShadows(false);
    ogreNode1 = scnMgr->getRootSceneNode()->createChildSceneNode();
    ogreNode1->attachObject(plane);

    //Read Obj Models
    Object helmet_obj;
    Object face_obj;
    read_data(obj1, "Centurion_helmet_0.jpg", helmet_obj);
    read_data(obj2, "Pasha_guard_head_0.png", face_obj);
    createMaterial("Centurion_helmet_0.jpg");
    createMaterial("Pasha_guard_head_0.png");
    for (int i = 0; i < helmet_obj.v.size(); i++) {
        helmet_obj.v[i] *= 9;
    }
    for (int i = 0; i < helmet_obj.v.size(); i++) {
        helmet_obj.v[i].y += 80;
    }
    for (int i = 0; i < face_obj.v.size(); i++) {
        Real old_y = face_obj.v[i].y;
        face_obj.v[i].y = face_obj.v[i].z;
        face_obj.v[i].z = -old_y;
    }

    Vector3 pos = camNode->getPosition();
    far_distance = getFar(pos, helmet_obj) > getFar(pos, face_obj) ? getFar(pos, helmet_obj) : getFar(pos, face_obj);
    far_distance += 10;
    near_distance = getNear(pos, helmet_obj) < getNear(pos, face_obj) ? getNear(pos, helmet_obj) : getNear(pos, face_obj);
    near_distance -= 10;
    float depth = (far_distance - near_distance) / num_of_thread;
    
    
    //Split the model, and create cameras and windows
    for (int i = 0; i < num_of_thread; i++) {
        string cameraname = "myCamera";
        cameraname += (i + '0');
        myCameras[i] = Mgrs[i]->createCamera(cameraname);
        myCameras[i]->setNearClipDistance(near_distance + i * depth);
        myCameras[i]->setFarClipDistance(near_distance + (i + 1) * depth);
        helmet_objs.push_back(getByViewport(camNode, myCameras[i], helmet_obj));
        face_objs.push_back(getByViewport(camNode, myCameras[i], face_obj));
        myCameras[i]->setNearClipDistance(5);
        myCameras[i]->setFarClipDistance(2000);
    }
    //std::vector<thread> Threads;
    for (int i = 0; i < num_of_thread; i++) {
        threadRenderScene(i);
        //Threads.push_back(thread(threadRenderScene, i));
    }

    //resource group
    Ogre::String lNameOfResourceGroup = "MyOgreProj Res";
    {
        Ogre::ResourceGroupManager& lRgMgr = Ogre::ResourceGroupManager::getSingleton();
        lRgMgr.createResourceGroup(lNameOfResourceGroup);

        // The function 'initialiseResourceGroup' parses scripts if any in the locations.
        lRgMgr.initialiseResourceGroup(lNameOfResourceGroup);

        // Files that can be loaded are loaded.
        lRgMgr.loadResourceGroup(lNameOfResourceGroup);
    }

    //RenderTexture
    Ogre::TextureManager& lTextureManager = Ogre::TextureManager::getSingleton();
    Ogre::String lTextureName = "MyRRt";
    bool lGammaCorrection = false;
    unsigned int lAntiAliasing = 0;
    unsigned int lNumMipmaps = 0;
    lTextureWithRtt = new TexturePtr[num_of_thread];
    for (int i = 0; i < num_of_thread; i++) {
        lTextureName.push_back('A');
        lTextureWithRtt[i] = lTextureManager.createManual(lTextureName, lNameOfResourceGroup,
            Ogre::TEX_TYPE_2D, 640, 480, lNumMipmaps,
            Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, 0, lGammaCorrection, lAntiAliasing);

        //RenderTarget
        Ogre::RenderTexture* mRenderTarget = NULL;
        HardwarePixelBufferSharedPtr mRttBuffer = lTextureWithRtt[i]->getBuffer();
        mRenderTarget = mRttBuffer->getRenderTarget();
        mRenderTarget->setAutoUpdated(true);
        Ogre::Viewport* lRttViewport1 = mRenderTarget->addViewport(myCameras[i]);
        lRttViewport1->setAutoUpdated(true);
        Ogre::ColourValue lBgColor1(0.0, 0.0, 0.0, 1.0);
        lRttViewport1->setBackgroundColour(lBgColor1);
    }
}

void MainApp::SaveImage(TexturePtr TextureToSave, String filename)
{
    Image img;
    TextureToSave->convertToImage(img);
    img.save(filename);
}

void MainApp::translate(int mx, int my)
{
    for (int i = 0; i < num_of_thread; i++) {
        helmet_Nodes[i]->setPosition(mx, my, 0);
        face_Nodes[i]->setPosition(mx, my, 0);
    }
}

bool MainApp::mouseWheelRolled(const MouseWheelEvent& evt) {
    if (evt.y >= 0)
        scale_factor += evt.y / 25.0;
    else
        scale_factor -= -evt.y / 25.0;
    if (scale_factor < 0)
        scale_factor = 0;
    for (int i = 0; i < num_of_thread; i++) {
        helmet_Nodes[i]->setScale(scale_factor, scale_factor, scale_factor);
        face_Nodes[i]->setScale(scale_factor, scale_factor, scale_factor);
    }
    return true;
}

bool MainApp::keyPressed(const KeyboardEvent& evt)
{
    if (evt.keysym.sym == SDLK_ESCAPE)
    {
        stop_flag = true;
        getRoot()->queueEndRendering();
    }
    switch (evt.keysym.sym)
    {
    case SDLK_SPACE:
        FS = rw->getStatistics();
        cout << "Main Window FPS: " << FS.avgFPS << endl;
        cout << "Main Window Batch Count: " << FS.batchCount << endl;
        cout << "Main Window Triangle Count: " << FS.triangleCount << endl;
        cout << "Main Window Best Frame Time: " << FS.bestFrameTime << endl;
        cout << "Main Window Worst Frame Time: " << FS.worstFrameTime << endl;
        for (int i = 0; i < num_of_thread; i++) {
            FS = myRenderWindows[i]->getStatistics();
            cout << "Window " << i << " FPS: " << FS.avgFPS << endl;
            cout << "Window " << i << " Batch Count: " << FS.batchCount << endl;
            cout << "Window " << i << " Triangle Count: " << FS.triangleCount << endl;
            cout << "Window " << i << " Best Frame Time: " << FS.bestFrameTime << endl;
            cout << "Window " << i << " Worst Frame Time: " << FS.worstFrameTime << endl;
        }
        break;
    case SDLK_LSHIFT:
        render_flag = true;
        break;
    case SDLK_UP:
        movement_y += 20;
        translate(movement_x, movement_y);
        break;
    case SDLK_DOWN:
        movement_y -= 20;
        translate(movement_x, movement_y);
        break;
    case SDLK_LEFT:
        movement_x -= 20;
        translate(movement_x, movement_y);
        break;
    case SDLK_RIGHT:
        movement_x += 20;
        translate(movement_x, movement_y);
        break;
    default:
        return false;
    }

    return true;
}

void MainApp::read_data(string file, string mat, Object& object)
{
    ifstream infile(file);
    string line;
    if (!infile.is_open())  cout << "未成功打开文件" << endl;
    std::vector<Vector3> v, f, ft;
    std::vector<Vector2> vt;
    string v1;
    Real v2, v3, v4;
    string s1, s2, s3;
    Vector3 tmp_3;
    Vector2 tmp_2;
    while (getline(infile, line)) {
        if (line[0] == 'v' && line[1] != 't') {
            istringstream si(line);
            si >> v1 >> v2 >> v3 >> v4;
            tmp_3.x = v2;
            tmp_3.y = v3;
            tmp_3.z = v4;
            v.push_back(tmp_3);
        }
        if (line[0] == 'v' && line[1] == 't') {
            istringstream si(line);
            si >> v1 >> v2 >> v3;
            tmp_2.x = v2;
            tmp_2.y = v3;
            vt.push_back(tmp_2);
        }
        if (line[0] == 'f') {
            istringstream si(line);
            si >> v1 >> s1 >> s2 >> s3;
            tmp_3.x = split(s1)[0];
            tmp_3.y = split(s2)[0];
            tmp_3.z = split(s3)[0];
            f.push_back(tmp_3);
            tmp_3.x = split(s1)[1];
            tmp_3.y = split(s2)[1];
            tmp_3.z = split(s3)[1];
            ft.push_back(tmp_3);
        }
        else continue;
    }
    std::vector<Vector3> vn;
    Vector3 ini;
    ini.x = 0;
    ini.y = 0;
    ini.z = 0;
    for (int i = 0; i < v.size(); i++) {
        vn.push_back(ini);
    }
    for (int i = 0; i < f.size(); i++) {
        Vector3 a, b, c;
        a = v[f[i][1] - 1] - v[f[i][0] - 1];
        b = v[f[i][2] - 1] - v[f[i][0] - 1];
        c.x = a.y * b.z - a.z * b.y;
        c.y = a.z * b.x - a.x * b.z;
        c.z = a.x * b.y - a.y * b.x;
        vn[f[i][0] - 1] += c;
        vn[f[i][1] - 1] += c;
        vn[f[i][2] - 1] += c;
    }
    for (int i = 0; i < v.size(); i++) {
        vn[i].normalise();
    }

    object.v = v;
    object.vn = vn;
    object.vt = vt;
    object.f = f;
    object.ft = ft;
    object.material = mat;
    for (int i = 0; i < v.size(); i++) {
        object.visible.push_back(true);
    }
}

std::vector<Real> MainApp::split(string str)
{
    char* strc = new char[strlen(str.c_str()) + 1];
    strcpy(strc, str.c_str());
    std::vector<Real> resultVec;
    string pattern = "/";
    char* tmpStr = strtok(strc, pattern.c_str());
    while (tmpStr != NULL)
    {
        resultVec.push_back((Real)atoi(tmpStr));
        tmpStr = strtok(NULL, pattern.c_str());
    }
    delete[] strc;
    return resultVec;
}

void MainApp::createMaterial(string mat)
{
    MaterialPtr material = MaterialManager::getSingletonPtr()->create(mat, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    material->getTechnique(0)->getPass(0)->createTextureUnitState(mat);
}

Real MainApp::getNear(Vector3 location, Object obj)
{
    Vector3 a = location;
    Vector3 b = obj.v[0];
    Real distance = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
    for (int i = 0; i < obj.v.size(); i++) {
        b = obj.v[i];
        Real tmp = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
        if (tmp < distance) distance = tmp;
    }
    return distance;
}

Real MainApp::getFar(Vector3 location, Object obj)
{
    Vector3 a = location;
    Vector3 b = obj.v[0];
    Real distance = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
    for (int i = 0; i < obj.v.size(); i++) {
        b = obj.v[i];
        Real tmp = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
        if (tmp > distance) distance = tmp;
    }
    return distance;
}

Object MainApp::getByViewport(SceneNode* camNode, Camera* cam, Object object)
{
    std::vector<bool> vis;
    Ray ray;
    Vector3 v1, v2, v3, v4, v5, v6, v7, v8;
    Real distance = 0;
    Vector3 a = camNode->getPosition(), b, direction;
    v1 = cam->getWorldSpaceCorners()[0] + a;
    v2 = cam->getWorldSpaceCorners()[1] + a;
    v3 = cam->getWorldSpaceCorners()[2] + a;
    v4 = cam->getWorldSpaceCorners()[3] + a;
    v5 = cam->getWorldSpaceCorners()[4] + a;
    v6 = cam->getWorldSpaceCorners()[5] + a;
    v7 = cam->getWorldSpaceCorners()[6] + a;
    v8 = cam->getWorldSpaceCorners()[7] + a;
    ray.setOrigin(a);
    for (size_t i = 0; i < object.v.size(); i++) {
        b = object.v[i];
        direction = b - a;
        direction.normalise();
        ray.setDirection(direction);
        if (Math::intersects(ray, v1, v2, v3, true, true).first == 1) {
            distance = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
            if ((distance >= (Math::intersects(ray, v1, v2, v3, true, true).second-1)) && (distance <= (Math::intersects(ray, v5, v6, v7, true, true).second+1)))
                vis.push_back(true);
            else vis.push_back(false);
        }
        else if (Math::intersects(ray, v1, v3, v4, true, true).first == 1) {
            if ((distance >= (Math::intersects(ray, v1, v3, v4, true, true).second-1)) && (distance <= (Math::intersects(ray, v5, v7, v8, true, true).second+1)))
                vis.push_back(true);
            else vis.push_back(false);
        }
        else
        {
            vis.push_back(false);
        }
    }
    Object ob;
    ob.v = object.v;
    ob.vn = object.vn;
    ob.vt = object.vt;
    ob.f = object.f;
    ob.ft = object.ft;
    ob.material = object.material;
    ob.visible = vis;
    return ob;
}

bool MainApp::frameRenderingQueued(const FrameEvent& evt)
{
    if (render_flag) {
        for (int i = 0; i < num_of_thread; i++) {
            string pic_name = "D:/ProgramFiles/OGRE/sdk/Media/materials/textures/file";
            pic_name += ((i + 1) + '0');
            pic_name += ".PNG";
            cout << "Start outputing image..." << pic_name << endl;
            SaveImage(lTextureWithRtt[i] , pic_name);
            cout << "Image generate success" << endl;
        }
        mergeImages();
        texture_name.push_back('A');
        MaterialPtr materialx = MaterialManager::getSingletonPtr()->create(texture_name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        materialx->getTechnique(0)->getPass(0)->createTextureUnitState("filedata.PNG");
        plane->setMaterialName(texture_name);
        //plane->setMaterial(textureMerged);
        render_flag = false;
    }
    return true;
}

void MainApp::draw_obj(Object object, ManualObject* obj)
{
    obj->begin(object.material, RenderOperation::OT_TRIANGLE_LIST);
    int m = 0;
    for (int i = 0; i < object.f.size(); i++) {
        if ((object.visible[object.f[i][0] - 1] == true) || (object.visible[object.f[i][1] - 1] == true) || (object.visible[object.f[i][2] - 1] == true)) {
            obj->position(object.v[object.f[i][0] - 1]);
            obj->normal(object.vn[object.f[i][0] - 1]);
            obj->textureCoord(object.vt[object.ft[i][0] - 1]);
            obj->position(object.v[object.f[i][1] - 1]);
            obj->normal(object.vn[object.f[i][1] - 1]);
            obj->textureCoord(object.vt[object.ft[i][1] - 1]);
            obj->position(object.v[object.f[i][2] - 1]);
            obj->normal(object.vn[object.f[i][2] - 1]);
            obj->textureCoord(object.vt[object.ft[i][2] - 1]);
            obj->triangle(m * 3, m * 3 + 1, m * 3 + 2);
            m++;
        }
    }
    obj->end();
}

void MainApp::threadRenderScene(int id)
{
    //mutex_lock.lock();
    Mgrs[id]->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
    cout << "Thread " << id << "Begin!" << endl;
    string lightname = "threadlight";
    lightname += (id + '0');
    Light* tlight = Mgrs[id]->createLight(lightname);
    threadLightNodes[id]->attachObject(tlight);
    threadLightNodes[id]->setPosition(0, 200, 100);
    //cout << 1111 << endl;
    threadCamNodes[id]->attachObject(myCameras[id]);
    threadCamNodes[id]->setPosition(0, 0, 1000);
   
    ManualObject* tface = Mgrs[id]->createManualObject();
    ManualObject* thelmet = Mgrs[id]->createManualObject();
    //cout << 22222 << endl;
    draw_obj(helmet_objs[id], thelmet);
    helmet_Nodes[id]->attachObject(thelmet);
    //cout << 3333 << endl;
    draw_obj(face_objs[id], tface);
    face_Nodes[id]->attachObject(tface);
    //cout << 4444 << endl;
    myRenderWindows[id]->addViewport(myCameras[id]);
    //mutex_lock.unlock();
    //cout << "ffffffff" << endl;
}

void MainApp::mergeImages()
{
    std::vector<Vector3> image_row;
    std::vector<std::vector<Vector3> > image_data;
    std::vector<std::vector<std::vector<Vector3> > > images;
    string file_name;
    Vector3 tmp;
    Vector3 default_color(0, 0, 0);
    Image image;
    int i = 0, j = 0, k = 0;

    for (i = 0; i < num_of_thread; i++) {
        //file_name = "D:/ProgramFiles/OGRE/sdk/Media/materials/textures/test";
        //file_name += (i + 1 + '0');
        //file_name += ".PNG";
        //cout << file_name << endl;
        //image.load(file_name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        //cout << image.getWidth() << endl;
        //cout << image.getHeight() << endl;
        lTextureWithRtt[i]->convertToImage(image);
        image_data.clear();
        for (j = 0; j < image.getWidth(); j++) {
            image_row.clear();
            for (k = 0; k < image.getHeight(); k++) {
                tmp.x = image.getColourAt(j, k, 0).r;
                tmp.y = image.getColourAt(j, k, 0).g;
                tmp.z = image.getColourAt(j, k, 0).b;
                image_row.push_back(tmp);
                //cout << image.getColourAt(j, k, 0).r << endl;
            }
            image_data.push_back(image_row);
        }
        images.push_back(image_data);
    }
    image_data.clear();
    image_data = images[0];
    cout << images[1][320][240] << endl;
    for (i = 1; i < num_of_thread; i++) {
        for (j = 0; j < 640; j++) {
            for (k = 0; k < 480; k++) {
                if ((images[i][j][k] != default_color) && (image_data[j][k] == default_color))
                    image_data[j][k] = images[i][j][k];
            }
        }
    }
    for (j = 0; j < 640; j++) {
        for (k = 0; k < 480; k++) {
            ColourValue color(image_data[j][k].x, image_data[j][k].y, image_data[j][k].z, 1);
            image.setColourAt(color, j, k, 0);
        }
    }
    image.save(path);
}

//class ThreadApp
//    : public ApplicationContext
//    , public InputListener
//    , public FrameListener
//{
//public:
//    ThreadApp();
//    virtual ~ThreadApp() {}
//    void setup();
//    bool keyPressed(const KeyboardEvent& evt);
//    bool frameRenderingQueued(const FrameEvent& evt);
//    void draw_obj(Object object, ManualObject* obj);
//    RenderWindow* rw;
//    int id;
//    Vector3 location_camera;
//private:
//    Camera* cam;
//    Image image;
//};
//
//ThreadApp::ThreadApp()
//    : ApplicationContext("OgreTutorialApp")
//{
//}
//
//void ThreadApp::setup()
//{
//    cout << "第" << id + 1 << "个线程开始构建渲染场景" << endl;
//    ApplicationContext::setup();
//    addInputListener(this);
//    rw = this->getRenderWindow();
//    Root* root = getRoot();
//    SceneManager* scnMgr = root->createSceneManager();
//
//    RTShader::ShaderGenerator* shadergen = RTShader::ShaderGenerator::getSingletonPtr();
//    shadergen->addSceneManager(scnMgr);
//
//    scnMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
//
//    Light* light = scnMgr->createLight("MainLight");
//    SceneNode* lightNode = scnMgr->getRootSceneNode()->createChildSceneNode();
//    lightNode->attachObject(light);
//    lightNode->setPosition(0, 200, 100);
//
//    SceneNode* camNode = scnMgr->getRootSceneNode()->createChildSceneNode();
//    cam = scnMgr->createCamera("myCam");
//    cam->setNearClipDistance(5);
//    cam->setAutoAspectRatio(true);
//    camNode->attachObject(cam);
//    camNode->setPosition(0, 0, 1000);
//
//    getRenderWindow()->addViewport(cam);
//
//    ManualObject* face = scnMgr->createManualObject();
//    ManualObject* helmet = scnMgr->createManualObject();
//    SceneNode* helmet_Node = scnMgr->getRootSceneNode()->createChildSceneNode();
//    SceneNode* face_Node = scnMgr->getRootSceneNode()->createChildSceneNode();
//
//    draw_obj(helmet_objs[id], helmet);
//    helmet_Node->attachObject(helmet);
//    draw_obj(face_objs[id], face);
//    face_Node->attachObject(face);
//}
//
//void ThreadApp::draw_obj(Object object, ManualObject* obj)
//{
//    obj->begin(object.material, RenderOperation::OT_TRIANGLE_LIST);
//    int m = 0;
//    for (int i = 0; i < object.f.size(); i++) {
//        if ((object.visible[object.f[i][0] - 1] == true) || (object.visible[object.f[i][1] - 1] == true) || (object.visible[object.f[i][2] - 1] == true)) {
//            obj->position(object.v[object.f[i][0] - 1]);
//            obj->normal(object.vn[object.f[i][0] - 1]);
//            obj->textureCoord(object.vt[object.ft[i][0] - 1]);
//            obj->position(object.v[object.f[i][1] - 1]);
//            obj->normal(object.vn[object.f[i][1] - 1]);
//            obj->textureCoord(object.vt[object.ft[i][1] - 1]);
//            obj->position(object.v[object.f[i][2] - 1]);
//            obj->normal(object.vn[object.f[i][2] - 1]);
//            obj->textureCoord(object.vt[object.ft[i][2] - 1]);
//            obj->triangle(m * 3, m * 3 + 1, m * 3 + 2);
//            m++;
//        }
//    }
//    obj->end();
//}
//
//bool ThreadApp::keyPressed(const KeyboardEvent& evt)
//{
//    if (evt.keysym.sym == SDLK_ESCAPE)
//    {
//        getRoot()->queueEndRendering();
//    }
//    switch (evt.keysym.sym)
//    {
//    case SDLK_SPACE:
//        RenderTarget::FrameStats FS = rw->getStatistics();
//        cout << "FPS: " << FS.avgFPS << endl;
//        cout << "Batch Count: " << FS.batchCount << endl;
//        cout << "Triangle Count: " << FS.triangleCount << endl;
//        cout << "Best Frame Time: " << FS.bestFrameTime << endl;
//        cout << "Worst Frame Time: " << FS.worstFrameTime << endl;
//        rw->writeContentsToFile("D:/Test/Virtual/file.PNG");
//        image.load("file.PNG", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
//        cout << image.getWidth() << endl;
//        cout << image.getHeight() << endl;
//        cout << image.getColourAt(320, 240, 0);
//
//        break;
//    default:
//        return false;
//    }
//}
//
//bool ThreadApp::frameRenderingQueued(const FrameEvent& evt)
//{
//    string pic_name = "D:/Test/Virtual/file";
//    pic_name.push_back(char(id));
//    pic_name += ".PNG";
//    rw->writeContentsToFile(pic_name);
//    return true;
//}

int main(int argc, char** argv)
{
    try
    {
        int n;
        cout << "Please input the number of threads: ";
        cin >> n;
        MainApp app(n);
        app.initApp();
        //app.getRoot()->renderOneFrame();
        /*std::vector<thread> Threads;
        std::vector<int>::iterator iter;
        for (int i = 0; i < n; i++) {
            Threads.push_back(thread(&MainApp::threadRenderScene,ref(app), i));
        }*/
        //while (!stop_flag) {
        //    iter = find(t_flags.begin(), t_flags.end(), 0);
        //    if (iter == t_flags.end()) {
        //        //merging images
        //        mutex_lock.lock();
        //        main_flag = 1;
        //        mutex_lock.unlock();
        //    }
        //}
        /*for (int i = 0; i < Threads.size(); i++) {
            Threads.at(i).join();
        }*/
        app.getRoot()->startRendering();
        //app.mergeImages();
        //app.getRoot()->renderOneFrame();
        //while (!stop_flag);
        cout << "rendering success!" << endl;
        //app.closeApp();
        cout << "fucking success" << endl;
        
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error occurred during execution: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

void renderFunction(int id) {

}

//void renderFunction(int id) {
//    try
//    {
//        MainApp app;
//        app.id = id;
//        cout << "创建第" << id + 1 << "个线程成功" << endl;
//        app.initApp();
//        cout << "初始化第" << id + 1 << "个线程成功" << endl;
//        while (!stop_flag) {
//            mutex_lock.lock();
//            t_flags[id] = 0;
//            mutex_lock.unlock();
//
//            app.getRoot()->renderOneFrame();
//
//            mutex_lock.lock();
//            main_flag = 0;
//            t_flags[id] = 1;
//            mutex_lock.unlock();
//            while (!main_flag);
//        }
//    }
//    catch (const std::exception & e)
//    {
//        std::cerr << "Error occurred during execution: " << e.what() << '\n';
//        cout << "初始化失败" << endl;
//    }
//}



#include "../common/common.h"

#include "scene_mgr.h"
#include "build_scene.h"
#include "image_mgr.h"
#include "card_data.h"
#include "deck_data.h"
#include "sungui.h"

using namespace ygopro;

static float xrate = 0.0f;
static float yrate = 0.0f;
static bool need_draw = true;

int main(int argc, char* argv[]) {
    if(!glfwInit())
        return 0;
    if(!commonCfg.LoadConfig(L"common.xml"))
        return 0;
    int width = commonCfg[L"window_width"];
    int height = commonCfg[L"window_height"];
	int fsaa = commonCfg[L"fsaa"];
    int vsync = commonCfg[L"vertical_sync"];
	if(fsaa)
		glfwWindowHint(GLFW_SAMPLES, fsaa);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    auto mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    GLFWwindow* window = glfwCreateWindow(width, height, "Ygopro", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 0;
    }
    glfwSetWindowPos(window, (mode->width - width) / 2, (mode->height - height) / 2);
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
	glewExperimental = true;
    glewInit();

    ImageMgr::Get().InitTextures(commonCfg[L"image_file"]);
    if(!stringCfg.LoadConfig(commonCfg[L"string_conf"])
       || DataMgr::Get().LoadDatas(commonCfg[L"database_file"])
       || !ImageMgr::Get().LoadImageConfig(commonCfg[L"textures_conf"])
       || !sgui::SGGUIRoot::GetSingleton().LoadConfigs(commonCfg[L"gui_conf"])) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
    LimitRegulationMgr::Get().LoadLimitRegulation(commonCfg[L"limit_regulation"], stringCfg[L"eui_list_default"]);
    stringCfg.ForEach([](const std::wstring& name, ValueStruct& value) {
        if(name.find(L"setname_") == 0 ) {
            DataMgr::Get().RegisterSetCode(static_cast<unsigned int>(value), name.substr(8));
        }
    });
    
    int bwidth, bheight;
    glfwGetFramebufferSize(window, &bwidth, &bheight);
    xrate = (float)bwidth / width;
    yrate = (float)bheight / height;
    sgui::SGGUIRoot::GetSingleton().SetSceneSize({bwidth, bheight});
    SceneMgr::Get().Init();
    SceneMgr::Get().SetSceneSize({bwidth, bheight});
    SceneMgr::Get().InitDraw();
    SceneMgr::Get().SetFrameRate((int)commonCfg[L"frame_rate"]);
    
    auto sc = std::make_shared<BuildScene>();
    SceneMgr::Get().SetScene(std::static_pointer_cast<Scene>(sc));

    glfwSetKeyCallback(window, [](GLFWwindow* wnd, int key, int scan, int action, int mods) {
        if(action == GLFW_PRESS) {
            if(key == GLFW_KEY_GRAVE_ACCENT && (mods & GLFW_MOD_ALT))
                SceneMgr::Get().ScreenShot();
            if(!sgui::SGGUIRoot::GetSingleton().InjectKeyDownEvent({key, mods}))
                SceneMgr::Get().KeyDown({key, mods});
        } else if(action == GLFW_RELEASE) {
            if(!sgui::SGGUIRoot::GetSingleton().InjectKeyUpEvent({key, mods}))
                SceneMgr::Get().KeyUp({key, mods});
        }
    });
    glfwSetCharCallback(window, [](GLFWwindow* wnd, unsigned int unichar) {
        sgui::SGGUIRoot::GetSingleton().InjectCharEvent({unichar});
    });
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* wnd, int width, int height) {
        int x, y;
        glfwGetWindowSize(wnd, &x, &y);
        xrate = (float)width / x;
        yrate = (float)height / y;
        SceneMgr::Get().SetSceneSize(v2i{width, height});
        sgui::SGGUIRoot::GetSingleton().SetSceneSize(v2i{width, height});
    });
    glfwSetCursorEnterCallback(window, [](GLFWwindow* wnd, int enter) {
        if(enter == GL_TRUE)
            sgui::SGGUIRoot::GetSingleton().InjectMouseEnterEvent();
        else
            sgui::SGGUIRoot::GetSingleton().InjectMouseLeaveEvent();
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* wnd, double xpos, double ypos) {
        SceneMgr::Get().SetMousePosition({(int)(xpos * xrate), (int)(ypos * yrate)});
        if(!sgui::SGGUIRoot::GetSingleton().InjectMouseMoveEvent({(int)(xpos * xrate), (int)(ypos * yrate)}))
            SceneMgr::Get().MouseMove({(int)(xpos * xrate), (int)(ypos * yrate)});
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* wnd, int button, int action, int mods) {
        double xpos, ypos;
        glfwGetCursorPos(wnd, &xpos, &ypos);
        xpos *= xrate;
        ypos *= yrate;
        SceneMgr::Get().SetMousePosition({(int)xpos, (int)ypos});
        if(action == GLFW_PRESS) {
            if(!sgui::SGGUIRoot::GetSingleton().InjectMouseButtonDownEvent({button, mods, (int)xpos, (int)ypos}))
                SceneMgr::Get().MouseButtonDown({button, mods, (int)xpos, (int)ypos});
        } else {
            if(!sgui::SGGUIRoot::GetSingleton().InjectMouseButtonUpEvent({button, mods, (int)xpos, (int)ypos}))
                SceneMgr::Get().MouseButtonUp({button, mods, (int)xpos, (int)ypos});
        }
    });
    glfwSetScrollCallback(window, [](GLFWwindow* wnd, double xoffset, double yoffset) {
        sgui::SGGUIRoot::GetSingleton().InjectMouseWheelEvent({(float)xoffset, (float)yoffset});
    });
    glfwSetWindowIconifyCallback(window, [](GLFWwindow* wnd, int iconified) {
        need_draw = (iconified == GL_FALSE);
        if(need_draw)
            SceneMgr::Get().SetFrameRate((int)commonCfg[L"frame_rate"]);
        else
            SceneMgr::Get().SetFrameRate(10);
    });
    if(vsync)
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);
    while (!glfwWindowShouldClose(window)) {
        SceneMgr::Get().CheckFrameRate();
        SceneMgr::Get().InitDraw();
        glfwPollEvents();
        SceneMgr::Get().Update();
        if(need_draw) {
            SceneMgr::Get().Draw();
            sgui::SGGUIRoot::GetSingleton().Draw();
        }
        glfwSwapBuffers(window);
    }
    
    SceneMgr::Get().Uninit();
    sgui::SGGUIRoot::GetSingleton().Unload();
    ImageMgr::Get().UninitTextures();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
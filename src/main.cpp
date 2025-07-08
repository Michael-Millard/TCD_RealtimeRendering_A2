#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <my_shader.h>
#include <my_camera.h>
#include <my_model.h>
#include <my_skybox.h>

#include <iostream>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Callback function declarations
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xIn, double yIn);
void scrollCallback(GLFWwindow* window, double xOff, double yOff);
void processUserInput(GLFWwindow* window);

// Global params
unsigned int SCREEN_WIDTH = 1920;
unsigned int SCREEN_HEIGHT = 1080;
bool firstMouse = true;
bool imguiMouseUse = true;

float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate to the left
float pitch = 0.0f;
float xPrev = static_cast<float>(SCREEN_WIDTH) / 2.0f;
float yPrev = static_cast<float>(SCREEN_HEIGHT) / 2.0f;

// Timing params
float deltaTime = 0.0f;	// time between current frame and previous frame
float prevFrame = 0.0f;

#define TEAPOT_MODEL "models/teapot_smooth.obj"
#define DONUT_MODEL "models/donut.obj"
#define SPHERE_MODEL "models/sphere.obj"
#define MONKEY_MODEL "models/suzanne_monkey.obj"

// Camera specs (set later, can't call functions here)
const float cameraSpeed = 3.0f;
const float mouseSensitivity = 0.1f;
const float cameraZoom = 50.0f;
const float xPosInit = -2.0f;
const float yPosInit = 0.0f;
const float zPosInit = 10.0f;
Camera camera(glm::vec3(xPosInit, yPosInit, zPosInit));

// Material properties
enum
{
    Water = 0,
    Air = 1,
    Metal = 2,
    Plastic = 3
};

enum
{
    None = 0,
    Weak = 1,
    Strong = 2
};

float etaR = 0.8f, etaG = 0.8f, etaB = 0.8f;
float etaAll = 0.8f, etaAllPrev = 0.8f;
float F0 = 0.02f;
bool imguiPresets = false;
const char* materialOptions[] = { "Water", "Air", "Metal", "Plastic" };
int selectedMaterial = Water;  
const char* dispersionOptions[] = { "None", "Weak", "Strong" };
int selectedDispersion = None;
float dispersionAmount = 0.0f;

void updateMaterialProperties(int materialIndex) 
{
    switch (materialIndex) 
    {
    case Water: // Water
        etaR = 0.75f - dispersionAmount; 
        etaG = 0.75f; 
        etaB = 0.75f + dispersionAmount;
        F0 = 0.02f;
        break;
    case Air: // Air (almost no refraction)
        etaR = 1.0f - dispersionAmount;
        etaG = 1.0f; 
        etaB = 1.0f + dispersionAmount;
        F0 = 0.01f;
        break;
    case Metal: // Metal (high reflectance, no refraction)
        etaR = 0.0f; 
        etaG = 0.0f; 
        etaB = 0.0f;    // No refraction in metals
        F0 = 1.0f;      // Strong reflection
        break;
    case Plastic: // Plastic
        etaR = 0.95f - dispersionAmount;
        etaG = 0.95f; 
        etaB = 0.95f + dispersionAmount;
        F0 = 0.05f;
        break;
    default:
        etaR = 0.8f - dispersionAmount;
        etaG = 0.8f; 
        etaB = 0.8f + dispersionAmount;
        F0 = 0.02f;
        break;
    }
}

void updateDispersionStrength(int strengthIndex)
{
    switch (strengthIndex)
    {
    case None: 
        dispersionAmount = 0.0f;
        break;
    case Weak:
        dispersionAmount = 0.01f;
        break;
    case Strong:
        dispersionAmount = 0.05f;
        break;
    default:
        dispersionAmount = 0.0f;
        break;
    }
}

// Setup IMGUI
void drawIMGUIWindow()
{
    ImGui::SetNextWindowCollapsed(!imguiMouseUse);
    ImGui::SetNextWindowSize(ImVec2(500, 400));
    ImGui::Begin("IMGUI");
    ImGui::Checkbox("Presets", &imguiPresets);

    // Check which GUI to draw
    if (!imguiPresets)
    {
        ImGui::Text("Adjust Eta Channels:");
        ImGui::SliderFloat("Eta Red", &etaR, 0.0f, 1.0f);
        ImGui::SliderFloat("Eta Green", &etaG, 0.0f, 1.0f);
        ImGui::SliderFloat("Eta Blue", &etaB, 0.0f, 1.0f);
        ImGui::SliderFloat("F0", &F0, 0.0f, 1.0f);
        ImGui::Text("Set All Eta Values:");
        ImGui::SliderFloat("Eta All", &etaAll, 0.0f, 1.0f);
    }
    else
    {
        // Dropdown menu for material selection
        ImGui::Text("Select Material:");
        ImGui::Combo("Material", &selectedMaterial, materialOptions, IM_ARRAYSIZE(materialOptions));
        updateMaterialProperties(selectedMaterial);
        ImGui::Combo("Dispersion Strength", &selectedDispersion, dispersionOptions, IM_ARRAYSIZE(dispersionOptions));
        updateDispersionStrength(selectedDispersion);
        ImGui::Text("Current values:");
        std::string F0Str = "F0: " + std::to_string(F0);
        ImGui::Text(F0Str.c_str());
        std::string etaRedStr = "Eta Red: " + std::to_string(etaR);
        ImGui::Text(etaRedStr.c_str());
        std::string etaGreenStr = "Eta Green: " + std::to_string(etaG);
        ImGui::Text(etaGreenStr.c_str());
        std::string etaBlueStr = "Eta Blue: " + std::to_string(etaB);
        ImGui::Text(etaBlueStr.c_str());
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Main function
int main()
{
    // glfw init and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, NULL); // Remove title bar

    // Screen params
    GLFWmonitor* MyMonitor = glfwGetPrimaryMonitor(); 
    const GLFWvidmode* mode = glfwGetVideoMode(MyMonitor);
    SCREEN_WIDTH = mode->width; SCREEN_HEIGHT = mode->height;

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Realtime Rendering Assign1", glfwGetPrimaryMonitor(), nullptr);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Callback functions
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Mouse capture (start with cursor diabled and ImGUI hidden)
    if (imguiMouseUse)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load all OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);    // Depth-testing
    glDepthFunc(GL_LESS);       // Smaller value as "closer" for depth-testing

    // Build and compile shaders
    Shader skyboxShader("shaders/skyboxShader.vs", "shaders/skyboxShader.fs");
    Shader refractionShader("shaders/refractionShader.vs", "shaders/refractionShader.fs");

    // Load models
    Model teapotModel(TEAPOT_MODEL);
    Model donutModel(DONUT_MODEL);
    Model sphereModel(SPHERE_MODEL);
    Model monkeyModel(MONKEY_MODEL);

    // Fine tune camera params
    camera.setMouseSensitivity(mouseSensitivity);
    camera.setCameraMovementSpeed(cameraSpeed);
    camera.setZoom(cameraZoom);
    camera.setFPSCamera(false, yPosInit);
    camera.setZoomEnabled(false);

    // IMGUI setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set font
    io.Fonts->Clear();
    ImFont* myFont = io.Fonts->AddFontFromFileTTF(
        "C:\\fonts\\Open_Sans\\static\\OpenSans_Condensed-Regular.ttf", 30.0f); // Adjust size here

    // Rebuild the font atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Setup skybox VAO
    GLuint skyboxVAO = setupSkyboxVAO();

    std::vector<std::string> facesCubemap =
    {
        "skybox/right.png",    // px
        "skybox/left.png",     // nx
        "skybox/top.png",      // py
        "skybox/bottom.png",   // ny
        "skybox/front.png",    // pz
        "skybox/back.png"      // nz
    };

    GLuint cubemapTexture = loadCubemap(facesCubemap);

    // Render loop
    float elapsedTime = 0.0f;
    float rotY = 0.0f; 
    float distApart = 2.8f;
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - prevFrame;
        elapsedTime += deltaTime;
        prevFrame = currentFrame;

        // User input handling
        processUserInput(window);

        // Clear screen colour and buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // IMGUI window
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Skybox
        glDisable(GL_DEPTH_TEST);
        skyboxShader.use();

        // Remove translation component from the view matrix for the skybox
        glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix()));
        skyboxShader.setMat4("view", view);
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
            static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 1000.0f);
        skyboxShader.setMat4("projection", projection);

        // Bind the skybox texture and render
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        skyboxShader.setInt("skybox", 0);

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        // Rotate the model slowly around the y axis at 20 degrees per second
        rotY += 20.0f * deltaTime;
        rotY = fmodf(rotY, 360.0f);

        // Draw models with refraction shader
        refractionShader.use();

        // If eta all changed, then change all separates to be the same
        if (etaAll != etaAllPrev)
        {
            etaR = etaG = etaB = etaAll;
            etaAllPrev = etaAll;
        }

        // Model, View & Projection transformations, set uniforms in modelShader
        view = camera.getViewMatrix();
        refractionShader.setFloat("etaR", etaR);
        refractionShader.setFloat("etaG", etaG);
        refractionShader.setFloat("etaB", etaB);
        refractionShader.setFloat("F0", F0);
        refractionShader.setMat4("view", view);
        refractionShader.setMat4("projection", projection);
        glm::mat4 inverseProjection = glm::inverse(projection);
        refractionShader.setMat4("inverseProjection", inverseProjection);
        refractionShader.setInt("skybox", 0);

        // Teapot
        glm::vec3 modelPosition = glm::vec3(-distApart, distApart, 0.0f);
        glm::mat4 model = glm::identity<glm::mat4>();
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        refractionShader.setMat4("model", model);
        teapotModel.draw(refractionShader);

        // Sphere
        modelPosition = glm::vec3(distApart, distApart, 0.0f);
        model = glm::identity<glm::mat4>();
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        refractionShader.setMat4("model", model);
        sphereModel.draw(refractionShader);

        // Donut
        modelPosition = glm::vec3(-distApart, -distApart, 0.0f);
        model = glm::identity<glm::mat4>();
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        refractionShader.setMat4("model", model);
        donutModel.draw(refractionShader);

        // Monkey
        modelPosition = glm::vec3(distApart, -distApart, 0.0f);
        model = glm::identity<glm::mat4>();
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        refractionShader.setMat4("model", model);
        monkeyModel.draw(refractionShader);

        // IMGUI drawing
        drawIMGUIWindow();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Shutdown procedure
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Destroy window
    glfwDestroyWindow(window);

    // Terminate and return success
    glfwTerminate();
    return 0;
}

// Process keyboard inputs
bool IKeyReleased = true;
void processUserInput(GLFWwindow* window)
{
    // Escape to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD to move, parse to camera processing commands
    // Positional constraints implemented in camera class
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_D, deltaTime);

    // QE for up/down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_Q, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_E, deltaTime);

    // Change mouse control between ImGUI and OpenGL
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && IKeyReleased)
    {
        IKeyReleased = false;

        // Give control to ImGUI
        if (!imguiMouseUse)
        {
            imguiMouseUse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show cursor
        }
        // Give control to OpenGL
        else
        {
            imguiMouseUse = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide cursor
        }
    }

    // Debouncer for I key
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE)
        IKeyReleased = true;
}

// Window size change callback
void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Ensure viewport matches new window dimensions
    glViewport(0, 0, width, height);

    // Adjust screen width and height params that set the aspect ratio in the projection matrix
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

// Mouse input callback
void mouseCallback(GLFWwindow* window, double xIn, double yIn)
{
    // If using ImGUI
    if (imguiMouseUse)
    {
        xPrev = static_cast<float>(xIn);
        yPrev = static_cast<float>(yIn);
        return;
    }

    // Cast doubles to floats
    float x = static_cast<float>(xIn);
    float y = static_cast<float>(yIn);

    // Check if first time this callback function is being used, set last variables if so
    if (firstMouse)
    {
        xPrev = x;
        yPrev = y;
        firstMouse = false;
    }

    // Compute offsets relative to last positions
    float xOff = x - xPrev;
    // Reverse since y-coordinates are inverted (bottom to top)
    float yOff = yPrev - y; 
    xPrev = x; yPrev = y;

    // Tell camera to process new mouse offsets
    camera.processMouseMovement(xOff, yOff);
}

// Mouse scroll wheel input callback - camera zoom must be enabled for this to work
void scrollCallback(GLFWwindow* window, double xOff, double yOff)
{
    // Tell camera to process new y-offset from mouse scroll whell
    camera.processMouseScroll(static_cast<float>(yOff));
}
// Created by Andrea Pulita

// --------------------INLCUDE SECTION---------------------

// we include the libraries for the application
#include <string>
#include <chrono>
#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glfw/glfw3.h>

#ifdef _WINDOWS_
#error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders and to load models
#include <utils/shader.h>
#include <utils/model.h>
#include <utils/camera.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// classes developed for this project
#include <my_structs/halfedgedata.h>
#include <my_structs/simplification.h>
#include <my_structs/line.h>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// ---------------------FUNCTIONS DECLARATION SECTION---------------------

// callback functions for keyboard events
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void apply_camera_movements();
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
// functions for the menu application
void show_menu();
void createModel(int model);
// the name of the subroutines are searched in the shaders, and placed in the shaders vector (to allow shaders swapping)
void SetupShader(int shader_program);
// print on console the name of current shader subroutine
void PrintCurrentShader(int subroutine);
// load the 6 images from disk and create an OpenGL cubemap
GLint LoadTextureCube(string path);

// --------------------GLOBAL VARIABLES SECTION---------------------

// dimensions of application's window
GLuint screenWidth = 1200, screenHeight = 900;

// camera/mouse variables
GLfloat lastX;
GLfloat lastY;
glm::mat4 view = glm::mat4(1.0f);
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f), GL_FALSE);
bool show_cursor = false;
bool first_mouse = true;

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;
// a vector for all the shader subroutines names used and swapped in the application
vector<std::string> shaders;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;

// boolean to start/stop animated rotation on Y angle
GLboolean spinning = GL_TRUE;
// boolean to collapse one edge
GLboolean collapseedge = GL_FALSE;
// boolean to activate/deactivate wireframe rendering
bool wireframe = false;
// boolean to make the simplification animated
bool animation = false;
bool animated_simplification_ongoing = false;
// boolean to smooth the model
bool smooth_model = false;
bool current_smooth_model = false;
// boolean to simplify the model
bool simplify = false;
// boolean to ignore the error
bool ignore_error = false;

// index of the selected model
int selected_model = 0;
int current_model = 0;
// sliders for the simplification
int slider_i = 50;
int slider_e = 50;
// number of edges collapsed in the animated simplification
int animated_simplification_edges = 0;

// we create a boolean array to store the state of the keys
bool keys[1024];

// Uniforms to pass to shaders
// position of a pointlight
glm::vec3 lightPos0 = glm::vec3(5.0f, 10.0f, 10.0f);
// diffusive, specular and ambient components
GLfloat diffuseColor[] = {1.0f, 0.0f, 0.0f};
GLfloat specularColor[] = {1.0f, 1.0f, 1.0f};
GLfloat ambientColor[] = {0.1f, 0.1f, 0.1f};
// weights for the diffusive, specular and ambient components
GLfloat Kd = 0.5f;
GLfloat Ks = 0.4f;
GLfloat Ka = 0.1f;
// shininess coefficient for Phong and Blinn-Phong shaders
GLfloat shininess = 25.0f;
// roughness index for GGX shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// texture unit for the cube map
GLuint textureCube;

// structures for the models and the simplification
Model currentModel;
Mesh* currentMesh;
my_structs::HalfEdgeMesh* currentHEMesh;
my_structs::MeshSimplification_QEM* simply;

// --------------------MAIN APP---------------------
int main()
{
    // Initialization of OpenGL context using GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // we create the application's window
    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "Simplification Project", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    // the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
    
    // we create the Shader Program used for objects (which presents different subroutines we can switch)
    Shader illumination_shader = Shader("../resources/shaders/09_illumination_models.vert", "../resources/shaders/10_illumination_models.frag");
    // we parse the Shader Program to search for the number and names of the subroutines.
    // the names are placed in the shaders vector
    SetupShader(illumination_shader.Program);
    // we print on console the name of the first subroutine used
    PrintCurrentShader(current_subroutine);

    // we create the Shader Program used for the environment map
    Shader skybox_shader("../resources/shaders/17_skybox.vert", "../resources/shaders/18_skybox.frag");

    // we load the cube map (we pass the path to the folder containing the 6 views)
    textureCube = LoadTextureCube("../resources/textures/cube/lake/");

    Model cubeModel("../resources/models/cube.obj"); // used for the environment map

    // we load the current model (code of Model class is in include/utils/model.h)
    createModel(selected_model);
    // we create the half-edge data structure for the simplification algorithm
    currentHEMesh = new my_structs::HalfEdgeMesh(currentModel.meshes[0]);
    simply = new my_structs::MeshSimplification_QEM(*currentHEMesh);
    currentMesh = currentHEMesh->ConvertToMesh(smooth_model);

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);
    
    // Model and Normal transformation matrices for the objects in the scene: we set to identity
    glm::mat4 currentModelMatrix = glm::mat4(1.0f);
    glm::mat3 currentNormalMatrix = glm::mat3(1.0f);
    
    // take time to check speed with different algorithms, number of edges and different models
    //auto start = std::chrono::high_resolution_clock::now();
    //simply.SimplifyMesh(1, 100);
    //auto end = std::chrono::high_resolution_clock::now();
    //std::chrono::duration<double> elapsed = end - start;
    //std::cout << "Time taken by function: " << elapsed.count() << " seconds" << std::endl;
    
    // we create a line to show the edge to collapse
    my_structs::Line line(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0));
    line.setColor(glm::vec3(1, 1, 0));

    // Rendering loop: this code is executed at each frame
    while (!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // we set the selected model
        if(current_model != selected_model) {
            animated_simplification_ongoing = false;
            current_model = selected_model;
            createModel(selected_model);
            delete(currentHEMesh);
            delete(simply);
            currentHEMesh = new my_structs::HalfEdgeMesh(currentModel.meshes[0]);
            simply = new my_structs::MeshSimplification_QEM(*currentHEMesh);
            currentMesh = currentHEMesh->ConvertToMesh(smooth_model);
        }
        // we execute the simplification algorithm for just one edge to make the animation or we execute it without 
        if(animated_simplification_ongoing) {
            --animated_simplification_edges;
            float errorToUse = 0.000002f + (slider_e / 100.0f) * (0.5f - 0.000002f);
            if(ignore_error) {
                errorToUse = 100.0f;
            }
            bool finish = simply->SimplifyMesh(1, errorToUse);
            currentMesh = currentHEMesh->ConvertToMesh(smooth_model);
            if(animated_simplification_edges == 0 || !finish) {
                animated_simplification_ongoing = false;
            }
        } else {
            if(simplify) {
                int edgesToCollapse = (currentHEMesh->faces.size() * slider_i / 100) / 2;
                float errorToUse = 0.000002f + (slider_e / 100.0f) * (0.5f - 0.000002f);
                if(ignore_error) {
                    errorToUse = 100.0f;
                }
                simply->SimplifyMesh(edgesToCollapse, errorToUse);
                currentMesh = currentHEMesh->ConvertToMesh(smooth_model);
                simplify = false;
            }
            // we just collapse one edge if not in the animation
            if(collapseedge) {
                simply->SimplifyMesh(1, 100);
                currentMesh = currentHEMesh->ConvertToMesh(smooth_model);
                collapseedge = false;
            }
        }
        // we smooth the model if the user wants
        if(current_smooth_model != smooth_model) {
            current_smooth_model = smooth_model;
            currentMesh = currentHEMesh->ConvertToMesh(smooth_model);
        }

        // Check is an I/O event is happening
        glfwPollEvents();
        apply_camera_movements();
        view = camera.GetViewMatrix();
        
        // we start the Dear ImGui frame and show the menu
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        show_menu();
        
        // we set the cursor visibility and the input mode
        if (show_cursor) {
            ImGui::GetIO().ConfigFlags = !ImGuiConfigFlags_NoMouse;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouse;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);     
        }

        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // if animated rotation is activated, than we increment the rotation angle using delta time and the rotation speed parameter
        if (spinning)
            orientationY += (deltaTime * spin_speed);

        // -----------------RENDERING---------------------
        illumination_shader.Use();
        // we search inside the Shader Program the name of the subroutine currently selected, and we get the numerical index
        GLuint index = glGetSubroutineIndex(illumination_shader.Program, GL_FRAGMENT_SHADER, shaders[current_subroutine].c_str());
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &index);

        // we determine the position in the Shader Program of the uniform variables
        GLint pointLightLocation = glGetUniformLocation(illumination_shader.Program, "pointLightPosition");
        GLint matDiffuseLocation = glGetUniformLocation(illumination_shader.Program, "diffuseColor");
        GLint matAmbientLocation = glGetUniformLocation(illumination_shader.Program, "ambientColor");
        GLint matSpecularLocation = glGetUniformLocation(illumination_shader.Program, "specularColor");
        GLint kaLocation = glGetUniformLocation(illumination_shader.Program, "Ka");
        GLint ksLocation = glGetUniformLocation(illumination_shader.Program, "Ks");
        GLint kdLocation = glGetUniformLocation(illumination_shader.Program, "Kd");
        GLint shineLocation = glGetUniformLocation(illumination_shader.Program, "shininess");
        GLint alphaLocation = glGetUniformLocation(illumination_shader.Program, "alpha");
        GLint f0Location = glGetUniformLocation(illumination_shader.Program, "F0");

        // we assign the value to the uniform variables
        glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPos0));
        glUniform3fv(matDiffuseLocation, 1, diffuseColor); // we already have the location (line 253), we just pass another value
        glUniform3fv(matAmbientLocation, 1, ambientColor);
        glUniform3fv(matSpecularLocation, 1, specularColor);
        glUniform1f(kaLocation, Ka);
        glUniform1f(ksLocation, Ks);
        glUniform1f(kdLocation, Kd);
        glUniform1f(shineLocation, shininess);
        glUniform1f(alphaLocation, alpha);
        glUniform1f(f0Location, F0);

        currentModelMatrix = glm::mat4(1.0f);
        currentNormalMatrix = glm::mat3(1.0f);
        currentModelMatrix = glm::translate(currentModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        currentModelMatrix = glm::rotate(currentModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
        currentModelMatrix = glm::scale(currentModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
        currentNormalMatrix = glm::inverseTranspose(glm::mat3(view * currentModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(illumination_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(currentModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(illumination_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(currentNormalMatrix));
        glUniformMatrix4fv(glGetUniformLocation(illumination_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(illumination_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
        currentMesh->Draw();
        // we draw the edge to collapse
        line.setMVP(projection * view * currentModelMatrix);
        line.setPoints(simply->next_edge_to_collapse.first, simply->next_edge_to_collapse.second);
        line.draw();

        /////////////////// SKYBOX ////////////////////////////////////////////////
        // we use the cube to attach the 6 textures of the environment map.
        // we render it after all the other objects, in order to avoid the depth tests as much as possible.
        // we will set, in the vertex shader for the skybox, all the values to the maximum depth. Thus, the environment map is rendered only where there are no other objects in the image (so, only on the background).
        //Thus, we set the depth test to GL_LEQUAL, in order to let the fragments of the background pass the depth test (because they have the maximum depth possible, and the default setting is GL_LESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDepthFunc(GL_LEQUAL);
        skybox_shader.Use();
        // we activate the cube map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureCube);
         // we pass projection and view matrices to the Shader Program of the skybox
        glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        // to have the background fixed during camera movements, we have to remove the translations from the view matrix
        // thus, we consider only the top-left submatrix, and we create a new 4x4 matrix
        view = glm::mat4(glm::mat3(view)); // Remove any translation component of the view matrix
        glUniformMatrix4fv(glGetUniformLocation(skybox_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint textureLocation = glGetUniformLocation(skybox_shader.Program, "tCube");
        // we assign the value to the uniform variable
        glUniform1i(textureLocation, 0);

        // we render the cube with the environment map
        cubeModel.Draw();
        // we set again the depth test to the default operation for the next frame
        glDepthFunc(GL_LESS);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Program
    illumination_shader.Delete();
    // we close and delete the created context
    glfwTerminate();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

// --------------------FUNCTIONS IMPLEMENTATION---------------------
// The function parses the content of the Shader Program, searches for the Subroutine type names,
// the subroutines implemented for each type, print the names of the subroutines on the terminal, and add the names of
// the subroutines to the shaders vector, which is used for the shaders swapping
void SetupShader(int program)
{
    int maxSub, maxSubU, countActiveSU;
    GLchar name[256];
    int len, numCompS;

    // global parameters about the Subroutines parameters of the system
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSub);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubU);
    std::cout << "Max Subroutines:" << maxSub << " - Max Subroutine Uniforms:" << maxSubU << std::endl;

    // get the number of Subroutine uniforms (only for the Fragment shader, due to the nature of the exercise)
    // it is possible to add similar calls also for the Vertex shader
    glGetProgramStageiv(program, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &countActiveSU);

    // print info for every Subroutine uniform
    for (int i = 0; i < countActiveSU; i++)
    {
        // get the name of the Subroutine uniform (in this example, we have only one)
        glGetActiveSubroutineUniformName(program, GL_FRAGMENT_SHADER, i, 256, &len, name);
        // print index and name of the Subroutine uniform
        std::cout << "Subroutine Uniform: " << i << " - name: " << name << std::endl;

        // get the number of subroutines
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_NUM_COMPATIBLE_SUBROUTINES, &numCompS);

        // get the indices of the active subroutines info and write into the array s
        int *s = new int[numCompS];
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_COMPATIBLE_SUBROUTINES, s);
        std::cout << "Compatible Subroutines:" << std::endl;

        // for each index, get the name of the subroutines, print info, and save the name in the shaders vector
        for (int j = 0; j < numCompS; ++j)
        {
            glGetActiveSubroutineName(program, GL_FRAGMENT_SHADER, s[j], 256, &len, name);
            std::cout << "\t" << s[j] << " - " << name << "\n";
            shaders.push_back(name);
        }
        std::cout << std::endl;

        delete[] s;
    }
}

// we print on console the name of the currently used shader subroutine
void PrintCurrentShader(int subroutine)
{
    std::cout << "Current shader subroutine: " << shaders[subroutine] << std::endl;
}

// callback for keyboard events
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    GLuint new_subroutine;

    // if ESC is pressed, we close the application
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    // if P is pressed, we start/stop the animated rotation of models
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        spinning = !spinning;
    // if G is pressed, we show the cursor
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        show_cursor = true;
    }
    // if L is pressed, we activate/deactivate wireframe rendering of models
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe = !wireframe;
    // if E is pressed, we collapse one edge
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        collapseedge = !collapseedge;
    // pressing a key number, we change the shader applied to the models
    // if the key is between 1 and 9, we proceed and check if the pressed key corresponds to
    // a valid subroutine
    if ((key >= GLFW_KEY_1 && key <= GLFW_KEY_9) && action == GLFW_PRESS)
    {
        // "1" to "9" -> ASCII codes from 49 to 59
        // we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 9
        // we subtract 1 to have indices from 0 to 8
        new_subroutine = (key - '0' - 1);
        // if the new index is valid ( = there is a subroutine with that index in the shaders vector),
        // we change the value of the current_subroutine variable
        // NB: we can just check if the new index is in the range between 0 and the size of the shaders vector,
        // avoiding to use the std::find function on the vector
        if (new_subroutine < shaders.size())
        {
            current_subroutine = new_subroutine;
            PrintCurrentShader(current_subroutine);
        }
    }
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

// we apply the camera movements
void apply_camera_movements()
    {
        if(keys[GLFW_KEY_W])
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if(keys[GLFW_KEY_S])
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if(keys[GLFW_KEY_A])
            camera.ProcessKeyboard(LEFT, deltaTime);
        if(keys[GLFW_KEY_D])
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }

// callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // we move the camera view following the mouse cursor
    // we calculate the offset of the mouse cursor from the position in the last frame
    // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
    if(first_mouse)
    {
        lastX = xpos;
        lastY = ypos;
        first_mouse = false;
    }

    // offset of mouse cursor position
    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    // the new position will be the previous one for the next frame
    lastX = xpos;
    lastY = ypos;

    // we pass the offset to the Camera class instance in order to update the rendering
    if(!show_cursor)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// callback for mouse button events
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!ImGui::GetIO().WantCaptureMouse && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        show_cursor = false;
    }
}

// function that manage the menu
void show_menu() {
    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Welcome to Andrea Pulita project!");
    if (ImGui::BeginTabBar("MyTabBar"))
    {
        if (ImGui::BeginTabItem("Home"))
        {
            ImGui::SeparatorText("INTRO:");
            ImGui::Text("This project will show you the implementation \nof a mesh simplification algorithm. Change \ntab to see all the options!");
            ImGui::SeparatorText("KEYS:");
            ImGui::Text("WASD + mouse: move around the scene.\n");
            ImGui::Text("1,2,3,4: change the shader.\n");
            ImGui::Text("ESC: quit the program.\n");
            ImGui::Text("G: show mouse for menu.\n");
            ImGui::Text("L: show/hide wireframe.\n");
            ImGui::Text("E: collapse one edge.\n");
            ImGui::Text("P: start/stop rotation.\n");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Simplification"))
        {
            // Select models
            const char* names[] = {"Bunny", "Horse", "Dragon"};
            if (ImGui::Button("Select Model.."))
                ImGui::OpenPopup("my_select_popup");
            ImGui::SameLine();
            ImGui::TextUnformatted(names[selected_model]);
            if (ImGui::BeginPopup("my_select_popup"))
            {
                ImGui::SeparatorText("Models");
                for (int i = 0; i < IM_ARRAYSIZE(names); ++i)
                    if (ImGui::Selectable(names[i]))
                        selected_model = i;
                ImGui::EndPopup();
            }
            // Percentage of edge simplification
            ImGui::Text("Percentage of edge to simplify:");
            ImGui::SliderInt("% Edges to remove", &slider_i, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);

            ImGui::Text("Percentage of error acceptable (higher error, less similar):");
            ImGui::SliderInt("% Error to accept", &slider_e, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::Checkbox("Ignore error", &ignore_error);
            ImGui::Checkbox("Animation of the simplification", &animation);
            ImGui::SameLine();
            if(!animated_simplification_ongoing) {
                if (ImGui::Button("Start Simplification")) {
                    if(animation) {
                        animated_simplification_ongoing = true;
                        animated_simplification_edges = (currentHEMesh->faces.size() * slider_i / 100) / 2;
                    } else {
                        simplify = true;
                    }
                }
            }
            ImGui::Text("Number of faces: %d", currentHEMesh->faces.size());
            ImGui::Text("Number of edges: %d", currentHEMesh->edges.size());

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Graphic"))
        {
            ImGui::Text("This is the graphic tab!\nblah blah blah blah blah");
            ImGui::SliderFloat("Spin speed", &spin_speed, 0.0f, 100.0f);
            ImGui::SliderFloat("Kd", &Kd, 0.0f, 1.0f);
            ImGui::SliderFloat("Ks", &Ks, 0.0f, 1.0f);
            ImGui::SliderFloat("Ka", &Ka, 0.0f, 1.0f);
            ImGui::SliderFloat("Shininess", &shininess, 0.0f, 100.0f);
            ImGui::SliderFloat("Alpha", &alpha, 0.0f, 1.0f);
            ImGui::SliderFloat("F0", &F0, 0.0f, 1.0f);
            ImGui::ColorEdit3("Diffuse color", diffuseColor);
            ImGui::ColorEdit3("Specular color", specularColor);
            ImGui::ColorEdit3("Ambient color", ambientColor);
            ImGui::Checkbox("Wireframe", &wireframe);
            ImGui::Checkbox("Smooth model", &smooth_model);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// function to create the model based on the selected model
void createModel(int model) {
    switch (model) {
        case 0:
            currentModel = Model("../resources/models/bunny.obj");
            break;
        case 1:
            currentModel = Model("../resources/models/horse.obj");
            break;
        case 2:
            currentModel = Model("../resources/models/dragon.obj");
            break;
    }
}

// load one side of the cubemap, passing the name of the file and the side of the corresponding OpenGL cubemap
void LoadTextureCubeSide(string path, string side_image, GLuint side_name)
{
    int w, h;
    unsigned char* image;
    string fullname;

    // full name and path of the side of the cubemap
    fullname = path + side_image;
    // we load the image file
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;
    // we set the image file as one of the side of the cubemap (passed as a parameter)
    glTexImage2D(side_name, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
}

// we load the 6 images from disk and we create an OpenGL cube map
GLint LoadTextureCube(string path)
{
    GLuint textureImage;

    // we create and activate the OpenGL cubemap texture
    glGenTextures(1, &textureImage);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureImage);

    // we load and set the 6 images corresponding to the 6 views of the cubemap
    // we use as convention that the names of the 6 images are "posx, negx, posy, negy, posz, negz", placed at the path passed as parameter
    // we load the images individually and we assign them to the correct sides of the cube map
    LoadTextureCubeSide(path, std::string("posx.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    LoadTextureCubeSide(path, std::string("negx.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    LoadTextureCubeSide(path, std::string("posy.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    LoadTextureCubeSide(path, std::string("negy.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    LoadTextureCubeSide(path, std::string("posz.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    LoadTextureCubeSide(path, std::string("negz.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // we set how to consider the texture coordinates outside [0,1] range
    // in this case we have a cube map, so
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // we set the binding to 0 once we have finished
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureImage;
}
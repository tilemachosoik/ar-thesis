#include <opengl_rendering/openglrenderer.hpp>
#include <opengl_rendering/windowless_contexts.hpp>
#include <utils/utils.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/videoio.hpp>

// Corrade
#include <Corrade/Utility/Debug.h>

#include <iostream>
#include <memory>
#include <signal.h>

#include <thread>

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

namespace global {
    bool stop_video = false;
    bool applied = false;

    StreamerConfiguration config;

    void lv_sigint_handler(int)
    {
        stop_video = true;
    }

    // Shot data
    ShotData shot_data;
    SharedShotData filtered_shot_data;

    HackyData hackyData;
} // namespace global

int streamer()
{

    // Open input video file
    cv::VideoCapture input_video(global::config.input_video_url);
    if (!input_video.isOpened()) {
        std::cerr << "Could not open input video file" << std::endl;
        return -1;
    }

    // Get input video file properties
    int frame_width = input_video.get(cv::CAP_PROP_FRAME_WIDTH);
    int frame_height = input_video.get(cv::CAP_PROP_FRAME_HEIGHT);
    double fps = input_video.get(cv::CAP_PROP_FPS); // video frame rate
    std::size_t total_frames = input_video.get(cv::CAP_PROP_FRAME_COUNT); // total number of frames

    // Create output video file
    cv::VideoWriter output_video(global::config.output_video_url, cv::VideoWriter::fourcc('a', 'v', 'c', '1'), fps, cv::Size(frame_width, frame_height));
    if (!output_video.isOpened()) {
        std::cerr << "Could not create output video file" << std::endl;
        return -1;
    }

    // Initialize object for background subtraction
    cv::Ptr<cv::BackgroundSubtractor> back_sub = cv::createBackgroundSubtractorMOG2(global::config.bg_sub_history, global::config.distance_threshold, global::config.detect_shadows);

    // Set maximum number of GL contexts
    GlobalGLContexts::instance().set_max_contexts(1, 1);
    // Initialize an OpenGLRenderer object - class that is responsible for rendering graphics with OpenGL
    std::unique_ptr<OpenGLRenderer> opengl_renderer = std::make_unique<OpenGLRenderer>(global::config);

    // Create GL contexts
    get_gl_context_select_with_sleep_and_creation_check(glcontext, 20, true, opengl_renderer->get_gpu_id());
    // Initialize OpenGL resources for rendering with OpenGLRenderer
    opengl_renderer->opengl_init(global::config);

    // Read frames from input video and write to output video
    // We assume that the input video is undistorted already
    cv::Mat frame, foreground_mask;
    std::size_t frame_counter = 0;
    //int i=0;
    while (input_video.read(frame) && !global::stop_video) {
        std::cout << "\r"
                  << "Processing frame: " << frame_counter++ << "/" << total_frames << std::flush;

        // Perform background subtraction
        back_sub->apply(frame(global::config.rendering_ROI), foreground_mask);
        // Post-process foreground mask
        foreground_mask.setTo(0, foreground_mask == 127);
        cv::medianBlur(foreground_mask, foreground_mask, 3);
        cv::erode(foreground_mask, foreground_mask, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
        cv::dilate(foreground_mask, foreground_mask, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5)), cv::Point(-1, -1), 2);
        cv::erode(foreground_mask, foreground_mask, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
        // Render logos
        opengl_renderer->render(frame, foreground_mask, global::filtered_shot_data);

        // Write processed frame to the output video file
        output_video.write(frame);

        // Outputing image instead or video for fast and easy debugging purposes
        /* if(i==0){
            imwrite("frame.png", frame);
            i++;
        } */
    }

    // Clear opengl resources
    opengl_renderer->opengl_destroy();

    // release GL contexts
    release_gl_context(glcontext);

    // Release video resources (write trailer to output video file, etc)
    input_video.release();
    output_video.release();

    return 0;
}

int createGui()
{
    Filter filter;

    bool apply = false;
    bool clear = false;

    if (!glfwInit())
        return 1;

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(720, 480, "Shot Chart Renderer GUI", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    //bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Window has no title bar
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar;

        //if (show_demo_window)
            //ImGui::ShowDemoWindow(&show_demo_window);

        // Filter Selector window
        {
            ImGui::Begin("Filter Selector", NULL, window_flags);

            ImGui::SeparatorText("Stat Settings");
            if (ImGui::TreeNode("Time Window")) {
                if (ImGui::BeginTable("split", 4)) {
                    ImGui::TableNextColumn();
                    ImGui::RadioButton("1st Quarter", &filter.quarter, 1);
                    ImGui::RadioButton("1st Half", &filter.quarter, 5);
                    ImGui::TableNextColumn();
                    ImGui::RadioButton("2nd Quarter", &filter.quarter, 2);
                    ImGui::RadioButton("2nd Half", &filter.quarter, 6);
                    ImGui::TableNextColumn();
                    ImGui::RadioButton("3rd Quarter", &filter.quarter, 3);
                    ImGui::RadioButton("Whole Game", &filter.quarter, 7);
                    ImGui::TableNextColumn();
                    ImGui::RadioButton("4th Quarter", &filter.quarter, 4);
                    ImGui::EndTable();
                }
            ImGui::TreePop();
            }

            if (ImGui::TreeNode("Team")) {
                ImGui::RadioButton(global::config.teamA_name.c_str(), &filter.team, 1);
                ImGui::RadioButton(global::config.teamB_name.c_str(), &filter.team, 2);
                ImGui::TreePop();
            }

            std::string nameLabel;
            std::size_t pos;
            if(filter.team == 1) {
                if (ImGui::TreeNode("Player")) {
                    if (ImGui::BeginTable("split", 4)) {
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("Team Stats", &filter.player, 0); ImGui::TableNextColumn(); ImGui::TableNextColumn(); ImGui::TableNextColumn(); ImGui::TableNextColumn();
                        pos = global::config.teamA_player_names[0].find(" "); 
                        nameLabel = global::config.teamA_player_names[0].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[0].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 1);
                        pos = global::config.teamA_player_names[4].find(" "); 
                        nameLabel = global::config.teamA_player_names[4].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[4].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 5);
                        pos = global::config.teamA_player_names[8].find(" "); 
                        nameLabel = global::config.teamA_player_names[8].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[8].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 9);
                        ImGui::TableNextColumn();
                        pos = global::config.teamA_player_names[1].find(" "); 
                        nameLabel = global::config.teamA_player_names[1].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[1].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 2);
                        pos = global::config.teamA_player_names[5].find(" "); 
                        nameLabel = global::config.teamA_player_names[5].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[5].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 6);
                        pos = global::config.teamA_player_names[9].find(" "); 
                        nameLabel = global::config.teamA_player_names[9].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[9].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 10);
                        ImGui::TableNextColumn();
                        pos = global::config.teamA_player_names[2].find(" "); 
                        nameLabel = global::config.teamA_player_names[2].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[2].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 3);
                        pos = global::config.teamA_player_names[6].find(" "); 
                        nameLabel = global::config.teamA_player_names[6].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[6].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 7);
                        pos = global::config.teamA_player_names[10].find(" "); 
                        nameLabel = global::config.teamA_player_names[10].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[10].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 11);
                        ImGui::TableNextColumn();
                        pos = global::config.teamA_player_names[3].find(" "); 
                        nameLabel = global::config.teamA_player_names[3].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[3].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 4);
                        pos = global::config.teamA_player_names[7].find(" "); 
                        nameLabel = global::config.teamA_player_names[7].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[7].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 8);
                        pos = global::config.teamA_player_names[11].find(" "); 
                        nameLabel = global::config.teamA_player_names[11].substr(pos + 1, std::string::npos) + " " + global::config.teamA_player_names[11].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 12);
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
            } else if (filter.team == 2) {
                if (ImGui::TreeNode("Player")) {
                    if (ImGui::BeginTable("split", 4)) {
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("Team Stats", &filter.player, 0); ImGui::TableNextColumn(); ImGui::TableNextColumn(); ImGui::TableNextColumn(); ImGui::TableNextColumn();
                        pos = global::config.teamB_player_names[0].find(" "); 
                        nameLabel = global::config.teamB_player_names[0].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[0].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 1);
                        pos = global::config.teamB_player_names[4].find(" "); 
                        nameLabel = global::config.teamB_player_names[4].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[4].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 5);
                        pos = global::config.teamB_player_names[8].find(" "); 
                        nameLabel = global::config.teamB_player_names[8].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[8].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 9);
                        ImGui::TableNextColumn();
                        pos = global::config.teamB_player_names[1].find(" "); 
                        nameLabel = global::config.teamB_player_names[1].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[1].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 2);
                        pos = global::config.teamB_player_names[5].find(" "); 
                        nameLabel = global::config.teamB_player_names[5].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[5].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 6);
                        pos = global::config.teamB_player_names[9].find(" "); 
                        nameLabel = global::config.teamB_player_names[9].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[9].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 10);
                        ImGui::TableNextColumn();
                        pos = global::config.teamB_player_names[2].find(" "); 
                        nameLabel = global::config.teamB_player_names[2].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[2].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 3);
                        pos = global::config.teamB_player_names[6].find(" "); 
                        nameLabel = global::config.teamB_player_names[6].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[6].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 7);
                        pos = global::config.teamB_player_names[10].find(" "); 
                        nameLabel = global::config.teamB_player_names[10].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[10].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 11);
                        ImGui::TableNextColumn();
                        pos = global::config.teamB_player_names[3].find(" "); 
                        nameLabel = global::config.teamB_player_names[3].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[3].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 4);
                        pos = global::config.teamB_player_names[7].find(" "); 
                        nameLabel = global::config.teamB_player_names[7].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[7].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 8);
                        pos = global::config.teamB_player_names[11].find(" "); 
                        nameLabel = global::config.teamB_player_names[11].substr(pos + 1, std::string::npos) + " " + global::config.teamB_player_names[11].substr(0, 1) + ".";
                        ImGui::RadioButton(nameLabel.c_str(), &filter.player, 12);
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
            }
            if (ImGui::TreeNode("Shot Type")) {
                ImGui::RadioButton("Two Pointer", &filter.shotType, 2);
                ImGui::SameLine();
                ImGui::RadioButton("Three Pointer", &filter.shotType, 3);
                ImGui::SameLine();
                ImGui::RadioButton("All Shots", &filter.shotType, 0);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Made/Missed Shots")) {
                ImGui::RadioButton("Missed", &filter.made, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Made", &filter.made, 1);
                ImGui::SameLine();
                ImGui::RadioButton("Both", &filter.made, 2);
                ImGui::TreePop();
            }

            ImGui::SeparatorText("Display Settings");

            if (ImGui::TreeNode("Select Court Side to Print on")) {
                ImGui::RadioButton("Left", &global::hackyData.side, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Right", &global::hackyData.side, 1);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Display Options")) {
                ImGui::Checkbox("Shots", &filter.displayShots);
                ImGui::SameLine();
                ImGui::Checkbox("Stats on Court", &filter.displayCourtStats);
                ImGui::SameLine();
                ImGui::Checkbox("Regions", &filter.displayRegions);
                // ImGui::SameLine();
                // ImGui::Checkbox("Tab under basket", &filter.displayTab);
                // ImGui::SameLine();
                // ImGui::Checkbox("Logo on Center", &filter.displayLogoMiddle);
                ImGui::TreePop();
            }

            ImGui::Spacing();
            ImGui::Spacing();

            ImVec2 button_size = ImVec2{100, 0};
            float width = ImGui::GetWindowSize().x;
            float centre_position_for_button = (width - (button_size.x)) / 2;
            ImGui::SetCursorPosX(centre_position_for_button);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(2 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(2 / 7.0f, 0.8f, 0.8f));
            if (ImGui::Button("Apply", ImVec2(100.0f, 25.0f)))
                apply = true;
            ImGui::PopStyleColor(3);
            
            button_size = ImVec2{100, 0};
            width = ImGui::GetWindowSize().x;
            centre_position_for_button = (width - (button_size.x)) / 2;
            ImGui::SetCursorPosX(centre_position_for_button);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(7 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(7 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(7 / 7.0f, 0.8f, 0.8f));
            if (ImGui::Button("Clear", ImVec2(100.0f, 25.0f)))
                clear = true;
            ImGui::PopStyleColor(3);

            if (apply) // Getting filtered_shot_data using filters
            {
                global::filtered_shot_data.mutex.lock();
                global::hackyData.timePeriod = filter.quarter;
                global::hackyData.displayTab = filter.displayTab;
                global::hackyData.displayShots = filter.displayShots;
                global::hackyData.displayCourtStats = filter.displayCourtStats;
                global::hackyData.displayLogoMiddle = filter.displayLogoMiddle;
                global::hackyData.displayRegions = filter.displayRegions;
                global::hackyData.team = filter.team;
                global::hackyData.player = filter.player;
                global::hackyData.shotType = filter.shotType;
                std::vector<ShotDataEntry> filtered_data = filter_shot_data(global::shot_data, filter);
                Stats stats = get_stats(global::shot_data, filter);
                global::filtered_shot_data.shot_data = filtered_data;
                global::filtered_shot_data.stats = stats;
                global::filtered_shot_data.updated.store(true);
                global::filtered_shot_data.mutex.unlock();
                apply = false;
            }
            else if (clear) {
                global::filtered_shot_data.mutex.lock();
                global::hackyData.reset();
                global::filtered_shot_data.shot_data.clear();
                global::filtered_shot_data.stats.reset();
                global::filtered_shot_data.updated.store(true);
                global::filtered_shot_data.mutex.unlock();
                clear = false;
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

int main(int argc, char** argv)
{
    // 2 threads, one for the GUI and one for video-processing
    std::thread GUIThread;
    std::thread streamerThread;

    Corrade::Utility::Debug magnum_silence_output{nullptr};

    // Handling ctrl-c - This enables graceful termination of main function by pressing Ctrl+C
    // When Ctrl+C is pressed, the function lv_sigint_handler is called
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = global::lv_sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGKILL, &sigIntHandler, NULL);

    // Read configuration file
    std::string config_file = "config.yml";
    if (argc >= 2) {
        config_file = std::string(argv[1]);
    }
    global::config = read_config_file(config_file);
    global::shot_data = load_shot_data(global::config.data_url);

    streamerThread = std::thread(streamer);
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    GUIThread = std::thread(createGui);
    GUIThread.join();
    streamerThread.join();

    return 0;
}
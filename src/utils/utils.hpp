#ifndef UTILS_UTILS_HPP
#define UTILS_UTILS_HPP

#include <ryml_std.hpp>
// order matters here
#include <ryml.hpp>

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <atomic>

struct LogoData {
    std::string path = "";
    cv::Mat transformation;
};

struct ShotChartData {
    cv::Mat transformation;
    double x, y;
    int made;
    int region;
};

struct ShotDataEntry {
    int minute;
    std::string teamId;
    std::string playerId;
    double xPos;
    double yPos;
    std::string shotType;
    int made;
};

using ShotData = std::vector<ShotDataEntry>;

struct Filter {
    int timeWindowBegin = 1;
    int timeWindowEnd = 10;
    int quarter = 1;
    int team = 1;
    int player = 0;
    int shotType = 0;
    int made = 2;
    bool displayShots = false;
    bool displayCourtStats = false;
    bool displayRegions = false;
    bool displayTab = false;
    bool displayLogoMiddle = false;
};

struct Stats {
    double made2p = 0.;
    double total2p = 0.;
    double made3p = 0.;
    double total3p = 0.;
    double percentage_2p = 0.;
    double percentage_3p = 0.;
    char formated_percentage_2p[6];
    char formated_percentage_3p[6];
    std::string label_2p;
    std::string label_3p;
    void reset()
    {
        made2p = 0.;
        total2p = 0.;
        made3p = 0.;
        total3p = 0.;
        percentage_2p = 0.;
        percentage_3p = 0.;
    }
};

struct SharedShotData {
    ShotData shot_data;
    std::mutex mutex;
    std::atomic<bool> updated{false};
    Stats stats;
};

struct Logos {
    cv::Mat teamA;
    cv::Mat teamB;
    cv::Mat alpha_teamA;
    cv::Mat alpha_teamB;

    std::vector<cv::Mat> teamA_player;
    std::vector<cv::Mat> teamB_player;
};

struct HackyData {
    int team = 0;
    int side = 0;
    int player = 0;
    int shotType = 0;
    int timePeriod = 0;
    bool displayShots = false;
    bool displayCourtStats = false;
    bool displayRegions = false;
    bool displayTab = false;
    bool displayLogoMiddle = false;

    void reset()
    {
        team = 0;
        side = 0;
        player = 0;
        shotType = 0;
        timePeriod = 0;
        displayShots = false;
        displayCourtStats = false;
        displayRegions = false;
        displayTab = false;
        displayLogoMiddle = false;
    }
};

struct Point_3d {
    double x, y;
    Point_3d(double xPos, double yPos) 
    {
        x = xPos;
        y = yPos;
    }
};

struct Edge {
    double x1, x2, y1, y2;
    void setPoints(double xa, double ya, double xb, double yb)
    {
        x1 = xa;
        y1 = ya;
        x2 = xb;
        y2 = yb;
    }
};

struct Polygon {
    std::vector<Edge> edges;
};

struct Region {
    int made = 0;
    int total = 0;
    void clear() {
        made = 0;
        total = 0;
    }
};

struct StreamerConfiguration {
    std::string input_video_url = "";
    std::string output_video_url = "";
    std::string data_url = "";
    // Shots
    std::string green_circle_url = "";
    std::string red_x_url = "";

    std::string black_dot_url = "";

    // Calibration
    std::string camera_type = "fisheye";
    std::string calibration_path = "lv_calib_full.npz";
    // Background Subtraction
    cv::Rect rendering_ROI = cv::Rect(1125, 852, 1685, 396);
    std::size_t bg_sub_history = 1000;
    double distance_threshold = 50.;
    bool detect_shadows = true;
    // OpenGL Rendering
    std::vector<LogoData> logos;
    std::vector<ShotChartData> shots;
    std::size_t gpu_id = 0;
    // Logos
    cv::Mat logo_teamA;
    cv::Mat logo_teamB;
    std::vector<cv::Mat> logo_teamA_players;
    std::vector<cv::Mat> logo_teamB_players;
    // Names
    std::string teamA_name;
    std::string teamB_name;
    std::vector<std::string> teamA_player_names;
    std::vector<std::string> teamB_player_names;
};

StreamerConfiguration read_config_file(const std::string& filename);
ShotData load_shot_data(std::string data_url);
ShotData filter_shot_data(const std::vector<ShotDataEntry>& data, const Filter& filter);
Stats get_stats(const std::vector<ShotDataEntry>& data, const Filter& filter);

inline std::string _get_str_val(const c4::yml::NodeRef& c)
{
    std::string n;
    c4::from_chars(c.val(), &n);
    if (n.find("env://") == 0) {
        char* env = std::getenv(n.substr(6).c_str());
        if (env != nullptr)
            n = env;
        else
            return "";
    }
    return n;
}

template <typename T>
T get_value(const c4::yml::NodeRef& c)
{
    std::string n = _get_str_val(c);

    std::stringstream ss(n);
    T val;
    ss >> val;

    return val;
}

cv::Rect calculate_fit_ROI(const cv::Mat& src, const cv::Rect& dst_roi, std::string align_x, std::string align_y);

cv::Mat read_image(const std::string& image_path, bool with_alpha);

void resize_image(const cv::Mat& src, cv::Mat& dest, const cv::Size& new_size);

void overlay_image(const cv::Mat& src, cv::Mat& dest, const cv::Mat& src_alpha_channel, bool blend, double alpha);

void place_image_into_ROI(const cv::Mat& src, cv::Mat& dest, const cv::Rect& dst_roi, const cv::Mat& src_alpha_channel, std::string align_x, std::string align_y, bool blend, double alpha);

void split_alpha_from_color_image(const cv::Mat& src, cv::Mat& color_image, cv::Mat& alpha);

void draw_text(cv::Mat& dest, const std::string& text, cv::Point origin, int max_text_width, cv::Ptr<cv::freetype::FreeType2>& font, int font_size, cv::Scalar color, std::string text_align = "center");

int is_inside(Polygon polygon, double xp, double yp);

#endif

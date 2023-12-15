#include "utils.hpp"

#include <opencv2/calib3d.hpp>

// std headers
#include <filesystem>
namespace fs = std::filesystem;

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <signal.h>
#include <thread>
#include <unordered_map>
#include <utility>

template <>
bool get_value<bool>(const c4::yml::NodeRef& c)
{
    std::string n = _get_str_val(c);

    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    std::stringstream ss(n);
    bool val;
    ss >> std::boolalpha >> val;

    return val;
}

template <>
std::string get_value<std::string>(const c4::yml::NodeRef& c)
{
    return _get_str_val(c);
}

inline void read_logo_data(const c4::yml::NodeRef& c, LogoData& logo)
{
    double x = 0., y = 0., z = 0.;
    double qx = 0., qy = 0., qz = 0.;
    double sx = 1., sy = 1., sz = 1.;
    for (auto c1 : c.children()) {
        if (c1.key() == "path") {
            logo.path = get_value<std::string>(c1);
        }
        else if (c1.key() == "translation") {
            std::size_t idx = 0;
            for (auto c2 : c1.children()) {
                if (idx == 0) {
                    x = get_value<double>(c2);
                }
                else if (idx == 1) {
                    y = get_value<double>(c2);
                }
                else if (idx == 2) {
                    z = get_value<double>(c2);
                }
                idx++;
                if (idx >= 3)
                    break;
            }
        }
        else if (c1.key() == "orientation") {
            std::size_t idx = 0;
            for (auto c2 : c1.children()) {
                if (idx == 0) {
                    qx = get_value<double>(c2);
                }
                else if (idx == 1) {
                    qy = get_value<double>(c2);
                }
                else if (idx == 2) {
                    qz = get_value<double>(c2);
                }
                idx++;
                if (idx >= 3)
                    break;
            }
        }
        else if (c1.key() == "scaling") {
            std::size_t idx = 0;
            for (auto c2 : c1.children()) {
                if (idx == 0) {
                    sx = get_value<double>(c2);
                }
                else if (idx == 1) {
                    sy = get_value<double>(c2);
                }
                else if (idx == 2) {
                    sz = get_value<double>(c2);
                }
                idx++;
                if (idx >= 3)
                    break;
            }
        }
    }

    cv::Mat r = (cv::Mat_<double>(3, 1) << qx, qy, qz);
    cv::Mat t = (cv::Mat_<double>(3, 1) << x, y, z);
    cv::Mat R;
    cv::Rodrigues(r, R);

    logo.transformation = cv::Mat::eye(4, 4, R.type()); // T is 4x4
    logo.transformation(cv::Range(0, 3), cv::Range(0, 3)) = R * 1; // copies R into T
    logo.transformation(cv::Range(0, 3), cv::Range(3, 4)) = t * 1; // copies tvec into T

    cv::Mat scaling = cv::Mat::eye(4, 4, R.type());
    scaling.at<double>(0, 0) = sx;
    scaling.at<double>(1, 1) = sy;
    scaling.at<double>(2, 2) = sz;

    logo.transformation = logo.transformation * scaling;
}

// Function for getting needed data from .csv file
/* timeWindowBegin and timeWindowEnd are in minutes (1-40), team is 1/2 or 0 if both, player is 1-12 or 0 if all,
   shotType is 1-3 or 0 if all, made is 0/1, or 2 if both */
ShotData load_shot_data(std::string data_url)
{
    // Preparing Arrays of Structs
    std::vector<ShotDataEntry> data;
    // std::vector<Data> trueData;

    std::ifstream data_file;
    data_file.open(data_url, std::ios::in); // Opening the file

    std::string line;
    ShotDataEntry tempData;

    // Getting all the data from the .csv file
    while (getline(data_file, line) && !line.empty()) {

        std::stringstream mystream(line);
        std::string temp;

        std::getline(mystream, temp, ',');
        tempData.minute = std::stoi(temp);
        std::getline(mystream, tempData.teamId, ',');
        std::getline(mystream, tempData.playerId, ',');
        std::getline(mystream, temp, ',');
        tempData.xPos = (double)std::stoi(temp);
        std::getline(mystream, temp, ',');
        tempData.yPos = (double)std::stoi(temp);
        std::getline(mystream, tempData.shotType, ',');
        std::getline(mystream, temp, ',');
        tempData.made = std::stoi(temp);

        data.push_back(tempData);
    }

    // Filtering data into trueData

    return data;
}

ShotData filter_shot_data(const std::vector<ShotDataEntry>& data, const Filter& filter)
{
    std::vector<ShotDataEntry> filtered_data;
    Filter filter2;

    if (filter.quarter == 1) {
        filter2.timeWindowBegin = 1;
        filter2.timeWindowEnd = 10;
    }
    else if (filter.quarter == 2) {
        filter2.timeWindowBegin = 11;
        filter2.timeWindowEnd = 20;
    }
    else if (filter.quarter == 3) {
        filter2.timeWindowBegin = 21;
        filter2.timeWindowEnd = 30;
    }
    else if (filter.quarter == 4) {
        filter2.timeWindowBegin = 31;
        filter2.timeWindowEnd = 40;
    }
    else if (filter.quarter == 5) {
        filter2.timeWindowBegin = 1;
        filter2.timeWindowEnd = 20;
    }
    else if (filter.quarter == 6) {
        filter2.timeWindowBegin = 21;
        filter2.timeWindowEnd = 40;
    }
     else if (filter.quarter == 7) {
        filter2.timeWindowBegin = 1;
        filter2.timeWindowEnd = 40;
    }

    for (std::size_t i = 0; i < data.size(); i++) {
        if (data[i].minute >= filter2.timeWindowBegin && data[i].minute <= filter2.timeWindowEnd) {
            if (filter.team == 0 || (filter.team == 1 && data[i].teamId == "A") || (filter.team == 2 && data[i].teamId == "B")) {
                if (filter.player == 0 || (filter.player == 1 && data[i].playerId.compare("P1") == 0) || (filter.player == 2 && data[i].playerId.compare("P2") == 0) || (filter.player == 3 && data[i].playerId.compare("P3") == 0) || (filter.player == 4 && data[i].playerId.compare("P4") == 0) || (filter.player == 5 && data[i].playerId.compare("P5") == 0) || (filter.player == 6 && data[i].playerId.compare("P6") == 0) || (filter.player == 7 && data[i].playerId.compare("P7") == 0) || (filter.player == 8 && data[i].playerId.compare("P8") == 0) || (filter.player == 9 && data[i].playerId.compare("P9") == 0) || (filter.player == 10 && data[i].playerId.compare("P10") == 0) || (filter.player == 11 && data[i].playerId.compare("P11") == 0) || (filter.player == 12 && data[i].playerId.compare("P12") == 0)) {
                    if ((filter.shotType == 0 && !(data[i].shotType.compare("1p") == 0)) || (filter.shotType == 2 && data[i].shotType.compare("2p") == 0) || (filter.shotType == 3 && data[i].shotType.compare("3p") == 0)) {
                        if (filter.made == 2 || (filter.made == 0 && data[i].made == 0) || (filter.made == 1 && data[i].made == 1)) {
                            filtered_data.push_back(data[i]);
                        }
                    }
                }
            }
        }
    }
    return filtered_data;
}

Stats get_stats(const std::vector<ShotDataEntry>& data, const Filter& filter) {
    Stats stats;
    Filter filter2;

    if (filter.quarter == 1) {
        filter2.timeWindowBegin = 1;
        filter2.timeWindowEnd = 10;
    }
    else if (filter.quarter == 2) {
        filter2.timeWindowBegin = 11;
        filter2.timeWindowEnd = 20;
    }
    else if (filter.quarter == 3) {
        filter2.timeWindowBegin = 21;
        filter2.timeWindowEnd = 30;
    }
    else if (filter.quarter == 4) {
        filter2.timeWindowBegin = 31;
        filter2.timeWindowEnd = 40;
    }
    else if (filter.quarter == 5) {
        filter2.timeWindowBegin = 1;
        filter2.timeWindowEnd = 20;
    }
    else if (filter.quarter == 6) {
        filter2.timeWindowBegin = 21;
        filter2.timeWindowEnd = 40;
    }

    for (std::size_t i = 0; i < data.size(); i++) {
        if (data[i].minute >= filter2.timeWindowBegin && data[i].minute <= filter2.timeWindowEnd) {
            if (filter.team == 0 || (filter.team == 1 && data[i].teamId == "A") || (filter.team == 2 && data[i].teamId == "B")) {
                if (filter.player == 0 || (filter.player == 1 && data[i].playerId.compare("P1") == 0) || (filter.player == 2 && data[i].playerId.compare("P2") == 0) || (filter.player == 3 && data[i].playerId.compare("P3") == 0) || (filter.player == 4 && data[i].playerId.compare("P4") == 0) || (filter.player == 5 && data[i].playerId.compare("P5") == 0) || (filter.player == 6 && data[i].playerId.compare("P6") == 0) || (filter.player == 7 && data[i].playerId.compare("P7") == 0) || (filter.player == 8 && data[i].playerId.compare("P8") == 0) || (filter.player == 9 && data[i].playerId.compare("P9") == 0) || (filter.player == 10 && data[i].playerId.compare("P10") == 0) || (filter.player == 11 && data[i].playerId.compare("P11") == 0) || (filter.player == 12 && data[i].playerId.compare("P12") == 0)) {
                    if ((filter.shotType == 0 && !(data[i].shotType.compare("1p") == 0)) || (filter.shotType == 2 && data[i].shotType.compare("2p") == 0) || (filter.shotType == 3 && data[i].shotType.compare("3p") == 0)) {
                        if ((data[i].shotType.compare("2p") == 0) && (data[i].made == 1)) {
                            stats.made2p++;
                            stats.total2p++;
                        }
                        else if ((data[i].shotType.compare("2p") == 0) && (data[i].made == 0)) {
                            stats.total2p++;
                        }
                        else if ((data[i].shotType.compare("3p") == 0) && (data[i].made == 1)) {
                            stats.made3p++;
                            stats.total3p++;
                        }
                        else if ((data[i].shotType.compare("3p") == 0) && (data[i].made == 0)) {
                            stats.total3p++;
                        }
                    }
                }
            }
        }
    }
    
    return stats;
}

StreamerConfiguration read_config_file(const std::string& filename)
{
    StreamerConfiguration config;

    // First load file contents to buffer
    std::ifstream t(fs::absolute(filename));
    if (!t.is_open()) { // Error in reading, return empty vector
        std::cerr << "Error reading input configuration file. Return default configuration" << std::endl;
        return config;
    }
    t.seekg(0, std::ios::end);
    std::size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);

    // Read yaml file to Tree structure
    ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(buffer));
    ryml::NodeRef r = tree.rootref();

    try {

        for (auto c : r.children()) {
            if (c.key() == "input_video_url") {
                config.input_video_url = get_value<std::string>(c);
            }
            else if (c.key() == "output_video_url") {
                config.output_video_url = get_value<std::string>(c);
            }
            else if (c.key() == "data_url") {
                config.data_url = get_value<std::string>(c);
            }
            else if (c.key() == "background_subtraction") {
                for (auto c1 : c.children()) {
                    if (c1.key() == "frame_ROI") {
                        std::size_t idx = 0;
                        for (auto c2 : c1.children()) {
                            if (idx == 0) {
                                config.rendering_ROI.x = get_value<int>(c2);
                            }
                            else if (idx == 1) {
                                config.rendering_ROI.y = get_value<int>(c2);
                            }
                            else if (idx == 2) {
                                config.rendering_ROI.width = get_value<int>(c2);
                            }
                            else if (idx == 3) {
                                config.rendering_ROI.height = get_value<int>(c2);
                            }
                            idx++;
                            if (idx >= 4)
                                break;
                        }
                    }
                    else if (c1.key() == "bg_sub_history") {
                        config.bg_sub_history = get_value<int>(c1);
                    }
                    else if (c1.key() == "distance_threshold") {
                        config.distance_threshold = get_value<double>(c1);
                    }
                    else if (c1.key() == "detect_shadows") {
                        config.detect_shadows = get_value<bool>(c1);
                    }
                }
            }
            else if (c.key() == "opengl_rendering") {
                for (auto c1 : c.children()) {
                    if (c1.key() == "gpu_id") {
                        config.gpu_id = get_value<int>(c1);
                    }
                    else if (c1.key() == "shots") {
                        for (auto c2 : c1.children()) {
                            if (c2.key() == "green_circle_path") {
                                config.green_circle_url = get_value<std::string>(c2);
                            }
                            else if (c2.key() == "red_x_path") {
                                config.red_x_url = get_value<std::string>(c2);
                            }
                            else if (c2.key() == "black_dot_path") {
                                config.black_dot_url = get_value<std::string>(c2);
                            }
                        }
                    }
                    else if (c1.key() == "logos") {
                        config.logos.clear();
                        for (auto c2 : c1.children()) {
                            LogoData logo;
                            read_logo_data(c2, logo);
                            config.logos.push_back(logo);
                        }
                    }
                    else if (c1.key() == "names") {
                        for (auto c2 : c1.children()) {
                            if (c2.key() == "teamA") {
                                // config.teamA_name = get_value<std::string>(c2);
                                config.teamA_name = get_value<std::string>(c2);
                            }
                            else if (c2.key() == "teamB") {
                                config.teamB_name = get_value<std::string>(c2);
                            }
                            else if (c2.key() == "teamA_players") {
                                for (auto c3 : c2.children()) {
                                    // std::cout << get_value<std::string>(c3) << std::endl;
                                    config.teamA_player_names.push_back(get_value<std::string>(c3));
                                }
                            }
                            else if (c2.key() == "teamB_players") {
                                for (auto c3 : c2.children()) {
                                    config.teamB_player_names.push_back(get_value<std::string>(c3));
                                }
                            }
                        }
                    }
                    else if (c1.key() == "tab_logos") {
                        for (auto c2 : c1.children()) {
                            if (c2.key() == "logo_teamA") {
                                config.logo_teamA = cv::imread(get_value<std::string>(c2), cv::IMREAD_UNCHANGED);
                            }
                            else if (c2.key() == "logo_teamB") {
                                config.logo_teamB = cv::imread(get_value<std::string>(c2), cv::IMREAD_UNCHANGED);
                            }
                            else if (c2.key() == "logo_teamA_players") {
                                for (auto c3 : c2.children()) {
                                    cv::Mat templogo;
                                    if (get_value<std::string>(c3) != "") {
                                        templogo = cv::imread(get_value<std::string>(c3), cv::IMREAD_UNCHANGED);
                                    }
                                    config.logo_teamA_players.push_back(templogo);
                                }
                            }
                            else if (c2.key() == "logo_teamB_players") {
                                for (auto c3 : c2.children()) {
                                    cv::Mat templogo;
                                    if (get_value<std::string>(c3) != "") {
                                        templogo = cv::imread(get_value<std::string>(c3), cv::IMREAD_UNCHANGED);
                                    }
                                    config.logo_teamB_players.push_back(templogo);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (...) {
        // Error in reading, return empty vector
        std::cerr << "Error reading input configuration file. Return default configuration" << std::endl;
    }
    return config;
}

cv::Rect calculate_fit_ROI(const cv::Mat& src, const cv::Rect& dst_roi, std::string align_x, std::string align_y)
{
    double src_ratio = static_cast<double>(src.cols) / src.rows;
    double roi_ratio = static_cast<double>(dst_roi.width) / dst_roi.height;

    cv::Rect output_roi;
    if (src_ratio > roi_ratio) {
        output_roi.width = dst_roi.width;
        output_roi.height = output_roi.width / src_ratio;
        output_roi.x = dst_roi.x;
        output_roi.y = (dst_roi.y + dst_roi.height / 2.) - (output_roi.height / 2.); // default align_y = center
        if (align_y == "top") {
            output_roi.y = dst_roi.y;
        }
        else if (align_y == "bottom") {
            output_roi.y = (dst_roi.y + dst_roi.height) - output_roi.height;
        }
    }
    else {
        output_roi.height = dst_roi.height;
        output_roi.width = output_roi.height * src_ratio;
        output_roi.x = (dst_roi.x + dst_roi.width / 2.) - (output_roi.width / 2.); // default align_x = center
        if (align_x == "left") {
            output_roi.x = dst_roi.x;
        }
        else if (align_x == "right") {
            output_roi.x = (dst_roi.x + dst_roi.width) - output_roi.width;
        }
        output_roi.y = dst_roi.y;
    }
    return output_roi;
}

std::string extract_filename(const std::string& path, bool with_extension)
{
    std::size_t last_slash = path.find_last_of("/\\");

    std::string filename = path;
    if (last_slash != std::string::npos) {
        filename = path.substr(last_slash + 1);
    }
    if (!with_extension) {
        std::size_t last_dot = filename.find_last_of(".");

        if (last_dot != std::string::npos) {
            filename = filename.substr(0, last_dot);
        }
    }

    return filename;
}

std::string extract_file_extension(const std::string& path)
{
    std::string extension = "";
    std::size_t last_dot = path.find_last_of(".");
    if (last_dot != std::string::npos) {
        extension = path.substr(last_dot + 1);
    }

    return extension;
}

cv::Mat read_image(const std::string& image_path, bool with_alpha)
{
    // This function ensures that a cv::Mat with at least 3 channels is loaded
    cv::Mat image;

    if (fs::exists(image_path)) {
        int type = cv::IMREAD_COLOR;
        if (with_alpha) {
            type = cv::IMREAD_UNCHANGED;
        }

        image = cv::imread(image_path, type);
        if (!image.empty() && with_alpha) {
            if (image.channels() < 4) {
                if (image.channels() == 3) {
                    cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
                }
                else if (image.channels() == 1) {
                    cv::cvtColor(image, image, cv::COLOR_GRAY2BGRA);
                }
            }
        }
    }

    return image;
}

void resize_image(const cv::Mat& src, cv::Mat& dest, const cv::Size& new_size)
{
    if (!src.empty()) {
        int interpolation = cv::INTER_AREA;
        if (src.size().width < new_size.width) {
            interpolation = cv::INTER_CUBIC;
        }
        cv::resize(src, dest, new_size, 0., 0., interpolation);
    }
}

void overlay_image(const cv::Mat& src, cv::Mat& dest, const cv::Mat& src_alpha_channel, bool blend, double alpha)
{
    // src, dst and src_alpha_channel must be CV_8UC3 (3 channels). In our pipeline, this function will be applied in each frame (real time).
    // It is better to extract the alpha channel from the src image during initialization (alpha channel for logos will be the same) and not in each frame
    if ((!src.empty()) && (!dest.empty())) {
        cv::Mat blended;
        src.copyTo(blended);
        if (blend) {
            cv::addWeighted(src, alpha, dest, 1. - alpha, 0.0, blended);
        }
        if (!src_alpha_channel.empty()) {
            // std::cout << src.type() << dest.type() << src_alpha_channel.type() << std::endl;
            // std::cout << src.size() << dest.size() << src_alpha_channel.size() << std::endl;
            cv::add(blended.mul(src_alpha_channel, 1.f / 255), dest.mul(cv::Scalar(255, 255, 255) - src_alpha_channel, 1.f / 255), dest);
        }
        else {
            blended.copyTo(dest);
        }
    }
}

void place_image_into_ROI(const cv::Mat& src, cv::Mat& dest, const cv::Rect& dst_roi, const cv::Mat& src_alpha_channel, std::string align_x, std::string align_y, bool blend, double alpha)
{
    // This function fits the src image aligned into the dst_roi in the dest image.
    if ((!src.empty()) && (!dest.empty()) && (!dst_roi.empty())) {
        cv::Rect display_roi = calculate_fit_ROI(src, dst_roi, align_x, align_y); // calculate new roi to fit src into dst_roi
        cv::Mat tmp_img, tmp_img_alpha;
        if ((display_roi.width != src.cols) || (display_roi.height != src.rows)) {
            // Resize src to match the width and height of the new display_roi
            resize_image(src, tmp_img, cv::Size(display_roi.width, display_roi.height));
            // We also need to resize alpha channel (if it was previously calculated and passed to the function)
            resize_image(src_alpha_channel, tmp_img_alpha, cv::Size(display_roi.width, display_roi.height));
        }
        else {
            src.copyTo(tmp_img);
            tmp_img_alpha = src_alpha_channel;
        }

        if (tmp_img_alpha.empty()) {
            // If no alpha channel is loaded, check if there is one and extract it
            split_alpha_from_color_image(tmp_img, tmp_img, tmp_img_alpha);
        }

        cv::Mat dst = dest(display_roi);
        // Overlay image on the dest image in the display_roi (the one that the src fits into)
        overlay_image(tmp_img, dst, tmp_img_alpha, blend, alpha);
    }
}

void split_alpha_from_color_image(const cv::Mat& src, cv::Mat& color_image, cv::Mat& alpha)
{
    if (src.channels() == 4) {
        std::vector<cv::Mat> src_channels;
        cv::split(src, src_channels);
        alpha = src_channels.back();
        cv::cvtColor(alpha, alpha, cv::COLOR_GRAY2BGR);
        src_channels.pop_back();
        cv::merge(src_channels, color_image);
    }
    else {
        src.copyTo(color_image);
        alpha = cv::Mat();
    }
}

void draw_text(cv::Mat& dest, const std::string& text, cv::Point origin, int max_text_width, cv::Ptr<cv::freetype::FreeType2>& font, int font_size, cv::Scalar color, std::string text_align)
{
    cv::Size text_size = font->getTextSize(text, font_size, -1, 0);
    while (text_size.width > max_text_width) {
        font_size -= 8;
        text_size = font->getTextSize(text, font_size, -1, 0);
    }
    int x = (origin.x + max_text_width / 2.) - text_size.width / 2.;
    if (text_align == "left") {
        x = origin.x;
    }
    else if (text_align == "right") {
        x = (origin.x + max_text_width) - text_size.width;
    }
    cv::Point p(x, origin.y - 0.28 * font_size);
    font->putText(dest, text, p, font_size, color, -1, cv::LINE_AA, false);
}

int is_inside(Polygon polygon, double xp, double yp)
{
    int cnt = 0;
    double x1, y1, x2, y2;
    for( size_t i = 0; i < polygon.edges.size(); i++) {
        x1 = polygon.edges[i].x1;
        y1 = polygon.edges[i].y1;
        x2 = polygon.edges[i].x2;
        y2 = polygon.edges[i].y2;

        //std::cout << "\nyp:" << yp << " y1: " << y1 << " y2: " << y2 << " xp: " << xp << " rest: " << x1 + ((yp-y1)/(y2-y1))*(x2-x1) << std::endl;

        if ((((yp < y1) && (yp > y2)) || ((yp > y1) && (yp < y2))) && (xp < (x1 + ((yp-y1)/(y2-y1))*(x2-x1)))) {
            //std::cout << "\nx: " << xp << " y: " << yp << std::endl;
            cnt += 1;
        }
    }
    return cnt % 2;
}

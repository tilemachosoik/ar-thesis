#include "openglrenderer.hpp"

#include <utils/utils.hpp>

/* #include <opencv2/calib3d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp> */

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

namespace global {
    extern HackyData hackyData; // hacky
    extern SharedShotData filtered_shot_data;
} // namespace global

OpenGLRenderer::OpenGLRenderer(const StreamerConfiguration& config) : Magnum::Platform::WindowlessApplication({mock_main_arguments::argc, mock_main_arguments::argv}, Magnum::NoCreate), _opengl_valid(false)
{
    _gpu_id = config.gpu_id;
    _logos.clear();
    _logos = config.logos;
    _rendering_ROI = config.rendering_ROI;
    _camera_type = config.camera_type;
    _use_opengl = true;
    data_url = config.data_url;
    green_circle_url = config.green_circle_url;
    red_x_url = config.red_x_url;
    black_dot_url = config.black_dot_url;

    // Tab configurations
    background_template = read_image("tab/transparent_background.png", true);
    orange_bar = read_image("tab/orange_bar.png", true);
    purple_bar = read_image("tab/purple_bar.png", true);
    blue_bar = read_image("tab/blue_bar.png", true);

    front_logo_roi = cv::Rect(34, 125, 225, 225);
    background_logo_roi = cv::Rect(141, 89, 355, 355);

    orange_bar_roi = calculate_fit_ROI(orange_bar, cv::Rect(343, 154, 295, 83), "right", "center");
    middle_bar_roi = calculate_fit_ROI(orange_bar, cv::Rect(343, 209., 295, 83), "right", "center");
    purple_bar_roi = calculate_fit_ROI(purple_bar, cv::Rect(343, 264, 295, 83), "right", "center");
    blue_bar_roi = calculate_fit_ROI(blue_bar, cv::Rect(434, 389, 204, 51), "right", "center");

    resize_image(orange_bar, orange_bar, orange_bar_roi.size());
    resize_image(purple_bar, purple_bar, purple_bar_roi.size());
    resize_image(blue_bar, blue_bar, blue_bar_roi.size());
    split_alpha_from_color_image(background_template, background_template, background_alpha_template);
    split_alpha_from_color_image(orange_bar, orange_bar, orange_bar_alpha);
    split_alpha_from_color_image(purple_bar, purple_bar, purple_bar_alpha);
    split_alpha_from_color_image(blue_bar, blue_bar, blue_bar_alpha);

    // Fonts
    _font0 = cv::freetype::createFreeType2();
    _font0->loadFontData("fonts/Fredoka-Regular.ttf", 0);
    _font1 = cv::freetype::createFreeType2();
    _font1->loadFontData("fonts/Fredoka-Medium.ttf", 0);
    fontHeightName = 60;
    fontHeightStats = 50;
    fontHeightTime = 35;
    tabFontHeightName = 150;
    tabFontHeightStats = 130;
    fontHeightRegion = 80;
    fontHeightRegionCorner = 40;

    // Points for information-printing on the tab
    namePoint = cv::Point(31, 30);
    periodPoint = cv::Point(437, 405);
    point_2p = cv::Point(361, 180);
    point_3p = cv::Point(361, 290);
    point_middle = cv::Point(361, 235);

    nameMaxWidth = 575;
    periodMaxWidth = 198;
    pointsMaxWidth = 270;

    // Getting list of names and logos
    tab_names.push_back(config.teamA_name);
    tab_names.push_back(config.teamB_name);
    for (int i = 0; i < 12; i++) {
        tab_names.push_back(config.teamA_player_names[i]);
    }
    for (int i = 0; i < 12; i++) {
        tab_names.push_back(config.teamB_player_names[i]);
    }

    cv::Mat tempLogo;
    for (int i = 0; i < 12; i++) {
        tab_logos.teamA_player.push_back(tempLogo);
        tab_logos.teamB_player.push_back(tempLogo);
    }

    if (!config.logo_teamA.empty()) {
        tab_logos.teamA = config.logo_teamA;
    }
    if (!config.logo_teamB.empty()) {
        tab_logos.teamB = config.logo_teamB;
    }
    for (int i = 0; i < 12; i++) {
        if (!config.logo_teamA_players[i].empty()) {
            tab_logos.teamA_player[i] = config.logo_teamA_players[i].clone();
        }
        if (!config.logo_teamB_players[i].empty()) {
            tab_logos.teamB_player[i] = config.logo_teamB_players[i].clone();
        }
    }

    // Regions

    region_template = read_image("tab/region_template.png", true);
    hotzone1 = read_image("region_templates/hotzone1.png", true);
    hotzone2 = read_image("region_templates/hotzone2.png", true);
    hotzone3 = read_image("region_templates/hotzone3.png", true);
    hotzone4 = read_image("region_templates/hotzone4.png", true);
    hotzone5 = read_image("region_templates/hotzone5.png", true);
    hotzone6 = read_image("region_templates/hotzone6.png", true);
    hotzone7 = read_image("region_templates/hotzone7.png", true);
    hotzone8 = read_image("region_templates/hotzone8.png", true);
    hotzone9 = read_image("region_templates/hotzone9.png", true);
    hotzone1_right = read_image("region_templates/hotzone1_right.png", true);
    hotzone2_right = read_image("region_templates/hotzone2_right.png", true);
    hotzone3_right = read_image("region_templates/hotzone3_right.png", true);
    hotzone4_right = read_image("region_templates/hotzone4_right.png", true);
    hotzone5_right = read_image("region_templates/hotzone5_right.png", true);
    hotzone6_right = read_image("region_templates/hotzone6_right.png", true);
    hotzone7_right = read_image("region_templates/hotzone7_right.png", true);
    hotzone8_right = read_image("region_templates/hotzone8_right.png", true);
    hotzone9_right = read_image("region_templates/hotzone9_right.png", true);
    
    split_alpha_from_color_image(region_template, region_template, region_alpha_template); 

    Point_3d point0(0., 0.); 
    Point_3d point1(0., 1.15);
    Point_3d point2(0., 5.1);
    Point_3d point3(0., 7.); 
    Point_3d point4(0., 9.7);
    Point_3d point5(0., 14.);
    Point_3d point6(0., 15.);
    Point_3d point7(5.5, 5.1);
    Point_3d point8(5.5, 9.7);
    Point_3d point9(2.9, 1.05);
    Point_3d point10(5.25, 2.1);
    Point_3d point11(6.6, 3.35);
    Point_3d point12(7.55, 5.1);
    Point_3d point13(8., 7.); 
    Point_3d point14(7.55, 9.7);
    Point_3d point15(6.6, 11.25);
    Point_3d point16(5.25, 12.6);
    Point_3d point17(2.9, 14.);
    Point_3d point18(10., 0.);
    Point_3d point19(10., 5.1);
    Point_3d point20(10., 9.7);
    Point_3d point21(10., 15.);
    Point_3d point9A(2.9, 0.);
    Point_3d point17A(2.9, 15.);

    

    Edge tempEdge;
    // Region 1
    tempEdge.setPoints(point1.x, point1.y, point9.x, point9.y);
    polygon1.edges.push_back(tempEdge);
    tempEdge.setPoints(point9.x, point9.y, point10.x, point10.y);
    polygon1.edges.push_back(tempEdge);
    tempEdge.setPoints(point10.x, point10.y, point11.x, point11.y);
    polygon1.edges.push_back(tempEdge);
    tempEdge.setPoints(point11.x, point11.y, point12.x, point12.y);
    polygon1.edges.push_back(tempEdge);
    tempEdge.setPoints(point12.x, point12.y, point2.x, point2.y);
    polygon1.edges.push_back(tempEdge);
    tempEdge.setPoints(point2.x, point2.y, point1.x, point1.y);
    polygon1.edges.push_back(tempEdge);

    // Region 2
    tempEdge.setPoints(point2.x, point2.y, point7.x, point7.y);
    polygon2.edges.push_back(tempEdge);
    tempEdge.setPoints(point7.x, point7.y, point8.x, point8.y);
    polygon2.edges.push_back(tempEdge);
    tempEdge.setPoints(point8.x, point8.y, point4.x, point4.y);
    polygon2.edges.push_back(tempEdge);
    tempEdge.setPoints(point4.x, point4.y, point2.x, point2.y);
    polygon2.edges.push_back(tempEdge);

    // Region 3
    tempEdge.setPoints(point4.x, point4.y, point14.x, point14.y);
    polygon3.edges.push_back(tempEdge);
    tempEdge.setPoints(point14.x, point14.y, point15.x, point15.y);
    polygon3.edges.push_back(tempEdge);
    tempEdge.setPoints(point15.x, point15.y, point16.x, point16.y);
    polygon3.edges.push_back(tempEdge);
    tempEdge.setPoints(point16.x, point16.y, point17.x, point17.y);
    polygon3.edges.push_back(tempEdge);
    tempEdge.setPoints(point17.x, point17.y, point5.x, point5.y);
    polygon3.edges.push_back(tempEdge);
    tempEdge.setPoints(point5.x, point5.y, point4.x, point4.y);
    polygon3.edges.push_back(tempEdge);

    // Region 4
    tempEdge.setPoints(point7.x, point7.y, point12.x, point12.y);
    polygon4.edges.push_back(tempEdge);
    tempEdge.setPoints(point12.x, point12.y, point13.x, point13.y);
    polygon4.edges.push_back(tempEdge);
    tempEdge.setPoints(point13.x, point13.y, point14.x, point14.y);
    polygon4.edges.push_back(tempEdge);
    tempEdge.setPoints(point14.x, point14.y, point8.x, point8.y);
    polygon4.edges.push_back(tempEdge);
    tempEdge.setPoints(point8.x, point8.y, point7.x, point7.y);
    polygon4.edges.push_back(tempEdge);

    // Region 5
    tempEdge.setPoints(point9A.x, point9A.y, point18.x, point18.y);
    polygon5.edges.push_back(tempEdge);
    tempEdge.setPoints(point18.x, point18.y, point19.x, point19.y);
    polygon5.edges.push_back(tempEdge);
    tempEdge.setPoints(point19.x, point19.y, point12.x, point12.y);
    polygon5.edges.push_back(tempEdge);
    tempEdge.setPoints(point12.x, point12.y, point11.x, point11.y);
    polygon5.edges.push_back(tempEdge);
    tempEdge.setPoints(point11.x, point11.y, point10.x, point10.y);
    polygon5.edges.push_back(tempEdge);
    tempEdge.setPoints(point10.x, point10.y, point9.x, point9.y);
    polygon5.edges.push_back(tempEdge);
    tempEdge.setPoints(point9.x, point9.y, point9A.x, point9A.y);
    polygon5.edges.push_back(tempEdge);

    // Region 6
    tempEdge.setPoints(point12.x, point12.y, point19.x, point19.y);
    polygon6.edges.push_back(tempEdge);
    tempEdge.setPoints(point19.x, point19.y, point20.x, point20.y);
    polygon6.edges.push_back(tempEdge);
    tempEdge.setPoints(point20.x, point20.y, point14.x, point14.y);
    polygon6.edges.push_back(tempEdge);
    tempEdge.setPoints(point14.x, point14.y, point13.x, point13.y);
    polygon6.edges.push_back(tempEdge);
    tempEdge.setPoints(point13.x, point13.y, point12.x, point12.y);
    polygon6.edges.push_back(tempEdge);

    // Region 7
    tempEdge.setPoints(point14.x, point14.y, point20.x, point20.y);
    polygon7.edges.push_back(tempEdge);
    tempEdge.setPoints(point20.x, point20.y, point21.x, point21.y);
    polygon7.edges.push_back(tempEdge);
    tempEdge.setPoints(point21.x, point21.y, point17A.x, point17A.y);
    polygon7.edges.push_back(tempEdge);
    tempEdge.setPoints(point17A.x, point17A.y, point17.x, point17.y);
    polygon7.edges.push_back(tempEdge);
    tempEdge.setPoints(point17.x, point17.y, point16.x, point16.y);
    polygon7.edges.push_back(tempEdge);
    tempEdge.setPoints(point16.x, point16.y, point15.x, point15.y);
    polygon7.edges.push_back(tempEdge);
    tempEdge.setPoints(point15.x, point15.y, point14.x, point14.y);
    polygon7.edges.push_back(tempEdge);

    // Region 8
    tempEdge.setPoints(point0.x, point0.y, point9A.x, point9A.y);
    polygon8.edges.push_back(tempEdge);
    tempEdge.setPoints(point9A.x, point9A.y, point9.x, point9.y);
    polygon8.edges.push_back(tempEdge);
    tempEdge.setPoints(point9.x, point9.y, point1.x, point1.y);
    polygon8.edges.push_back(tempEdge);
    tempEdge.setPoints(point1.x, point1.y, point0.x, point0.y);
    polygon8.edges.push_back(tempEdge);

    // Region 9
    tempEdge.setPoints(point5.x, point5.y, point17.x, point17.y);
    polygon9.edges.push_back(tempEdge);
    tempEdge.setPoints(point17.x, point17.y, point17A.x, point17A.y);
    polygon9.edges.push_back(tempEdge);
    tempEdge.setPoints(point17A.x, point17A.y, point6.x, point6.y);
    polygon9.edges.push_back(tempEdge);
    tempEdge.setPoints(point6.x, point6.y, point5.x, point5.y);
    polygon9.edges.push_back(tempEdge);


    try {
        _calibration_params = cnpy::npz_load(config.calibration_path);
        _init_intrinsic_map();
        _init_extrinsic_map();
    }
    catch (const std::exception& error) {
        _use_opengl = false;
        std::cerr << error.what() << std::endl;
        std::cout << "Calibration file could not be loaded. OpenGL operations will be disabled." << std::endl;
    }
}

OpenGLRenderer::~OpenGLRenderer()
{
    opengl_destroy();
}

void OpenGLRenderer::_init_extrinsic_map()
{
    cnpy::NpyArray cshape, cR, cT;
    cshape = _calibration_params["shape"];

    cR = _calibration_params["rvecs"];
    cT = _calibration_params["tvecs"];

    cv::Mat r = cv::Mat(cR.shape[0], cR.shape[1], CV_64F, cR.data<double>()).clone();
    cv::Mat t = cv::Mat(cT.shape[0], cT.shape[1], CV_64F, cT.data<double>()).clone();

    cv::Mat R;
    cv::Rodrigues(r, R);

    _Tr = cv::Mat::eye(4, 4, R.type()); // T is 4x4
    _Tr(cv::Range(0, 3), cv::Range(0, 3)) = R * 1; // copies R into T
    _Tr(cv::Range(0, 3), cv::Range(3, 4)) = t * 1; // copies tvec into T
}

void OpenGLRenderer::_init_intrinsic_map()
{
    cnpy::NpyArray cK, cD, cshape, cfov_scale, cR, cT;
    cK = _calibration_params["K"];
    cD = _calibration_params["D"];
    cfov_scale = _calibration_params["fov_scale"];
    cshape = _calibration_params["shape"];
    cv::Size shape;

    cv::Mat K = cv::Mat(cK.shape[0], cK.shape[1], CV_64F, cK.data<double>());
    _K = cv::Mat(cK.shape[0], cK.shape[1], CV_64F);
    K.copyTo(_K);
    _D = cv::Mat(cD.shape[0], cD.shape[1], CV_64F, cD.data<double>());
    _fov_scale = *cfov_scale.data<double>();
    std::size_t* data = cshape.data<std::size_t>();
    shape = cv::Size(data[0], data[1]);

    _original_width = data[0];
    _original_height = data[1];

    if (_camera_type == "fisheye") {
        cv::fisheye::estimateNewCameraMatrixForUndistortRectify(_K, _D, shape, cv::Mat::eye(3, 3, CV_32F), _new_K, 1.0, shape, _fov_scale);
        cv::fisheye::initUndistortRectifyMap(_K, _D, cv::Mat::eye(3, 3, CV_32F), _new_K, shape, CV_32F, _map1, _map2);
    }
    else {
        _new_K = cv::getOptimalNewCameraMatrix(_K, _D, shape, 1., shape);
        cv::initUndistortRectifyMap(_K, _D, cv::Mat(), _new_K, shape, 5, _map1, _map2);
    }
}

// Function for converting 2D image points in 3D world points
// u is the 2D x coordinate and v is the 2D y coordinate
std::array<double, 3> OpenGLRenderer::_compute_xy(double u, double v, double Z, double skew, double tol)
{
    double fx = _new_K.at<double>(0, 0);
    double fy = _new_K.at<double>(1, 1);
    double cx = _new_K.at<double>(0, 2);
    double cy = _new_K.at<double>(1, 2);

    double b = (v - cy) / fy;
    double a = (u - cx - skew * b) / fx;

    double a11 = _Tr.at<double>(0, 0) - a * _Tr.at<double>(2, 0);
    double a12 = _Tr.at<double>(0, 1) - a * _Tr.at<double>(2, 1);

    double x = _Tr.at<double>(0, 3);
    double y = _Tr.at<double>(1, 3);
    double z = _Tr.at<double>(2, 3);
    double b1 = a * z - x + (a * _Tr.at<double>(2, 2) - _Tr.at<double>(0, 2)) * Z;

    double a21 = _Tr.at<double>(1, 0) - b * _Tr.at<double>(2, 0);
    double a22 = _Tr.at<double>(1, 1) - b * _Tr.at<double>(2, 1);
    double b2 = b * z - y + (b * _Tr.at<double>(2, 2) - _Tr.at<double>(1, 2)) * Z;

    cv::Mat A(2, 2, CV_64FC1);
    A.at<double>(0, 0) = a11;
    A.at<double>(0, 1) = a12;
    A.at<double>(1, 0) = a21;
    A.at<double>(1, 1) = a22;

    cv::Mat vec_b(2, 1, CV_64FC1);
    vec_b.at<double>(0, 0) = b1;
    vec_b.at<double>(1, 0) = b2;

    cv::Mat sol_x;

    if (!cv::solve(A, vec_b, sol_x))
        return {0., 0., 0.};

    return {sol_x.at<double>(0, 0), sol_x.at<double>(1, 0), Z};
}

ShotChartData OpenGLRenderer::add_point(double x, double y)
{
    ShotChartData black_dot;
    black_dot.made = 2;

    double z = 0.;
    double qx = 0., qy = 0., qz = 0.;
    double sx = 0.5, sy = 0.5, sz = 1.;

    cv::Mat r = (cv::Mat_<double>(3, 1) << qx, qy, qz);
    cv::Mat t = (cv::Mat_<double>(3, 1) << x, y, z);
    cv::Mat R;
    cv::Rodrigues(r, R);

    black_dot.transformation = cv::Mat::eye(4, 4, R.type());
    black_dot.transformation(cv::Range(0, 3), cv::Range(0, 3)) = R * 1;
    black_dot.transformation(cv::Range(0, 3), cv::Range(3, 4)) = t * 1;

    cv::Mat scaling = cv::Mat::eye(4, 4, R.type());
    scaling.at<double>(0, 0) = sx;
    scaling.at<double>(1, 1) = sy;
    scaling.at<double>(2, 2) = sz;

    black_dot.transformation = black_dot.transformation * scaling;
    return black_dot;
}

// Function that gets the path and creates the transformation matrix for a shot
ShotChartData OpenGLRenderer::_read_shot_data(const ShotDataEntry& data)
{
    ShotChartData shot;

    double x_for_region, y_for_region;

    double x = 0., y = 0., z = 0.;
    double qx = 0., qy = 0., qz = 0.;
    double sx = 0.65, sy = 0.65, sz = 1.;

    // Convert 2D to 3D
    auto point_3D = _compute_xy(data.xPos, data.yPos);

    x = point_3D[0];
    y = point_3D[1];

    if (x > 14.) {
        x_for_region = 28. - x;
        y_for_region = 15. - y;
    } else {
        x_for_region = x;
        y_for_region = y;
    }

    // Positioning shots on selected side of the court
    if ((global::hackyData.side == 0 && x > 14.) || (global::hackyData.side == 1 && x < 14.)) {
        x = 28. - x;
        y = 15. - y;
    }

    // Getting the region of the shot
   /*  if (global::hackyData.side == 0) {
        if ((x >= 0) && (x <= 5.79) && (y >= 5.1) && (y <= 9.9)) {
            shot.region = 1;
        }
        
    } */


    cv::Mat r = (cv::Mat_<double>(3, 1) << qx, qy, qz);
    cv::Mat t = (cv::Mat_<double>(3, 1) << x, y, z);
    cv::Mat R;
    cv::Rodrigues(r, R);

    shot.transformation = cv::Mat::eye(4, 4, R.type());
    shot.transformation(cv::Range(0, 3), cv::Range(0, 3)) = R * 1;
    shot.transformation(cv::Range(0, 3), cv::Range(3, 4)) = t * 1;

    cv::Mat scaling = cv::Mat::eye(4, 4, R.type());
    scaling.at<double>(0, 0) = sx;
    scaling.at<double>(1, 1) = sy;
    scaling.at<double>(2, 2) = sz;

    shot.transformation = shot.transformation * scaling;

    shot.made = data.made;

    shot.x = x_for_region;
    shot.y = y_for_region;

    return shot;
}

void OpenGLRenderer::update_shots(const ShotData& data)
{
    _shots.clear();
    region1.clear();
    region2.clear();
    region3.clear();
    region4.clear();
    region5.clear();
    region6.clear();
    region7.clear();
    region8.clear();
    region9.clear();
    for (std::size_t i = 0; i < data.size(); i++) {
        ShotChartData shot_chart_data = _read_shot_data(data.at(i));
        _shots.push_back(shot_chart_data);
    }

    for (std::size_t i = 0; i < _shots.size(); i++) {
        if (is_inside(polygon1, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 1;
        }
        else if (is_inside(polygon2, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 2;
        }
        else if (is_inside(polygon3, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 3;
        }
        else if (is_inside(polygon4, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 4;
        }
        else if (is_inside(polygon5, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 5;
        }
        else if (is_inside(polygon6, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 6;
        }
        else if (is_inside(polygon7, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 7;
        }
        else if (is_inside(polygon8, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 8;
        }
        else if (is_inside(polygon9, _shots[i].x, _shots[i].y) == 1) {
            _shots[i].region = 9;
        }
    }

    for (std::size_t i = 0; i < _shots.size(); i++) {
        switch(_shots[i].region) {
        case 1:
            if (_shots[i].made == 1) {
                region1.made++;
                region1.total++;
            } else if (_shots[i].made == 0) {
                region1.total++;
            }
            break;
        case 2:
            if (_shots[i].made == 1) {
                region2.made++;
                region2.total++;
            } else if (_shots[i].made == 0) {
                region2.total++;
            }
            break;
        case 3:
            if (_shots[i].made == 1) {
                region3.made++;
                region3.total++;
            } else if (_shots[i].made == 0) {
                region3.total++;
            }
            break;
        case 4:
            if (_shots[i].made == 1) {
                region4.made++;
                region4.total++;
            } else if (_shots[i].made == 0) {
                region4.total++;
            }
            break;
        case 5:
            if (_shots[i].made == 1) {
                region5.made++;
                region5.total++;
            } else if (_shots[i].made == 0) {
                region5.total++;
            }
            break;
        case 6:
            if (_shots[i].made == 1) {
                region6.made++;
                region6.total++;
            } else if (_shots[i].made == 0) {
                region6.total++;
            }
            break;
        case 7:
            if (_shots[i].made == 1) {
                region7.made++;
                region7.total++;
            } else if (_shots[i].made == 0) {
                region7.total++;
            }
            break;
        case 8:
            if (_shots[i].made == 1) {
                region8.made++;
                region8.total++;
            } else if (_shots[i].made == 0) {
                region8.total++;
            }
            break;
        case 9:
            if (_shots[i].made == 1) {
                region9.made++;
                region9.total++;
            } else if (_shots[i].made == 0) {
                region9.total++;
            }
            break;
        }
    }

    //_shots.push_back(add_point(14., 7.));


    /* // Baseline points
    _shots.push_back(add_point(0., 0.)); // 0
    _shots.push_back(add_point(0., 1.15)); // 1
    _shots.push_back(add_point(0., 5.1)); // 2
    _shots.push_back(add_point(0., 7.)); // 3
    _shots.push_back(add_point(0., 9.7)); // 4
    _shots.push_back(add_point(0., 13.75)); // 5
    _shots.push_back(add_point(0., 14.5)); // 6
    // Elbow Points
    _shots.push_back(add_point(5.5, 5.1)); // 7
    _shots.push_back(add_point(5.5, 9.7)); // 8
    // 3pt line points
    _shots.push_back(add_point(2.9, 1.05)); // 9
    _shots.push_back(add_point(5.25, 2.1)); // 10
    _shots.push_back(add_point(6.6, 3.35)); // 11
    _shots.push_back(add_point(7.55, 5.1)); // 12
    _shots.push_back(add_point(8., 7.)); // 13 - Center
    _shots.push_back(add_point(7.55, 9.7)); // 14
    _shots.push_back(add_point(6.6, 11.25)); // 15
    _shots.push_back(add_point(5.25, 12.6)); // 16
    _shots.push_back(add_point(2.9, 13.75)); // 17
    // MidCourt Points
    _shots.push_back(add_point(10., 0.)); // 18
    _shots.push_back(add_point(10., 5.1)); // 19
    _shots.push_back(add_point(10., 9.7)); // 20
    _shots.push_back(add_point(10., 14.5)); // 21
    // Sideline Points
    _shots.push_back(add_point(2.9, 0.)); // 9A
    _shots.push_back(add_point(2.9, 14.5)); // 17A */

    /* _shots.push_back(add_point(10., 0.));
    _shots.push_back(add_point(10., 1.));
    _shots.push_back(add_point(10., 2.));
    _shots.push_back(add_point(10., 3.));
    _shots.push_back(add_point(10., 4.));
    _shots.push_back(add_point(10., 5.));
    _shots.push_back(add_point(10., 6.));
    _shots.push_back(add_point(10., 7.)); */
    

    /*
    Region 1:
    1-9-10-11-12-2-1

    Region 2:
    2-7-8-4-2

    Region 3:
    4-14-15-16-17-5-4

    Region 4:
    7-12-13-14-8-7

    Region 5:
    9A-18-19-12-11-10-9-9A

    Region 6:
    12-19-20-14-13-12

    Region 7:
    14-20-21-17A-17-16-15-14

    Region 8:
    0-9A-9-1-0

    Region 9:
    5-17-17A-6-5

    */

 
}

void OpenGLRenderer::print_tab(const ShotData& data)
{
    
    stats.reset();

    cv::Mat tab_logo, resized_logo;
    std::string tab_name;

    int interpolation = cv::INTER_AREA;

    double final_width = 400.000;
    double final_height = 400.000;
    cv::Size final_size(final_width, final_height);

    cv::Rect logo_roi(150.000, 253.000, 400.000, 400.000);
    cv::Point namePoint(200.000, 83.000 - (0.28 * tabFontHeightName));
    cv::Point namePointTeam(540.000, 83.000 - (0.28 * tabFontHeightName));
    cv::Point point_2p(675.000, 295.000 - (0.28 * tabFontHeightStats));
    cv::Point point_3p(675.000, 535.000 - (0.28 * tabFontHeightStats));
    cv::Point point_middle(675.000, 395.000 - (0.28 * tabFontHeightStats));

    double x = -2.2, y = 5.2, z = 0.;
    double qx = 2., qy = 1.2, qz = 0.8; // Best translation and orientation i could get
    double sx = 3., sy = 3., sz = 1.;

    if (global::hackyData.side == 1) {
        x = 30.2;
        qy = -1.2;
        qz = -0.8;
    }

    /* double x = 13., y = 4.5, z = 0.;
    double qx = 0., qy = 0., qz = 0.;              // Render tab in the middle of the court for viewing purposes
    double sx = 8., sy = 6., sz = 1.;  */

    cv::Mat tab_background = cv::imread("tab/tab_background.png", cv::IMREAD_UNCHANGED);
    // tab_background = cv::Mat(1500, 1500, CV_8UC4, cv::Scalar(255, 255, 255, 1));                     <-- This did not work

    if (!tab_background.empty()) {
        if (tab_background.channels() == 3) {
            cv::cvtColor(tab_background, tab_background, cv::COLOR_BGR2BGRA);
        }
        else if (tab_background.channels() == 1) {
            cv::cvtColor(tab_background, tab_background, cv::COLOR_GRAY2BGRA);
        }

        if (global::hackyData.player == 0) {
            if (data[0].teamId == "A") {
                tab_logo = tab_logos.teamA;
                tab_name = "Team Stats";
            }
            else if (data[0].teamId == "B") {
                tab_logo = tab_logos.teamB;
                tab_name = "Team Stats";
            }
        }
        else if (global::hackyData.player == 1) {
            if (data[0].teamId == "A") {
                tab_logo = tab_logos.teamA_player[0];
                tab_name = tab_names[2];
            }
            else if (data[0].teamId == "B") {
                tab_logo = tab_logos.teamB_player[0];
                tab_name = tab_names[14];
            }
        }

        /* if (!tab_logo.empty()) {
            std::cout << "NOT EMPTY";                                  // To check: teamA logo is empty, teamB logo is not, but both work!
        } */

        // resize
        if (tab_logo.size().width < final_width) {
            interpolation = cv::INTER_CUBIC;
        }
        cv::resize(tab_logo, resized_logo, final_size, interpolation);

        std::vector<cv::Mat> tab_layers;
        cv::split(tab_background, tab_layers);
        cv::merge(std::vector<cv::Mat>{tab_layers[0], tab_layers[1], tab_layers[2]}, tab_background);
        cv::Mat alpha = tab_layers[3].clone();

        std::vector<cv::Mat> logo_layers;
        cv::split(resized_logo, logo_layers);
        cv::merge(std::vector<cv::Mat>{logo_layers[0], logo_layers[1], logo_layers[2]}, resized_logo);
        cv::Mat logo_alpha = logo_layers[3].clone();

        resized_logo.copyTo(tab_background(logo_roi), logo_alpha);

        // Print name
        if (global::hackyData.player == 0) {
            _font0->putText(tab_background, tab_name, namePointTeam, tabFontHeightName, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
        }
        else
            _font0->putText(tab_background, tab_name, namePoint, tabFontHeightName, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);

        // Get stats
        for (size_t i = 0; i < global::filtered_shot_data.shot_data.size(); i++) {
            if ((global::filtered_shot_data.shot_data[i].shotType.compare("2p") == 0) && (global::filtered_shot_data.shot_data[i].made == 1)) {
                stats.made2p++;
                stats.total2p++;
            }
            else if ((global::filtered_shot_data.shot_data[i].shotType.compare("2p") == 0) && (global::filtered_shot_data.shot_data[i].made == 0)) {
                stats.total2p++;
            }
            else if ((global::filtered_shot_data.shot_data[i].shotType.compare("3p") == 0) && (global::filtered_shot_data.shot_data[i].made == 1)) {
                stats.made3p++;
                stats.total3p++;
            }
            else if ((global::filtered_shot_data.shot_data[i].shotType.compare("3p") == 0) && (global::filtered_shot_data.shot_data[i].made == 0)) {
                stats.total3p++;
            }
        }

        // Print 2p stats
        if ((global::hackyData.shotType == 2) || global::hackyData.shotType == 0) {
            double percentage_2p = (stats.made2p / stats.total2p) * 100;
            char formated_percentage_2p[5];
            std::sprintf(formated_percentage_2p, "%.1lf", percentage_2p);
            std::string label_2p = "2FG: " + std::to_string(static_cast<int>(stats.made2p)) + "/" + std::to_string(static_cast<int>(stats.total2p)) + " " + formated_percentage_2p + "%";
            if (global::hackyData.shotType == 0) {
                _font1->putText(tab_background, label_2p, point_2p, tabFontHeightStats, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
            }
            else if (global::hackyData.shotType == 2) {
                _font1->putText(tab_background, label_2p, point_middle, tabFontHeightStats, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
            }
        }

        // Print 3p stats
        if ((global::hackyData.shotType == 3) || global::hackyData.shotType == 0) {
            double percentage_3p = (stats.made3p / stats.total3p) * 100;
            char formated_percentage_3p[5];
            std::sprintf(formated_percentage_3p, "%.1lf", percentage_3p);
            std::string label_3p = "3FG: " + std::to_string(static_cast<int>(stats.made3p)) + "/" + std::to_string(static_cast<int>(stats.total3p)) + " " + formated_percentage_3p + "%";
            if (global::hackyData.shotType == 0) {
                _font1->putText(tab_background, label_3p, point_3p, tabFontHeightStats, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
            }
            else if (global::hackyData.shotType == 3) {
                _font1->putText(tab_background, label_3p, point_middle, tabFontHeightStats, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
            }
        }

        tab_layers.clear();
        cv::split(tab_background, tab_layers);
        tab_layers.push_back(alpha);
        cv::merge(std::vector<cv::Mat>{tab_layers[0], tab_layers[1], tab_layers[2], tab_layers[3]}, tab_background);

        cv::flip(tab_background, tab_background, 0);

        _tab_texture.emplace_back(new Magnum::GL::Texture2D);
        (*_tab_texture[0])
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {tab_background.size().width, tab_background.size().height})
            .setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGBA8Unorm, {tab_background.size().width, tab_background.size().height}, Magnum::Containers::ArrayView<unsigned char>{tab_background.data, tab_background.size().width * tab_background.size().height * tab_background.elemSize()}});
    }
    else
        std::cout << "Could not load image: ";

    cv::Mat r = (cv::Mat_<double>(3, 1) << qx, qy, qz);
    cv::Mat t = (cv::Mat_<double>(3, 1) << x, y, z);
    cv::Mat R;
    cv::Rodrigues(r, R);

    tab_transformation = cv::Mat::eye(4, 4, R.type());
    tab_transformation(cv::Range(0, 3), cv::Range(0, 3)) = R * 1;
    tab_transformation(cv::Range(0, 3), cv::Range(3, 4)) = t * 1;

    cv::Mat scaling = cv::Mat::eye(4, 4, R.type());
    scaling.at<double>(0, 0) = sx;
    scaling.at<double>(1, 1) = sy;
    scaling.at<double>(2, 2) = sz;

    tab_transformation = tab_transformation * scaling;
}

void OpenGLRenderer::print_logo_middle()
{
    cv::Mat logo;
    if (global::hackyData.team == 1) {
        logo = tab_logos.teamA;
    }
    else if (global::hackyData.team == 2) {
        logo = tab_logos.teamB;
    }

    if (!logo.empty()) {
        if (logo.channels() == 3) {
            cv::cvtColor(logo, logo, cv::COLOR_BGR2BGRA);
        }
        else if (logo.channels() == 1) {
            cv::cvtColor(logo, logo, cv::COLOR_GRAY2BGRA);
        }
    }
    else
        std::cout << "Could not load image: ";

    double final_width = 250.000;
    double final_height = 250.000;
    cv::Size final_logo_size(final_width, final_height);

    // resize
    int interpolation = cv::INTER_AREA;
    if (logo.size().width < final_width) {
        interpolation = cv::INTER_CUBIC;
    }
    cv::resize(logo, logo, final_logo_size, interpolation);

    cv::flip(logo, logo, 0);

    _logo_texture.emplace_back(new Magnum::GL::Texture2D);
    (*_logo_texture[0])
        .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
        .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
        .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {logo.size().width, logo.size().height})
        .setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGBA8Unorm, {logo.size().width, logo.size().height}, Magnum::Containers::ArrayView<unsigned char>{logo.data, logo.size().width * logo.size().height * logo.elemSize()}});

    double x = 14., y = 7.2, z = 0.;
    double qx = 0., qy = 0., qz = 0.; // Render shot in the middle of the court
    double sx = 3., sy = 3., sz = 1.;

    cv::Mat r = (cv::Mat_<double>(3, 1) << qx, qy, qz);
    cv::Mat t = (cv::Mat_<double>(3, 1) << x, y, z);
    cv::Mat R;
    cv::Rodrigues(r, R);

    logo_transformation = cv::Mat::eye(4, 4, R.type());
    logo_transformation(cv::Range(0, 3), cv::Range(0, 3)) = R * 1;
    logo_transformation(cv::Range(0, 3), cv::Range(3, 4)) = t * 1;

    cv::Mat scaling = cv::Mat::eye(4, 4, R.type());
    scaling.at<double>(0, 0) = sx;
    scaling.at<double>(1, 1) = sy;
    scaling.at<double>(2, 2) = sz;

    logo_transformation = logo_transformation * scaling;
}

void OpenGLRenderer::print_stats_on_court(const ShotData& data)
{
    // Position, orientation and scaling of our tab
    double x = 0., y = 0., z = 0.;
    double qx = 0., qy = 0., qz = 0.;
    double sx = 4.487, sy = 3.5, sz = 1.; // Scaling with correct aspect ratio!

    if (global::hackyData.side == 0) { // Left Court
        x = 11.72;
        y = 1.58;
    }
    else if (global::hackyData.side == 1) { // Right Court
        x = 16.3;
        y = 1.6;
    }

    cv::Mat team_logo, player_logo;
    std::string tab_name, time_period;

    background = background_template.clone();
    background_alpha = background_alpha_template.clone();

    if (!background.empty()) {

        switch (global::hackyData.timePeriod) { // Setting up time-period label
        case 1:
            time_period = "1st Quarter";
            break;
        case 2:
            time_period = "2nd Quarter";
            break;
        case 3:
            time_period = "3rd Quarter";
            break;
        case 4:
            time_period = "4th Quarter";
            break;
        case 5:
            time_period = "1st Half";
            break;
        case 6:
            time_period = "2nd Half";
            break;
        }

        // Getting Team/Player name and logo
        if (global::hackyData.player == 0) {
            if (data[0].teamId == "A") {
                team_logo = tab_logos.teamA;
                tab_name = tab_names[0];
            }
            else if (data[0].teamId == "B") {
                team_logo = tab_logos.teamB;
                tab_name = tab_names[1];
            }
        }
        else if (data[0].teamId == "A") {
            team_logo = tab_logos.teamA;
            tab_name = tab_names[global::hackyData.player + 1] + " #" + std::to_string(global::hackyData.player);
            if (!tab_logos.teamA_player[global::hackyData.player - 1].empty()) {
                player_logo = tab_logos.teamA_player[global::hackyData.player - 1];
            }
        }
        else if (data[0].teamId == "B") {
            team_logo = tab_logos.teamB;
            tab_name = tab_names[global::hackyData.player + 13] + " #" + std::to_string(global::hackyData.player);
            if (!tab_logos.teamB_player[global::hackyData.player - 1].empty()) {
                player_logo = tab_logos.teamB_player[global::hackyData.player - 1];
            }
        }

        // Formatting the name label
        std::size_t spacePos, numPos;
        if (global::hackyData.player != 0) {
            spacePos = tab_name.find(" ");
            numPos = tab_name.find("#");
            tab_name = tab_name.substr(spacePos + 1, numPos - 2 - spacePos + 1) + " " + tab_name.substr(0, 1) + ". " + tab_name.substr(numPos, std::string::npos);
        }

        // Placing Player and Team logo
        if ((global::hackyData.player != 0) && (!player_logo.empty())) {
            place_image_into_ROI(team_logo, background, background_logo_roi, cv::Mat(), "center", "center", false, 1.0);
            cv::Rect player_roi = calculate_fit_ROI(player_logo, front_logo_roi, "center", "center");
            cv::Mat tmp_img, tmp_img_alpha;
            // Resize src to match the width and height of the new display_roi
            resize_image(player_logo, tmp_img, player_roi.size());
            split_alpha_from_color_image(tmp_img, tmp_img, tmp_img_alpha);
            cv::Mat dst = background(player_roi);
            overlay_image(tmp_img, dst, tmp_img_alpha, false, 1.);
            background_alpha(player_roi) = cv::max(background_alpha(player_roi), tmp_img_alpha);
        }
        else {
            cv::Rect team_logo_roi = calculate_fit_ROI(team_logo, front_logo_roi, "center", "center");
            cv::Mat tmp_img, tmp_img_alpha;
            // Resize src to match the width and height of the new display_roi
            resize_image(team_logo, tmp_img, team_logo_roi.size());
            split_alpha_from_color_image(tmp_img, tmp_img, tmp_img_alpha);
            cv::Mat dst = background(team_logo_roi);
            overlay_image(tmp_img, dst, tmp_img_alpha, false, 1.);
            background_alpha(team_logo_roi) = cv::max(background_alpha(team_logo_roi), tmp_img_alpha);
        }

        // Placing the Stats bar depending on what stats are being shown (2p/3p/both)
        if ((global::hackyData.shotType == 2) || (global::hackyData.shotType == 3)) {
            cv::Mat dst = background(middle_bar_roi);
            overlay_image(orange_bar, dst, orange_bar_alpha, false, 1.);
            background_alpha(middle_bar_roi) = cv::max(background_alpha(middle_bar_roi), orange_bar_alpha);
        }
        else {
            cv::Mat dst = background(orange_bar_roi);
            overlay_image(orange_bar, dst, orange_bar_alpha, false, 1.);
            background_alpha(orange_bar_roi) = cv::max(background_alpha(orange_bar_roi), orange_bar_alpha);
            dst = background(purple_bar_roi);
            overlay_image(purple_bar, dst, purple_bar_alpha, false, 1.);
            background_alpha(purple_bar_roi) = cv::max(background_alpha(purple_bar_roi), purple_bar_alpha);
        }
        // Placing blue time-period bar
        cv::Mat dst = background(blue_bar_roi);
        overlay_image(blue_bar, dst, blue_bar_alpha, false, 1.);
        background_alpha(blue_bar_roi) = cv::max(background_alpha(blue_bar_roi), blue_bar_alpha);

        // Placing name and time-period labels
        draw_text(background, tab_name, namePoint, nameMaxWidth, _font0, fontHeightName, cv::Scalar(0, 0, 0));
        draw_text(background_alpha, tab_name, namePoint, nameMaxWidth, _font0, fontHeightName, cv::Scalar(255, 255, 255));

        draw_text(background, time_period, periodPoint, periodMaxWidth, _font0, fontHeightTime, cv::Scalar(255, 255, 255));

        // Print 2p stats
        if ((global::hackyData.shotType == 2) || (global::hackyData.shotType == 0)) {
            global::filtered_shot_data.stats.percentage_2p = (global::filtered_shot_data.stats.made2p / global::filtered_shot_data.stats.total2p) * 100;
            std::sprintf(global::filtered_shot_data.stats.formated_percentage_2p, "%.1lf", global::filtered_shot_data.stats.percentage_2p);
            global::filtered_shot_data.stats.label_2p = "2FG  " + std::string(global::filtered_shot_data.stats.formated_percentage_2p) + "%";
            if (global::hackyData.shotType == 2) {
                draw_text(background, global::filtered_shot_data.stats.label_2p, point_middle, pointsMaxWidth, _font1, fontHeightStats, cv::Scalar(255, 255, 255));
            }
            else {
                draw_text(background, global::filtered_shot_data.stats.label_2p, point_2p, pointsMaxWidth, _font1, fontHeightStats, cv::Scalar(255, 255, 255));
            }
        }

        // Print 3p stats
        if ((global::hackyData.shotType == 3) || (global::hackyData.shotType == 0)) {
            global::filtered_shot_data.stats.percentage_3p = (global::filtered_shot_data.stats.made3p / global::filtered_shot_data.stats.total3p) * 100;
            std::sprintf(global::filtered_shot_data.stats.formated_percentage_3p, "%.1lf", global::filtered_shot_data.stats.percentage_3p);
            global::filtered_shot_data.stats.label_3p = "3FG  " + std::string(global::filtered_shot_data.stats.formated_percentage_3p) + "%";
            if (global::hackyData.shotType == 3) {
                draw_text(background, global::filtered_shot_data.stats.label_3p, point_middle, pointsMaxWidth, _font1, fontHeightStats, cv::Scalar(255, 255, 255));
            }
            else {
                draw_text(background, global::filtered_shot_data.stats.label_3p, point_3p, pointsMaxWidth, _font1, fontHeightStats, cv::Scalar(255, 255, 255));
            }
        }

        // Going from BGR to BGRA
        std::vector<cv::Mat> tab_layers;
        cv::split(background, tab_layers);
        cv::cvtColor(background_alpha, background_alpha, cv::COLOR_BGR2GRAY);
        tab_layers.push_back(background_alpha);
        cv::merge(std::vector<cv::Mat>{tab_layers[0], tab_layers[1], tab_layers[2], tab_layers[3]}, background);

        cv::flip(background, background, 0);

        // Preparing our texture
        _court_texture.emplace_back(new Magnum::GL::Texture2D);
        (*_court_texture[0])
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {background.size().width, background.size().height})
            .setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGBA8Unorm, {background.size().width, background.size().height}, Magnum::Containers::ArrayView<unsigned char>{background.data, background.size().width * background.size().height * background.elemSize()}});
    }
    else
        std::cout << "Could not load image: ";

    // Preparing the transformation for the texture
    cv::Mat r = (cv::Mat_<double>(3, 1) << qx, qy, qz);
    cv::Mat t = (cv::Mat_<double>(3, 1) << x, y, z);
    cv::Mat R;
    cv::Rodrigues(r, R);

    court_transformation = cv::Mat::eye(4, 4, R.type());
    court_transformation(cv::Range(0, 3), cv::Range(0, 3)) = R * 1;
    court_transformation(cv::Range(0, 3), cv::Range(3, 4)) = t * 1;

    cv::Mat scaling = cv::Mat::eye(4, 4, R.type());
    scaling.at<double>(0, 0) = sx;
    scaling.at<double>(1, 1) = sy;
    scaling.at<double>(2, 2) = sz;

    court_transformation = court_transformation * scaling;
}

void OpenGLRenderer::print_regions() {
    
    int hotzone;
    int highNum = 0;
    double highPercent = 0.0;

    hotzones.clear();

    if (region1.made > highNum) {
        highNum = region1.made;
        hotzone = 1;
    }
    if (region2.made > highNum) {
        highNum = region2.made;
        hotzone = 2;
    }
    if (region3.made > highNum) {
        highNum = region3.made;
        hotzone = 3;
    }
    if (region4.made > highNum) {
        highNum = region4.made;
        hotzone = 4;
    }
    if (region5.made > highNum) {
        highNum = region5.made;
        hotzone = 5;
    }
    if (region6.made > highNum) {
        highNum = region6.made;
        hotzone = 6;
    }
    if (region7.made > highNum) {
        highNum = region7.made;
        hotzone = 7;
    }
    if (region8.made > highNum) {
        highNum = region8.made;
        hotzone = 8;
    }
    if (region9.made > highNum) {
        highNum = region9.made;
        hotzone = 9;
    }


    // If more than 3 made shots in a region, declare hot-zone by percentage of made shots.
    if (region1.made > 2) {
            hotzones.push_back(1);
        }
    if (region2.made > 2) {
            hotzones.push_back(2);
        }
    if (region3.made > 2) {
            hotzones.push_back(3);
        }
    if (region4.made > 2) {
            hotzones.push_back(4);
        }
    if (region5.made > 2) {
            hotzones.push_back(5);
        }
    if (region6.made > 2) {
            hotzones.push_back(6);
        }
    if (region7.made > 2) {
            hotzones.push_back(7);
        }
    if (region8.made > 2) {
            hotzones.push_back(8);
        }
    if (region9.made > 2) {
            hotzones.push_back(9);
        }

    if (highNum >= 3) {
        for (size_t i = 0; i < hotzones.size(); i++) {
            if (hotzones[i] == 1) {
                if (((double)region1.made / (double)region1.total) > highPercent) {
                    highPercent = (double)region1.made / (double)region1.total;
                    hotzone = 1;
                }
            }
            if (hotzones[i] == 2) {
                if (((double)region2.made / (double)region2.total) > highPercent) {
                    highPercent = (double)region2.made / (double)region2.total;
                    hotzone = 2;
                }
            }
            if (hotzones[i] == 3) {
                if (((double)region3.made / (double)region3.total) > highPercent) {
                    highPercent = (double)region3.made / (double)region3.total;
                    hotzone = 3;
                }
            }
            if (hotzones[i] == 4) {
                if (((double)region4.made / (double)region4.total) > highPercent) {
                    highPercent = (double)region4.made / (double)region4.total;
                    hotzone = 4;
                }
            }
            if (hotzones[i] == 5) {
                if (((double)region5.made / (double)region5.total) > highPercent) {
                    highPercent = (double)region5.made / (double)region5.total;
                    hotzone = 5;
                }
            }
            if (hotzones[i] == 6) {
                if (((double)region6.made / (double)region6.total) > highPercent) {
                    highPercent = (double)region6.made / (double)region6.total;
                    hotzone = 6;
                }
            }
            if (hotzones[i] == 7) {
                if (((double)region7.made / (double)region7.total) > highPercent) {
                    highPercent = (double)region7.made / (double)region7.total;
                    hotzone = 7;
                }
            }
            if (hotzones[i] == 8) {
                if (((double)region8.made / (double)region8.total) > highPercent) {
                    highPercent = (double)region8.made / (double)region8.total;
                    hotzone = 8;
                }
            }
            if (hotzones[i] == 9) {
                if (((double)region9.made / (double)region9.total) > highPercent) {
                    highPercent = (double)region9.made / (double)region9.total;
                    hotzone = 9;
                }
            }
        }
    }

    double x = 0., y = 0., z = 0.;
    double qx = 0., qy = 0., qz = 0.; 
    double sx = 12.1, sy = 14.85, sz = 1.;

    if (global::hackyData.side == 0) { // Left Court
        x = 5.7;
        y = 7.1;
    }
    else if (global::hackyData.side == 1) { // Right Court
        x = 22.3;
        y = 7.1;
    }

    // Varied point to print stats depending on the side of the court
    region1Point = cv::Point((global::hackyData.side * 1400) + 260 - (2 * global::hackyData.side * 260) - (global::hackyData.side * 100), 1185);
    region2Point = cv::Point((global::hackyData.side * 1400) + 260 - (2 * global::hackyData.side * 260) - (global::hackyData.side * 100), 700);
    region3Point = cv::Point((global::hackyData.side * 1400) + 260 - (2 * global::hackyData.side * 260) - (global::hackyData.side * 100), 255);
    region4Point = cv::Point((global::hackyData.side * 1400) + 710 - (2 * global::hackyData.side * 710) - (global::hackyData.side * 180), 700);
    region5Point = cv::Point((global::hackyData.side * 1400) + 1020 - (2 * global::hackyData.side * 1020) - (global::hackyData.side * 125), 1185);
    region6Point = cv::Point((global::hackyData.side * 1400) + 1020 - (2 * global::hackyData.side * 1020) - (global::hackyData.side * 180), 700);
    region7Point = cv::Point((global::hackyData.side * 1400) + 1020 - (2 * global::hackyData.side * 1020) - (global::hackyData.side * 125), 255);
    region8Point = cv::Point((global::hackyData.side * 1400) + 130 - (2 * global::hackyData.side * 130) - (global::hackyData.side * 100), 1435);
    region9Point = cv::Point((global::hackyData.side * 1400) + 130 - (2 * global::hackyData.side * 130) - (global::hackyData.side * 100), 20);


    if (global::hackyData.side == 0) {
        switch (hotzone)
        {
        case 1:
            split_alpha_from_color_image(hotzone1, region_background, region_background_alpha);
            break;
        case 2:
            split_alpha_from_color_image(hotzone2, region_background, region_background_alpha);
            break;
        case 3:
            split_alpha_from_color_image(hotzone3, region_background, region_background_alpha);
            break;
        case 4:
            split_alpha_from_color_image(hotzone4, region_background, region_background_alpha);
            break;
        case 5:
            split_alpha_from_color_image(hotzone5, region_background, region_background_alpha);
            break;
        case 6:
            split_alpha_from_color_image(hotzone6, region_background, region_background_alpha);
            break;
        case 7:
            split_alpha_from_color_image(hotzone7, region_background, region_background_alpha);
            break;
        case 8:
            split_alpha_from_color_image(hotzone8, region_background, region_background_alpha);
            break;
        case 9:
            split_alpha_from_color_image(hotzone9, region_background, region_background_alpha);
            break;
        default:
            region_background = region_template.clone();
            region_background_alpha = region_alpha_template.clone();
            std::cout << "\nERROR: No Hotzone\n";
            break;
        }
    } else if (global::hackyData.side == 1) {
        switch (hotzone)
        {
        case 1:
            split_alpha_from_color_image(hotzone1_right, region_background, region_background_alpha);
            break;
        case 2:
            split_alpha_from_color_image(hotzone2_right, region_background, region_background_alpha);
            break;
        case 3:
            split_alpha_from_color_image(hotzone3_right, region_background, region_background_alpha);
            break;
        case 4:
            split_alpha_from_color_image(hotzone4_right, region_background, region_background_alpha);
            break;
        case 5:
            split_alpha_from_color_image(hotzone5_right, region_background, region_background_alpha);
            break;
        case 6:
            split_alpha_from_color_image(hotzone6_right, region_background, region_background_alpha);
            break;
        case 7:
            split_alpha_from_color_image(hotzone7_right, region_background, region_background_alpha);
            break;
        case 8:
            split_alpha_from_color_image(hotzone8_right, region_background, region_background_alpha);
            break;
        case 9:
            split_alpha_from_color_image(hotzone9_right, region_background, region_background_alpha);
            break;
        default:
            region_background = region_template.clone();
            region_background_alpha = region_alpha_template.clone();
            std::cout << "\nERROR: No Hotzone\n";
            break;
        }
    }

    std::string region1_stats, region2_stats, region3_stats, region4_stats, region5_stats, region6_stats, region7_stats, region8_stats, region9_stats;
    region1_stats = std::to_string(region1.made) + "/" + std::to_string(region1.total);
    region2_stats = std::to_string(region2.made) + "/" + std::to_string(region2.total);
    region3_stats = std::to_string(region3.made) + "/" + std::to_string(region3.total);
    region4_stats = std::to_string(region4.made) + "/" + std::to_string(region4.total);
    region5_stats = std::to_string(region5.made) + "/" + std::to_string(region5.total);
    region6_stats = std::to_string(region6.made) + "/" + std::to_string(region6.total);
    region7_stats = std::to_string(region7.made) + "/" + std::to_string(region7.total);
    region8_stats = std::to_string(region8.made) + "/" + std::to_string(region8.total);
    region9_stats = std::to_string(region9.made) + "/" + std::to_string(region9.total);

    _font0->putText(region_background, region1_stats, region1Point, fontHeightRegion, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region1_stats, region1Point, fontHeightRegion, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region2_stats, region2Point, fontHeightRegion, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region2_stats, region2Point, fontHeightRegion, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region3_stats, region3Point, fontHeightRegion, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region3_stats, region3Point, fontHeightRegion, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region4_stats, region4Point, fontHeightRegion, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region4_stats, region4Point, fontHeightRegion, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region5_stats, region5Point, fontHeightRegion, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region5_stats, region5Point, fontHeightRegion, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region6_stats, region6Point, fontHeightRegion, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region6_stats, region6Point, fontHeightRegion, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region7_stats, region7Point, fontHeightRegion, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region7_stats, region7Point, fontHeightRegion, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region8_stats, region8Point, fontHeightRegionCorner, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region8_stats, region8Point, fontHeightRegionCorner, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);

    _font0->putText(region_background, region9_stats, region9Point, fontHeightRegionCorner, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    _font0->putText(region_background_alpha, region9_stats, region9Point, fontHeightRegionCorner, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
    
    // Going from BGR to BGRA
    std::vector<cv::Mat> region_layers;
    cv::split(region_background, region_layers);
    cv::cvtColor(region_background_alpha, region_background_alpha, cv::COLOR_BGR2GRAY);
    region_layers.push_back(region_background_alpha);
    cv::merge(std::vector<cv::Mat>{region_layers[0], region_layers[1], region_layers[2], region_layers[3]}, region_background);

    cv::flip(region_background, region_background, 0);

    _region_texture.emplace_back(new Magnum::GL::Texture2D);
    (*_region_texture[0])
        .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
        .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
        .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {region_background.size().width, region_background.size().height})
        .setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGBA8Unorm, {region_background.size().width, region_background.size().height}, Magnum::Containers::ArrayView<unsigned char>{region_background.data, region_background.size().width * region_background.size().height * region_background.elemSize()}});


    cv::Mat r = (cv::Mat_<double>(3, 1) << qx, qy, qz);
    cv::Mat t = (cv::Mat_<double>(3, 1) << x, y, z);
    cv::Mat R;
    cv::Rodrigues(r, R);

    region_transformation = cv::Mat::eye(4, 4, R.type());
    region_transformation(cv::Range(0, 3), cv::Range(0, 3)) = R * 1;
    region_transformation(cv::Range(0, 3), cv::Range(3, 4)) = t * 1;

    cv::Mat scaling = cv::Mat::eye(4, 4, R.type());
    scaling.at<double>(0, 0) = sx;
    scaling.at<double>(1, 1) = sy;
    scaling.at<double>(2, 2) = sz;

    region_transformation = region_transformation * scaling;
}

void OpenGLRenderer::opengl_init(const StreamerConfiguration& config)
{

    _opengl_valid = false;
    if (_use_opengl) {
        /* Assume context is given externally, if not create it */
        if (!Magnum::GL::Context::hasCurrent()) {
            std::cout << "GL::Context not provided. Creating for gpu #" + std::to_string(_gpu_id) + "." << std::endl;
            Configuration configur = Configuration();
            configur.setDevice(_gpu_id);
            if (!tryCreateContext(configur)) {
                std::cerr << "Could not create GL context for gpu #" + std::to_string(_gpu_id) + "." << std::endl;
                return;
            }
        }

        _combine_mask_shader.reset(new Magnum::CombineMaskShader);
        _textured_quad_shader.reset(new Magnum::TexturedQuadShader);

        _frame_texture.reset(new Magnum::GL::Texture2D);
        _mask_texture.reset(new Magnum::GL::Texture2D);
        _render_texture.reset(new Magnum::GL::Texture2D);
        // Initialize textures
        _frame_texture->setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {static_cast<int>(_rendering_ROI.width), static_cast<int>(_rendering_ROI.height)});

        _mask_texture->setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, Magnum::GL::TextureFormat::R8, {static_cast<int>(_rendering_ROI.width), static_cast<int>(_rendering_ROI.height)});

        // Shot textures
        cv::Mat made_shot_img = cv::imread(config.green_circle_url, cv::IMREAD_UNCHANGED);
        cv::Mat missed_shot_img = cv::imread(config.red_x_url, cv::IMREAD_UNCHANGED);
        cv::Mat black_dot_img = cv::imread(config.black_dot_url, cv::IMREAD_UNCHANGED);

        if (!made_shot_img.empty()) {
            // Check if shot has an alpha channel
            if (made_shot_img.channels() == 3) {
                cv::cvtColor(made_shot_img, made_shot_img, cv::COLOR_BGR2BGRA);
            }
            else if (made_shot_img.channels() == 1) {
                cv::cvtColor(made_shot_img, made_shot_img, cv::COLOR_GRAY2BGRA);
            }
            cv::flip(made_shot_img, made_shot_img, 0);

            _shot_textures.emplace_back(new Magnum::GL::Texture2D);
            (*_shot_textures[0])
                .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
                .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {made_shot_img.size().width, made_shot_img.size().height})
                .setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGBA8Unorm, {made_shot_img.size().width, made_shot_img.size().height}, Magnum::Containers::ArrayView<unsigned char>{made_shot_img.data, made_shot_img.size().width * made_shot_img.size().height * made_shot_img.elemSize()}});
        }
        else
            std::cout << "Could not load shot image: " + config.green_circle_url << std::endl;

        if (!missed_shot_img.empty()) {
            // Check if shot has an alpha channel
            if (missed_shot_img.channels() == 3) {
                cv::cvtColor(missed_shot_img, missed_shot_img, cv::COLOR_BGR2BGRA);
            }
            else if (missed_shot_img.channels() == 1) {
                cv::cvtColor(missed_shot_img, missed_shot_img, cv::COLOR_GRAY2BGRA);
            }
            cv::flip(missed_shot_img, missed_shot_img, 0);

            _shot_textures.emplace_back(new Magnum::GL::Texture2D);
            (*_shot_textures[1])
                .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
                .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {missed_shot_img.size().width, missed_shot_img.size().height})
                .setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGBA8Unorm, {missed_shot_img.size().width, missed_shot_img.size().height}, Magnum::Containers::ArrayView<unsigned char>{missed_shot_img.data, missed_shot_img.size().width * missed_shot_img.size().height * missed_shot_img.elemSize()}});
        }
        else
            std::cout << "Could not load shot image: " + config.red_x_url << std::endl;

        if (!black_dot_img.empty()) {
            // Check if shot has an alpha channel
            if (black_dot_img.channels() == 3) {
                cv::cvtColor(black_dot_img, black_dot_img, cv::COLOR_BGR2BGRA);
            }
            else if (black_dot_img.channels() == 1) {
                cv::cvtColor(black_dot_img, black_dot_img, cv::COLOR_GRAY2BGRA);
            }
            cv::flip(black_dot_img, black_dot_img, 0);

            _shot_textures.emplace_back(new Magnum::GL::Texture2D);
            (*_shot_textures[2])
                .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
                .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {black_dot_img.size().width, black_dot_img.size().height})
                .setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGBA8Unorm, {black_dot_img.size().width, black_dot_img.size().height}, Magnum::Containers::ArrayView<unsigned char>{black_dot_img.data, black_dot_img.size().width * black_dot_img.size().height * black_dot_img.elemSize()}});
        }
        else
            std::cout << "Could not load shot image: " + config.red_x_url << std::endl;

        // Prepare render texture
        _render_texture->setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, Magnum::GL::TextureFormat::RGBA8, {static_cast<int>(_original_width), static_cast<int>(_original_height)}); // size same as input image

        // Create FrameBuffer
        _framebuffer.reset(new Magnum::GL::Framebuffer({{}, {static_cast<int>(_original_width), static_cast<int>(_original_height)}}));

        _framebuffer->attachTexture(Magnum::GL::Framebuffer::ColorAttachment{0}, *_render_texture, 0);
        _framebuffer->mapForDraw({{0, {Magnum::GL::Framebuffer::ColorAttachment{0}}}});

        // Create quad mesh for shot printing
        {
            const QuadVertex quad_data[]{
                {{0.5f, -0.5f, 0.f}, {1.0f, 0.0f}},
                {{0.5f, 0.5f, 0.f}, {1.0f, 1.0f}},
                {{-0.5f, -0.5f, 0.f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f, 0.f}, {1.0f, 1.0f}},
                {{-0.5f, 0.5f, 0.f}, {0.0f, 1.0f}},
                {{-0.5f, -0.5f, 0.f}, {0.0f, 0.0f}}};

            Magnum::GL::Buffer buffer;
            buffer.setData(quad_data);

            _quad_mesh.reset(new Magnum::GL::Mesh);
            (*_quad_mesh)
                .setCount(6)
                .addVertexBuffer(std::move(buffer), 0,
                    Magnum::TexturedQuadShader::Position{},
                    Magnum::TexturedQuadShader::TextureCoordinates{});
        }
        // Compute Camera matrices
        {
            auto size = _render_texture->imageSize(0);
            Magnum::Float near = 0.1f;
            Magnum::Float far = 300.f;

            for (std::size_t col = 0; col != 4; ++col)
                for (std::size_t row = 0; row != 4; ++row)
                    if (row == 1 || row == 2)
                        _view_matrix[col][row] = -static_cast<Magnum::Float>(_Tr.at<double>(row, col));
                    else
                        _view_matrix[col][row] = static_cast<Magnum::Float>(_Tr.at<double>(row, col));

            Magnum::Matrix4 persp;
            for (std::size_t col = 0; col != 4; ++col)
                for (std::size_t row = 0; row != 4; ++row)
                    persp[col][row] = 0.f;
            for (std::size_t col = 0; col != 3; ++col)
                for (std::size_t row = 0; row != 3; ++row) {
                    std::size_t r = row;
                    if (row == 2)
                        r = r + 1;
                    persp[col][r] = static_cast<Magnum::Float>(_new_K.at<double>(row, col));
                }

            persp[2] = -persp[2];
            persp[2][2] = near + far;
            persp[3][2] = near * far;

            Magnum::Float left = 0.f;
            Magnum::Float right = static_cast<Magnum::Float>(size[0]);
            Magnum::Float bottom = 0.f;
            Magnum::Float top = static_cast<Magnum::Float>(size[1]);
            Magnum::Float tx = -(left + right) / (right - left);
            Magnum::Float ty = -(top + bottom) / (top - bottom);
            Magnum::Matrix4 ortho = Magnum::Matrix4::orthographicProjection({static_cast<Magnum::Float>(size[0]), static_cast<Magnum::Float>(size[1])}, near, far);
            ortho[3][0] = tx;
            ortho[3][1] = ty;
            _proj_matrix = ortho * persp;
        }
        _opengl_valid = true;
    }
}

void OpenGLRenderer::opengl_destroy()
{
    if (!_opengl_valid)
        return;

    _combine_mask_shader.reset(nullptr);
    _textured_quad_shader.reset(nullptr);

    _frame_texture.reset(nullptr);
    _mask_texture.reset(nullptr);
    _render_texture.reset(nullptr);
    _quad_mesh.reset(nullptr);
    for (auto& text : _logo_texture)
        text.reset(nullptr);
    _framebuffer.reset(nullptr);
    for (auto& text : _shot_textures)
        text.reset(nullptr);
    _framebuffer.reset(nullptr);
    _opengl_valid = false;
}

void OpenGLRenderer::render(cv::Mat& frame, const cv::Mat& foreground_mask, SharedShotData& shots)
{
    if (_opengl_valid) {
        // Check if we need to update resources for rendering shots
        shots.mutex.lock();
        if (shots.updated.load()) {
            _region_texture.clear();
            _tab_texture.clear();
            _court_texture.clear();
            _logo_texture.clear();

            update_shots(shots.shot_data);
            if (global::hackyData.displayTab) {
                print_tab(shots.shot_data);
            }
            if (global::hackyData.displayCourtStats) {
                print_stats_on_court(shots.shot_data);
                /* std::cout << "\nRegion 1: " << region1.made << "/" << region1.total << std::endl;
                std::cout << "Region 2: " << region2.made << "/" << region2.total << std::endl;
                std::cout << "Region 3: " << region3.made << "/" << region3.total << std::endl;
                std::cout << "Region 4: " << region4.made << "/" << region4.total << std::endl;
                std::cout << "Region 5: " << region5.made << "/" << region5.total << std::endl;
                std::cout << "Region 6: " << region6.made << "/" << region6.total << std::endl;
                std::cout << "Region 7: " << region7.made << "/" << region7.total << std::endl;
                std::cout << "Region 8: " << region8.made << "/" << region8.total << std::endl;
                std::cout << "Region 9: " << region9.made << "/" << region9.total << std::endl; */
                
            }
            if (global::hackyData.displayLogoMiddle) {
                print_logo_middle();
            }
            if (global::hackyData.displayRegions) {
                //std::cout << "\nHERE1\n";
                print_regions();
            }
            shots.updated.store(false);
            global::filtered_shot_data.stats.reset();
        }
        shots.mutex.unlock();

        if (_shots.size() > 0) {
            // Flip image and foreground mask as OpenGL has bottom-left point as (0,0)
            cv::Mat fr, fg_mask;
            cv::flip(foreground_mask, fg_mask, 0);
            frame(_rendering_ROI).copyTo(fr);
            cv::flip(fr, fr, 0);

            // This is to pass OpenCV cv::Mat to Magnum
            auto image_view = Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::RGB8Unorm, {fr.size().width, fr.size().height}, Magnum::Containers::ArrayView<unsigned char>{fr.data, fr.size().width * fr.size().height * fr.elemSize()}};
            _frame_texture->setSubImage(0, {}, image_view);
            _mask_texture->setSubImage(0, {}, Magnum::ImageView2D{Magnum::PixelStorage{}.setAlignment(1), Magnum::PixelFormat::R8Unorm, {fg_mask.size().width, fg_mask.size().height}, Magnum::Containers::ArrayView<unsigned char>{fg_mask.data, fg_mask.size().width * fg_mask.size().height * fg_mask.elemSize()}});

            // Bind the _framebuffer
            _framebuffer->bind();
            // Clear _framebuffer
            _framebuffer->clearColor(0, Magnum::Color4{0.f, 0.f, 0.f, 0.f});

            Magnum::Matrix4 model_matrix;
            Magnum::Matrix4 mat;

            // Regions
            if (_region_texture.size() > 0) {
                for (std::size_t col = 0; col != 4; ++col)
                    for (std::size_t row = 0; row != 4; ++row)
                        model_matrix[col][row] = static_cast<Magnum::Float>(region_transformation.at<double>(row, col));
                mat = _proj_matrix * _view_matrix * model_matrix;

                (*_textured_quad_shader)
                    .setTransformationMatrix(mat)
                    .bindTexture(*_region_texture[0]);

                _textured_quad_shader->draw(*_quad_mesh);
            }

            // Tab under basket
            if (_tab_texture.size() > 0) {
                for (std::size_t col = 0; col != 4; ++col)
                    for (std::size_t row = 0; row != 4; ++row)
                        model_matrix[col][row] = static_cast<Magnum::Float>(tab_transformation.at<double>(row, col));
                mat = _proj_matrix * _view_matrix * model_matrix;

                (*_textured_quad_shader)
                    .setTransformationMatrix(mat)
                    .bindTexture(*_tab_texture[0]);

                _textured_quad_shader->draw(*_quad_mesh);
            }

            // Middle_logo
            if (_logo_texture.size() > 0) {
                for (std::size_t col = 0; col != 4; ++col)
                    for (std::size_t row = 0; row != 4; ++row)
                        model_matrix[col][row] = static_cast<Magnum::Float>(logo_transformation.at<double>(row, col));
                mat = _proj_matrix * _view_matrix * model_matrix;

                (*_textured_quad_shader)
                    .setTransformationMatrix(mat)
                    .bindTexture(*_logo_texture[0]);

                _textured_quad_shader->draw(*_quad_mesh);
            }

            // Tab on court
            if (_court_texture.size() > 0) {
                for (std::size_t col = 0; col != 4; ++col)
                    for (std::size_t row = 0; row != 4; ++row)
                        model_matrix[col][row] = static_cast<Magnum::Float>(court_transformation.at<double>(row, col));
                mat = _proj_matrix * _view_matrix * model_matrix;

                (*_textured_quad_shader)
                    .setTransformationMatrix(mat)
                    .bindTexture(*_court_texture[0]);

                _textured_quad_shader->draw(*_quad_mesh);
            }

            // Shots
            if (global::hackyData.displayShots) {
                for (std::size_t i = 0; i < _shots.size(); i++) {
                    // Model matrix is different for each shot. Projection and view matrix are the same for all shots.
                    for (std::size_t col = 0; col != 4; ++col)
                        for (std::size_t row = 0; row != 4; ++row)
                            model_matrix[col][row] = static_cast<Magnum::Float>(_shots[i].transformation.at<double>(row, col));
                    mat = _proj_matrix * _view_matrix * model_matrix;

                    if (_shots[i].made == 1) {
                        (*_textured_quad_shader)
                            .setTransformationMatrix(mat)
                            .bindTexture(*_shot_textures[0]);

                        _textured_quad_shader->draw(*_quad_mesh);
                    }
                    else if (_shots[i].made == 0){
                        (*_textured_quad_shader)
                            .setTransformationMatrix(mat)
                            .bindTexture(*_shot_textures[1]);

                        _textured_quad_shader->draw(*_quad_mesh);
                    }
                    else if (_shots[i].made == 2){
                        (*_textured_quad_shader)
                            .setTransformationMatrix(mat)
                            .bindTexture(*_shot_textures[2]);

                        _textured_quad_shader->draw(*_quad_mesh);
                    }
                }
            }
            

            

            

            Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::Blending);
            // Run combine mask shader
            Magnum::Int offsetx = _rendering_ROI.x;
            Magnum::Int offsety = _original_height - _rendering_ROI.y - _rendering_ROI.height;
            (*_combine_mask_shader)
                .setWidth(_rendering_ROI.width)
                .setHeight(_rendering_ROI.height)
                .setOffsetX(offsetx)
                .setOffsetY(offsety)
                .bindInputTexture(*_frame_texture)
                .bindMaskTexture(*_mask_texture)
                .bindOutputTexture(*_render_texture);

            _combine_mask_shader->dispatchCompute({static_cast<Magnum::UnsignedInt>(_rendering_ROI.width), static_cast<Magnum::UnsignedInt>(_rendering_ROI.height), 1});
            Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess | Magnum::GL::Renderer::MemoryBarrier::TextureFetch | Magnum::GL::Renderer::MemoryBarrier::ShaderStorage);
            auto image = _render_texture->subImage(0, {{offsetx, offsety}, {offsetx + _rendering_ROI.width, offsety + _rendering_ROI.height}}, {Magnum::GL::PixelFormat::RGB, Magnum::GL::PixelType::UnsignedByte});
            Corrade::Containers::StridedArrayView2D<const Magnum::Color3ub> src = image.pixels<Magnum::Color3ub>().flipped<0>();
            Corrade::Containers::StridedArrayView2D<Magnum::Color3ub> dst{Corrade::Containers::arrayCast<Magnum::Color3ub>(Corrade::Containers::arrayView(fr.data, image.size().product() * sizeof(Magnum::Color3ub))), {std::size_t(image.size().y()), std::size_t(image.size().x())}};
            Corrade::Utility::copy(src, dst);
            // Here fr contains the ROI of the frame with the shots rendered on it. We just need to update the corresponding ROI of the original frame with fr.
            fr.copyTo(frame(_rendering_ROI));
        }
    }
}

std::size_t OpenGLRenderer::get_gpu_id() const { return _gpu_id; }

std::size_t OpenGLRenderer::num_logos() const { return _logos.size(); }

// std::size_t OpenGLRenderer::num_shots() const { return _shots.size(); }

bool OpenGLRenderer::is_opengl_init() const { return _opengl_valid; }

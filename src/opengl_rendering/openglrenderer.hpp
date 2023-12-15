#ifndef OPENGL_RENDERING_OPENGLRENDERER_HPP
#define OPENGL_RENDERING_OPENGLRENDERER_HPP

#include <opengl_rendering/shaders/combine_mask_shader.hpp>
#include <opengl_rendering/shaders/render_texture_shader.hpp>
#include <opengl_rendering/shaders/textured_quad_shader.hpp>
#include <opengl_rendering/windowless_contexts.hpp>
#include <utils/utils.hpp>

#include <opencv2/core.hpp>

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/ArrayViewStl.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Resource.h>

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/ImageFormat.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Shaders/DistanceFieldVector.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include <cnpy/cnpy.h>

#include <string>

namespace mock_main_arguments {
    // We need those because Magnum::Platform::WindowlessApplication needs to accept argc, argv. Passing those directly in the constructor does not work; they need to be stored in memory (no idea why!).
    inline int argc = 0;
    inline char** argv = nullptr;
} // namespace mock_main_arguments

class OpenGLRenderer : public Magnum::Platform::WindowlessApplication {
public:
    struct QuadVertex {
        Magnum::Vector3 position;
        Magnum::Vector2 textureCoordinates;
    };

    OpenGLRenderer(const StreamerConfiguration& config);
    ~OpenGLRenderer();

    std::array<double, 3> _compute_xy(double u, double v, double Z = 0., double skew = 0., double tol = 1e-3);

    void opengl_init(const StreamerConfiguration& config);
    void opengl_destroy();

    void update_shots(const ShotData& data);
    void print_tab(const ShotData& data);
    void print_logo_middle();
    void print_stats_on_court(const ShotData& data);
    void print_regions();
    void render(cv::Mat& frame, const cv::Mat& foreground_mask, SharedShotData& shots);

    std::size_t get_gpu_id() const;
    std::size_t num_logos() const;
    std::size_t num_shots() const;

    bool is_opengl_init() const;

    // for Magnum
    virtual int exec() override { return 0; }

protected:
    // urls
    std::string data_url;
    std::string green_circle_url;
    std::string red_x_url;
    std::string black_dot_url;
    // Parameters
    std::size_t _gpu_id = 0;
    std::vector<LogoData> _logos;
    std::vector<ShotChartData> _shots;
    cv::Rect _rendering_ROI;

    // OpenGL related
    bool _use_opengl = false;
    bool _opengl_valid = false;
    std::unique_ptr<Magnum::CombineMaskShader> _combine_mask_shader;
    std::unique_ptr<Magnum::TexturedQuadShader> _textured_quad_shader;
    std::unique_ptr<Magnum::GL::Texture2D> _frame_texture, _mask_texture;
    std::vector<std::unique_ptr<Magnum::GL::Texture2D>> _logo_texture;
    std::vector<std::unique_ptr<Magnum::GL::Texture2D>> _shot_textures;
    std::vector<std::unique_ptr<Magnum::GL::Texture2D>> _tab_texture;
    std::vector<std::unique_ptr<Magnum::GL::Texture2D>> _court_texture;
    std::vector<std::unique_ptr<Magnum::GL::Texture2D>> _region_texture;

    cv::Mat tab_transformation;
    cv::Mat logo_transformation;
    cv::Mat court_transformation;
    cv::Mat region_transformation;

    std::unique_ptr<Magnum::GL::Texture2D> _render_texture;
    std::unique_ptr<Magnum::GL::Mesh> _quad_mesh;
    std::unique_ptr<Magnum::GL::Framebuffer> _framebuffer;
    Magnum::Matrix4 _view_matrix, _proj_matrix;

    // Fonts
    cv::Ptr<cv::freetype::FreeType2> _font0;
    cv::Ptr<cv::freetype::FreeType2> _font1;
    int fontHeightName;
    int fontHeightStats;
    int fontHeightTime;
    int tabFontHeightName;
    int tabFontHeightStats;
    int fontHeightRegion;
    int fontHeightRegionCorner;

    // Tab
    cv::Mat background, background_template, orange_bar, purple_bar, blue_bar, background_alpha, background_alpha_template, orange_bar_alpha, purple_bar_alpha, blue_bar_alpha, team_logo_alpha, player_logo_alpha;
    cv::Rect front_logo_roi, background_logo_roi, orange_bar_roi, middle_bar_roi, purple_bar_roi, blue_bar_roi;
    cv::Point namePoint, periodPoint, point_2p, point_3p, point_middle;
    std::size_t nameMaxWidth, periodMaxWidth, pointsMaxWidth;

    // Logos
    Logos tab_logos;

    // Tab stats
    std::vector<std::string> tab_names;
    Stats stats;

    // Polygons and regions
    Polygon polygon1, polygon2, polygon3, polygon4, polygon5, polygon6, polygon7, polygon8, polygon9;
    Region region1, region2, region3, region4, region5, region6, region7, region8, region9;
    cv::Mat region_template, region_background, region_alpha_template, region_background_alpha, hotzone1, hotzone2, hotzone3, hotzone4, hotzone5, hotzone6, hotzone7, hotzone8, hotzone9, hotzone1_right, hotzone2_right, hotzone3_right, hotzone4_right, hotzone5_right, hotzone6_right, hotzone7_right, hotzone8_right, hotzone9_right;
    std::vector<int> hotzones;
    cv::Point region1Point, region2Point, region3Point, region4Point, region5Point, region6Point, region7Point, region8Point, region9Point;
    
    // Calibration-related parameters needed for opengl
    cnpy::npz_t _calibration_params;
    std::string _camera_type = "fisheye";
    std::size_t _original_width = 3840;
    std::size_t _original_height = 2160;
    double _fov_scale = 1.;
    cv::Mat _K, _D, _new_K;
    cv::Mat _map1;
    cv::Mat _map2;
    cv::Mat _Tr;

    // Methods
    void _init_intrinsic_map();
    void _init_extrinsic_map();
    ShotChartData _read_shot_data(const ShotDataEntry& data);
    ShotChartData add_point(double x, double y);
};

#endif
#ifndef OPENGL_RENDERING_SHADERS_RENDER_TEXTURE_SHADER_HPP
#define OPENGL_RENDERING_SHADERS_RENDER_TEXTURE_SHADER_HPP

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum {
    class RenderTextureShader : public GL::AbstractShaderProgram {
    public:
        explicit RenderTextureShader(NoCreateT) : GL::AbstractShaderProgram{NoCreate} {}

        explicit RenderTextureShader()
        {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("opengl-render-data");

            GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

            vert.addSource(rs.getString("RenderTextureShader.vert"));
            frag.addSource(rs.getString("RenderTextureShader.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
            CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());
        }

        RenderTextureShader& bindOutputTexture(GL::Texture2D& tex)
        {
            tex.bind(_texturePos);
            return *this;
        }

    private:
        Int _texturePos = 0;
    };
} // namespace Magnum

#endif
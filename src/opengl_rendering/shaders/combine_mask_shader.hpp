#ifndef OPENGL_RENDERING_SHADERS_COMBINE_MASK_SHADER_HPP
#define OPENGL_RENDERING_SHADERS_COMBINE_MASK_SHADER_HPP

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/ImageFormat.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum {

    class CombineMaskShader : public GL::AbstractShaderProgram {
    public:
        explicit CombineMaskShader(NoCreateT) : GL::AbstractShaderProgram{NoCreate} {}

        explicit CombineMaskShader()
        {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            /* Load and compile shaders from compiled-in resource */
            Utility::Resource rs("opengl-render-data");

            GL::Shader comp{GL::Version::GL430, GL::Shader::Type::Compute};

            comp.addSource("#extension GL_ARB_shader_image_load_store : require\n");
            comp.addSource(rs.getString("CombineMask.comp"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(comp.compile());

            attachShaders({comp});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            /* Get uniform locations */
            _widthUniform = uniformLocation("width");
            _heightUniform = uniformLocation("height");
            _offsetxUniform = uniformLocation("offset_x");
            _offsetyUniform = uniformLocation("offset_y");
        }

        CombineMaskShader& setWidth(UnsignedInt width)
        {
            setUniform(_widthUniform, width);
            return *this;
        }

        CombineMaskShader& setHeight(UnsignedInt height)
        {
            setUniform(_heightUniform, height);
            return *this;
        }

        CombineMaskShader& setOffsetX(UnsignedInt offsetx)
        {
            setUniform(_offsetxUniform, offsetx);
            return *this;
        }

        CombineMaskShader& setOffsetY(UnsignedInt offsety)
        {
            setUniform(_offsetyUniform, offsety);
            return *this;
        }

        CombineMaskShader& bindInputTexture(GL::Texture2D& input)
        {
            input.bindImage(_inputPos, 0, GL::ImageAccess::ReadOnly, GL::ImageFormat::RGBA8);
            return *this;
        }

        CombineMaskShader& bindMaskTexture(GL::Texture2D& mask)
        {
            mask.bindImage(_maskPos, 0, GL::ImageAccess::ReadOnly, GL::ImageFormat::R8);
            return *this;
        }

        CombineMaskShader& bindOutputTexture(GL::Texture2D& output)
        {
            output.bindImage(_outputPos, 0, GL::ImageAccess::ReadWrite, GL::ImageFormat::RGBA8);
            return *this;
        }

    private:
        Int _widthUniform, _heightUniform, _offsetxUniform, _offsetyUniform;
        Int _inputPos = 0, _maskPos = 2, _outputPos = 4;
    };
} // namespace Magnum

#endif
#ifndef OPENGL_RENDERING_SHADERS_TEXTURED_QUAD_SHADER_HPP
#define OPENGL_RENDERING_SHADERS_TEXTURED_QUAD_SHADER_HPP

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum {
    class TexturedQuadShader : public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;
        typedef GL::Attribute<1, Vector2> TextureCoordinates;

        explicit TexturedQuadShader()
        {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

            const Utility::Resource rs{"opengl-render-data"};

            GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

            vert.addSource("#extension GL_ARB_explicit_uniform_location : enable\n");
            vert.addSource(rs.getString("TexturedQuad.vert"));
            frag.addSource(rs.getString("TexturedQuad.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
            CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

            attachShaders({vert, frag});

            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            _transformationMatrixUniform = uniformLocation("transformationMatrix");

            setUniform(uniformLocation("textureData"), TextureUnit);
        }

        TexturedQuadShader& setTransformationMatrix(const Matrix4& mat)
        {
            setUniform(_transformationMatrixUniform, mat);
            return *this;
        }

        TexturedQuadShader& bindTexture(GL::Texture2D& texture)
        {
            texture.bind(TextureUnit);
            return *this;
        }

    private:
        enum : Int { TextureUnit = 0 };

        Int _transformationMatrixUniform = 0;
    };
} // namespace Magnum

#endif
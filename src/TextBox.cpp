
#include "TextBox.h"

#include <array>
#include <filesystem>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H          // <freetype/freetype.h>
#include FT_MODULE_H            // <freetype/ftmodapi.h>
#include FT_GLYPH_H             // <freetype/ftglyph.h>
#include FT_SYNTHESIS_H         // <freetype/ftsynth.h>

#include <vkl/PipelineFactory.h>
#include <vkl/BufferManager.h>
#include <vkl/Pipeline.h>
#include <vkl/VertexBuffer.h>
#include <vkl/DrawCall.h>
#include <vkl/IndexBuffer.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace
{
	constexpr const char* VertShader = R"Shader(

#version 450

layout(location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
layout(location = 0) out vec2 TexCoords;

layout(binding = 1) uniform UBO {
    vec4  viewport;
} u_common;

void main()
{
    vec4 pos = vec4(vertex.xy, 0.0, 1.0);
    pos.x = (pos.x - u_common.viewport.x) / u_common.viewport.z;
    pos.y = (pos.y - u_common.viewport.y) / u_common.viewport.w;
    pos.x = pos.x * 2 - 1;
    pos.y = pos.y * 2 - 1;

    gl_Position = pos;
    TexCoords = vertex.zw;
}
)Shader";
	constexpr const char* FragShader = R"Shader(

#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 TexCoords;

layout(binding = 0) uniform sampler2D Tex;

void main() {
    outColor = texture(Tex, TexCoords);
	gl_FragDepth = .4f;
}
)Shader";


	class FontTexture
	{
	public:
		void setSize(size_t x, size_t y)
		{
			m_width = (int)x;
			m_height = (int)y;
			m_data.resize(m_width * m_height * 4, 0);
		}
		void setData(unsigned char* data, size_t width, size_t height, size_t offset)
		{
			for (size_t y = 0; y < height; ++y)
			{
				unsigned char* output = &m_data[(y * m_width * 4) + (offset * 4)];
				unsigned char* input = &data[y * width];
				for (int x = 0; x < width; ++x)
				{
					//Work around VKL not supporting other texture formats yet - TODO
					output[0] = (unsigned char)255;
					output[1] = (unsigned char)255;
					output[2] = (unsigned char)255;
					output[3] = *input;
					output += 4;
					++input;
				}
				//memcpy(output, input, width);
			}

		}
		void writeTest()
		{
			stbi_write_png(std::string("FontAtlas.png").c_str(), m_width, m_height, 4, m_data.data(), m_width*4);
		}
		int m_width, m_height;
		std::vector<unsigned char> m_data;
	};


	struct char_data
	{
		float ax; // advance.x
		float ay; // advance.y

		float bw; // bitmap.width;
		float bh; // bitmap.rows;

		float bl; // bitmap_left;
		float bt; // bitmap_top;

		float tx; // x offset of glyph in texture coordinates
	};

	struct FontAtlas
	{

		std::array<char_data, 128> chars;
		FontTexture tex;
		bool valid = false;
	};

	const FontAtlas buildFontAtlas()
	{
		FontAtlas atlas;

		FT_Library ft;
		if (FT_Init_FreeType(&ft))
		{
			std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
			return {};
		}

		std::error_code ec;
		auto path = std::filesystem::absolute(std::filesystem::path("calibri.ttf"), ec);

		FT_Face face;
		if (ec.value() != 0 || FT_New_Face(ft, path.string().c_str(), 0, &face))
		{
			std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
			return {};
		}

		FT_Set_Pixel_Sizes(face, TextBox::typical_title_font_size, 0);

		FT_GlyphSlot g = face->glyph;
		unsigned int w = 0;
		unsigned int h = 0;

		for (unsigned int i = 32; i < 128; i++) {
			if (auto code = FT_Load_Char(face, i, FT_LOAD_RENDER); code) {
				auto errStr = FT_Error_String(code);
				if (errStr)
					std::cerr << errStr << std::endl;
				std::cerr << "Loading character " << std::to_string(i) << "failed!\n";
				continue;
			}

			w += g->bitmap.width;
			h = std::max(h, g->bitmap.rows);
		}

		atlas.tex.setSize(w, h);

		int x = 0;

		for (int i = 32; i < 128; i++) {
			if (FT_Load_Char(face, i, FT_LOAD_RENDER))
				continue;

			atlas.tex.setData(g->bitmap.buffer, g->bitmap.width, g->bitmap.rows, x);

			atlas.chars[i].ax = (float)(g->advance.x >> 6);
			atlas.chars[i].ay = (float)(g->advance.y >> 6);

			atlas.chars[i].bw = (float)g->bitmap.width;
			atlas.chars[i].bh = (float)g->bitmap.rows;

			atlas.chars[i].bl = (float)g->bitmap_left;
			atlas.chars[i].bt = (float)g->bitmap_top;

			atlas.chars[i].tx = (float)x / w;

			x += g->bitmap.width;

		}

		atlas.tex.writeTest();
		atlas.valid = true;
		return atlas;
	}

	const FontAtlas& fontAtlas()
	{
		static const FontAtlas fontAtlas = buildFontAtlas();
		return fontAtlas;
	}
}
REGISTER_PIPELINE(TextBox, TextBox::describePipeline)

TextBox::TextBox(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager)
{
	_vbo = bufferManager.createVertexBuffer(device, swapChain);
	addVBO(_vbo, 0);
	_drawCall = std::make_shared<vkl::DrawCall>();
	addDrawCall(_drawCall);

	auto texBuff = bufferManager.createTextureBuffer(device, swapChain, fontAtlas().tex.m_data.data(), fontAtlas().tex.m_width, fontAtlas().tex.m_height, 4);
	addTexture(texBuff, 0);

	_uniform = bufferManager.createTypedUniform<glm::vec4>(device, swapChain);
	addUniform(_uniform, 1);

}
void TextBox::setText(std::string_view text)
{
	_text = text;
	_textDirty = true;
}
std::string_view TextBox::getText() const
{
	return _text;
}
void TextBox::setPosition(const glm::vec2& ndc)
{
	_position = ndc;
}
glm::vec2 TextBox::getPosition() const
{
	return _position;
}
void TextBox::setSize(const glm::vec2& pixels)
{
	_size = pixels;
	_textDirty = true;
}
glm::vec2 TextBox::getSize() const
{
	return _size;
}
void TextBox::update(const glm::vec4& viewport)
{
	if (_textDirty || _positionDirty || _lastViewport != viewport)
	{
		_vertexData.resize(_text.size() * 6);

		int n = 0;
		float x = (_position.x + 1.0f) * 0.5f * viewport.z;
		float y = (_position.y + 1.0f) * 0.5f * viewport.w;
		float sx = _size.x;
		float sy = _size.y;
		int atlas_width = fontAtlas().tex.m_width;
		int atlas_height = fontAtlas().tex.m_height;
		const auto& c = fontAtlas().chars;

		for (char p : _text) {
			float x2 = x + c[p].bl * sx;
			float y2 = y - c[p].bt * sy;
			float w = c[p].bw * sx;
			float h = c[p].bh * sy;

			/* Advance the cursor to the start of the next character */
			x += c[p].ax * sx;
			y += c[p].ay * sy;

			/* Skip glyphs that have no pixels */
			if (!w || !h)
				continue;


			/*
			4 3
			1 2
			*/

			glm::vec4 p1{ x2, y2 + h,                  c[p].tx,                            c[p].bh / atlas_height };
			glm::vec4 p2{ x2 + w, y2 + h,              c[p].tx + c[p].bw / atlas_width,    c[p].bh / atlas_height };
			glm::vec4 p3{ x2 + w, y2,                  c[p].tx + c[p].bw / atlas_width,    0 };
			glm::vec4 p4{ x2, y2,                      c[p].tx,                            0 };

			_vertexData[n++] = p4;
			_vertexData[n++] = p1;
			_vertexData[n++] = p2;
			_vertexData[n++] = p4;
			_vertexData[n++] = p2;
			_vertexData[n++] = p3;
		}

		_vbo->setData(_vertexData.data(), sizeof(glm::vec4), _vertexData.size());
		_drawCall->setCount((uint32_t)_vertexData.size());
		_uniform->setData(viewport);

	}

	_lastViewport = viewport;
	_positionDirty = false;
	_textDirty = false;
}

void TextBox::describePipeline(vkl::PipelineDescription& description)
{
	description.addShaderGLSL(VK_SHADER_STAGE_VERTEX_BIT, VertShader);
	description.addShaderGLSL(VK_SHADER_STAGE_FRAGMENT_BIT, FragShader);
	description.setPrimitiveTopology(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	description.declareVertexAttribute(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(glm::vec4), 0);
	description.declareTexture(0);
	description.declareUniform(1, sizeof(glm::vec4));
	description.setBlendEnabled(true);
	//description.setDepthOp(VK_COMPARE_OP_ALWAYS);
}



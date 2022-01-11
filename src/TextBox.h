#pragma once

#include <vkl/RenderObject.h>
#include <vxt/LinearAlgebra.h>
#include <vkl/PipelineFactory.h>
#include <vkl/VertexBuffer.h>
#include <vxt/LinearAlgebra.h>
#include <vkl/UniformBuffer.h>

class TextBox : public vkl::RenderObject
{
	PIPELINE_TYPE


	static void describePipeline(vkl::PipelineDescription& description);

public:
	TextBox(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager);

	void setText(std::string_view text);
	std::string_view getText() const;

	void setPosition(const glm::vec2& pixels);
	glm::vec2 getPosition() const;

	void setSize(const glm::vec2& ndc);
	glm::vec2 getSize() const;

	void update(const glm::vec4& view);

	static constexpr inline int typical_title_font_size = 32;

private:
	std::string _text;
	bool _textDirty = true;

	glm::vec2 _position{ 0,0 };
	glm::vec2 _size{ 1,1 };
	bool _positionDirty = true;

	glm::vec4 _lastViewport{ 0,0,0,0 };

	std::vector<glm::vec4> _vertexData;
	std::shared_ptr<vkl::VertexBuffer> _vbo;
	std::shared_ptr<vkl::DrawCall> _drawCall;
	std::shared_ptr<vkl::TypedUniform<glm::vec4>> _uniform;
};
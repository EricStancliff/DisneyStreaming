#pragma once
#include <vkl/RenderObject.h>
#include <vxt/LinearAlgebra.h>
#include <vkl/PipelineFactory.h>


class Background : public vkl::RenderObject
{
	PIPELINE_TYPE

	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;
	};

	static void describePipeline(vkl::PipelineDescription& description);

public:
	Background(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager);

private:
	std::vector<Vertex> _verts;
	std::vector<uint32_t> _indices;
};


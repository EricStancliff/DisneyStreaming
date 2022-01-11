#pragma once
#include <string>
#include <vkl/RenderObject.h>
#include <vxt/LinearAlgebra.h>
#include <vkl/PipelineFactory.h>
#include <future>
#include <vkl/UniformBuffer.h>

struct TileData
{
	static constexpr inline float tile_aspect_ratio = 4.f / 3.f;
	static constexpr inline float tile_height = .4f;
	static constexpr inline float tile_width = /*tile_height * tile_aspect_ratio*/tile_height; //NDC will make a square into the aspect ratio - really this should be pixels in the correct AR of the image in case of resized windows
	static constexpr inline float tile_gap_vertical = tile_height / 4.f;
	static constexpr inline float tile_gap_horizontal = tile_width / 8.f;

	static constexpr inline int visible_tiles = (int)(2.f / (tile_height+(tile_gap_vertical*2)) + 1);

	std::string title;
	std::string imageURL;
};

class ImagePlane : public vkl::RenderObject
{
	PIPELINE_TYPE
	static void describePipeline(vkl::PipelineDescription& description);

	struct Vertex
	{
		glm::vec2 pos;
		glm::vec2 uv;
	};

public:
	struct ImageData
	{
		void* data = nullptr;
		uint32_t width, height;
	};
	struct UniformData
	{
		glm::mat4 transform;
		float selected = 0.f;
	};

	ImagePlane() = delete;
	ImagePlane(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager);

	void setImage(const std::string& path);
	void setSelected(bool selected);

	void update(const vkl::Device& device, const vkl::SwapChain& swapChain, vkl::BufferManager& bufferManager);

	~ImagePlane();

	void setScreenPosition(float x, float y);

private:

	void init(const vkl::Device& device, const vkl::SwapChain& swapChain, vkl::BufferManager& bufferManager);

	glm::mat4 _transform;
	bool _selected = false;
	std::vector<Vertex> _verts;
	std::vector<uint32_t> _indices;
	ImageData _imageData;
	std::future<ImageData> _imageFuture;
	std::shared_ptr<vkl::TypedUniform<UniformData>> _uniform;
};


class Tile
{
public:

	TileData& data() { return _data; }
	const TileData& data() const { return _data; }

	void update(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager, bool visible, const glm::vec2& position);
	std::shared_ptr< ImagePlane> _imagePlane;

private:
	std::vector<glm::vec2> _verts;
	TileData _data;
	bool _init = false;
};


#include "Tile.h"
#include <vxt/PNGLoader.h>
#include <vxt/LinearAlgebra.h>
#include <vkl/PipelineFactory.h>
#include <vkl/BufferManager.h>
#include <vkl/Pipeline.h>
#include <vkl/VertexBuffer.h>
#include <vkl/DrawCall.h>
#include <vkl/IndexBuffer.h>

#include "RemoteAccess.h"

#include <iostream>

namespace
{
	constexpr const char* VertShader = R"Shader(

#version 450

layout(binding = 0) uniform MVP {
	mat4 model;
	float selected;
} u_mvp;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

void main() {
	vec2 position = inPosition;

    gl_Position = u_mvp.model * vec4(position, 0.0, 1.0);
    fragUV = inUV;
}
)Shader";
	constexpr const char* FragShader = R"Shader(

#version 450

layout(binding = 0) uniform MVP {
	mat4 model;
	float selected;
} u_mvp;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
	
    outColor = texture(texSampler, fragUV);

	if(u_mvp.selected > 0.f)
	{
		if(fragUV.x < .01 || fragUV.x >.99 || fragUV.y < .01 || fragUV.y > .99)
			outColor = vec4(1,1,0,1);
		else if(fragUV.x < .02 || fragUV.x >.98 || fragUV.y < .02 || fragUV.y > .98)
			outColor = vec4(0,0,0,1);
	}

	gl_FragDepth=.5f;
}
)Shader";

	static const unsigned char whitePixel[4] = { 255, 255,255, 255 };
	static const ImagePlane::ImageData s_defaultImage = { (void*)whitePixel, 1, 1 };
}

REGISTER_PIPELINE(ImagePlane, ImagePlane::describePipeline)



void ImagePlane::describePipeline(vkl::PipelineDescription& description)
{
	description.setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	description.addShaderGLSL(VK_SHADER_STAGE_VERTEX_BIT, VertShader);
	description.addShaderGLSL(VK_SHADER_STAGE_FRAGMENT_BIT, FragShader);

	description.declareVertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(Vertex), offsetof(Vertex, pos));
	description.declareVertexAttribute(0, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(Vertex), offsetof(Vertex, uv));

	description.declareUniform(0, sizeof(glm::mat4));

	description.declareTexture(1);

	//description.setDepthOp(VK_COMPARE_OP_ALWAYS);
}

ImagePlane::ImagePlane(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager)
{
	init(device, swapChain, bufferManager);
	auto texBuff = bufferManager.createTextureBuffer(device, swapChain, s_defaultImage.data, (size_t)s_defaultImage.width, (size_t)s_defaultImage.height, 4);
	addTexture(texBuff, 1);
}

void ImagePlane::setImage(const std::string& url)
{
	assert(!_imageData.data);
	_imageFuture = std::async(std::launch::async | std::launch::deferred, [url]() -> ImageData {
		
		auto jpegData = receiveImageData(url.c_str());
		if (jpegData.empty())
			return {};

		ImageData retData;
		int width{ 0 }, height{ 0 }, channels{ 0 };
		retData.data = vxt::loadJPGData_fromMem(jpegData.data(), jpegData.size(), width, height, channels);
		retData.width = (uint32_t)width;
		retData.height = (uint32_t)height;
		
		return retData;
		});
}

void ImagePlane::setSelected(bool selected)
{
	_selected = selected;
	_uniform->setData({ _transform, _selected ? 1.f : 0.f });
}

void ImagePlane::update(const vkl::Device& device, const vkl::SwapChain& swapChain, vkl::BufferManager& bufferManager)
{
	if (!_imageData.data && _imageFuture.valid())
	{
		bool ready = _imageFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
		if (ready)
		{
			_imageData = _imageFuture.get();
			_imageFuture = {};
			if (!_imageData.data)
				return;

			init(device, swapChain, bufferManager);
			vkl::TextureOptions opt;
			auto texBuff = bufferManager.createTextureBuffer(device, swapChain, _imageData.data, (size_t)_imageData.width, (size_t)_imageData.height, 4, opt);
			addTexture(texBuff, 1);
		}
	}
}

ImagePlane::~ImagePlane()
{
	if (_imageData.data)
		vxt::freeJPGData(_imageData.data);
}

void ImagePlane::setScreenPosition(float x, float y)
{
	_transform = glm::translate(glm::identity<glm::mat4>(), { x - -1.f,y - -1.f,0 });
	_uniform->setData({ _transform, _selected ? 1.f : 0.f });
}

void ImagePlane::init(const vkl::Device& device, const vkl::SwapChain& swapChain, vkl::BufferManager& bufferManager)
{
	reset();
	_verts.clear();
	_indices.clear();

	auto vbo = bufferManager.createVertexBuffer(device, swapChain);

	_verts.push_back({ glm::vec2(-1.0,							-1.0) , glm::vec2(0,0) });  //TL
	_verts.push_back({ glm::vec2(-1.0 + TileData::tile_width,	-1.0) , glm::vec2(1,0) });  //TR
	_verts.push_back({ glm::vec2(-1.0 + TileData::tile_width,	-1.0 + TileData::tile_height) , glm::vec2(1,1) });  //BR
	_verts.push_back({ glm::vec2(-1.0,							-1.0 + TileData::tile_height) , glm::vec2(0,1) });  //BL

	vbo->setData(_verts.data(), sizeof(Vertex), _verts.size());
	addVBO(vbo, 0);

	auto drawCall = std::make_shared<vkl::DrawCall>();

	//CCW wind
	_indices.push_back(3);
	_indices.push_back(1);
	_indices.push_back(0);

	_indices.push_back(3);
	_indices.push_back(2);
	_indices.push_back(1);

	auto indexBuffer = bufferManager.createIndexBuffer(device, swapChain);
	indexBuffer->setData(_indices);
	drawCall->setCount(_indices.size());

	drawCall->setIndexBuffer(indexBuffer);

	addDrawCall(drawCall);

	_uniform = bufferManager.createTypedUniform<UniformData>(device, swapChain);
	_uniform->setData({ _transform, _selected ? 1.f : 0.f });
	addUniform(_uniform, 0);
}

void Tile::update(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager, bool visible, const glm::vec2& position)
{
	if (!_init && visible)
	{
		_imagePlane = std::make_shared<ImagePlane>(device, swapChain, pipelines, bufferManager);
		_imagePlane->setImage(_data.imageURL);
		_imagePlane->setScreenPosition(position.x, position.y);
		_init = true;
	}

	if (_imagePlane)
		_imagePlane->update(device, swapChain, bufferManager);
}

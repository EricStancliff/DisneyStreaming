#include "Background.h"
#include <vkl/BufferManager.h>
#include <vkl/VertexBuffer.h>
#include <vkl/DrawCall.h>
#include <vkl/IndexBuffer.h>

namespace
{
	constexpr const char* VertShader = R"Shader(

#version 450


layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

const float gamma = 2.2f;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = pow(inColor, vec3(gamma));
}
)Shader";
	constexpr const char* FragShader = R"Shader(

#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
	gl_FragDepth = .9f;
}
)Shader";

	//be sure to fix the color space in our shader
	constexpr glm::vec3 DarkBlue = { 5.f / 255.f , 3.f / 255.f , 33.f / 255.f };
	constexpr glm::vec3 LightBlue = { 29.f / 255.f , 62.f / 255.f , 139.f / 255.f };
	constexpr glm::vec3 MediumBlue = { 14.f / 255.f , 20.f / 255.f , 66.f / 255.f };

}

void Background::describePipeline(vkl::PipelineDescription& description)
{
	description.setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	description.addShaderGLSL(VK_SHADER_STAGE_VERTEX_BIT, VertShader);
	description.addShaderGLSL(VK_SHADER_STAGE_FRAGMENT_BIT, FragShader);

	description.declareVertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(Vertex), offsetof(Vertex, pos));
	description.declareVertexAttribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(Vertex), offsetof(Vertex, color));
}

Background::Background(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager)
{
	auto vbo = bufferManager.createVertexBuffer(device, swapChain);

	//Vulkan right hand NDC
	_verts.push_back({ glm::vec2(-1.0, -1.0), DarkBlue });	//TL
	_verts.push_back({ glm::vec2(1.0, -1.0), MediumBlue });	//TR
	_verts.push_back({ glm::vec2(1.0, 1.0), LightBlue });	//BR
	_verts.push_back({ glm::vec2(-1.0, 1.0), MediumBlue });	//BL

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

	drawCall->setCount(_indices.size());
	auto indexBuffer = bufferManager.createIndexBuffer(device, swapChain);
	indexBuffer->setData(_indices);

	drawCall->setIndexBuffer(indexBuffer);

	addDrawCall(drawCall);
}


REGISTER_PIPELINE(Background, Background::describePipeline)


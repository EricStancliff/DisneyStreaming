
#include <vkl/Common.h>

#include <vkl/Instance.h>
#include <vkl/Device.h>
#include <vkl/SwapChain.h>
#include <vkl/Window.h>
#include <vkl/Surface.h>
#include <vkl/BufferManager.h>
#include <vkl/Pipeline.h>
#include <vkl/RenderPass.h>
#include <vkl/CommandDispatcher.h>

#include "Background.h"
#include "TileManager.h"

struct VulkanWindow
{
	vkl::Window window;
	vkl::Surface surface;
	vkl::Device device;
	vkl::SwapChain swapChain;
	vkl::RenderPass mainPass;
	vkl::BufferManager bufferManager;
	vkl::PipelineManager pipelineManager;
	vkl::CommandDispatcher commandDispatcher;
	std::vector<std::shared_ptr<vkl::RenderObject>> renderObjects;

	void cleanUp(const vkl::Instance& instance)
	{
		device.waitIdle();
		for (auto&& ro : renderObjects)
			ro->cleanUp(device);
		renderObjects.clear();
		commandDispatcher.cleanUp(device);
		pipelineManager.cleanUp(device);
		bufferManager.cleanUp(device);
		mainPass.cleanUp(device);
		swapChain.cleanUp(device);
		device.cleanUp();
		surface.cleanUp(instance);
		window.cleanUp();
	}
};

VulkanWindow buildWindow(const vkl::Instance& instance, const std::string& title)
{
	vkl::Window window(1080, 720, title.c_str());
	vkl::Surface surface(instance, window);
	vkl::Device device(instance, surface);

	vkl::SwapChainOptions swapChainOptions{};
	swapChainOptions.swapChainExtent.width = window.getWindowSize().width;
	swapChainOptions.swapChainExtent.height = window.getWindowSize().height;
	swapChainOptions.presentMode = VK_PRESENT_MODE_FIFO_KHR;  //VSync
	vkl::SwapChain swapChain(device, surface, swapChainOptions);

	vkl::RenderPassOptions mainPassOptions;
	mainPassOptions.clearColor = { 0.2f, 0.2f, 0.2f, 1.f };
	vkl::RenderPass mainPass(device, swapChain, mainPassOptions);

	swapChain.registerRenderPass(device, mainPass);

	vkl::BufferManager bufferManager(device, swapChain);
	vkl::PipelineManager pipelineManager(device, swapChain, mainPass);

	vkl::CommandDispatcher commandDispatcher(device, swapChain);

	return { std::move(window), std::move(surface), std::move(device), std::move(swapChain),
		std::move(mainPass), std::move(bufferManager), std::move(pipelineManager), std::move(commandDispatcher),
		{}
	};
}

void updateWindow(VulkanWindow& window)
{
	window.bufferManager.update(window.device, window.swapChain);
	window.commandDispatcher.processUnsortedObjects(window.renderObjects, window.device, window.pipelineManager, window.mainPass, window.swapChain, window.swapChain.frameBuffer(window.swapChain.frame()), window.swapChain.swapChainExtent());
	window.swapChain.swap(window.device, window.surface, window.commandDispatcher, window.mainPass, window.window.getWindowSize());
}

bool seedRandom()
{
	std::srand((unsigned int)std::time(0));
	return true;
}
float fRand(float fMin, float fMax)
{
	float f = (float)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}
glm::vec3 randomPosition()
{
	static bool seeded = seedRandom();
	if (seeded)
	{
		glm::vec3 position;
		position.x = fRand(-50, 50);
		position.y = 0;
		position.z = fRand(-50, 50);
		return position;
	}
	return {};
}

int main(int argc, char* argv[])
{
	vkl::Instance instance("disney_streaming", true);

	VulkanWindow window = buildWindow(instance, "Disney Streaming");

	auto bg = std::make_shared<Background>(window.device, window.swapChain, window.pipelineManager, window.bufferManager);
	window.renderObjects.push_back(bg);

	TileManager mgr;

	while (!window.window.shouldClose())
	{
		window.window.clearLastFrame();
		vkl::Window::pollEventsForAllWindows();
		window.swapChain.prepNextFrame(window.device, window.surface, window.commandDispatcher, window.mainPass, window.window.getWindowSize());

		mgr.update(window.device, window.swapChain, window.pipelineManager, window.bufferManager, window.renderObjects, window.window);

		updateWindow(window);
		//window.bufferManager.cleanUnusedBuffers(window.device);
	}

	window.cleanUp(instance);
	instance.cleanUp();
	vkl::Window::cleanUpWindowSystem();
}
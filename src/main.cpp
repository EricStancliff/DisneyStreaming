
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

int main(int argc, char* argv[])
{
	vkl::Instance instance("disney_streaming", false);

	vkl::Window window(1080, 720, "Disney Streaming");
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
	std::vector<std::shared_ptr<vkl::RenderObject>> renderObjects;


	auto bg = std::make_shared<Background>(device, swapChain, pipelineManager, bufferManager);
	renderObjects.push_back(bg);

	TileManager mgr;

	while (!window.shouldClose())
	{
		window.clearLastFrame();
		vkl::Window::pollEventsForAllWindows();
		swapChain.prepNextFrame(device, surface, commandDispatcher, mainPass, window.getWindowSize());

		mgr.update(device, swapChain, pipelineManager, bufferManager, renderObjects, window);

		bufferManager.update(device, swapChain);
		commandDispatcher.processUnsortedObjects(renderObjects, device, pipelineManager, mainPass, swapChain, swapChain.frameBuffer(swapChain.frame()), swapChain.swapChainExtent());
		swapChain.swap(device, surface, commandDispatcher, mainPass, window.getWindowSize());
		//window.bufferManager.cleanUnusedBuffers(window.device);
	}

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

	instance.cleanUp();
	vkl::Window::cleanUpWindowSystem();
}
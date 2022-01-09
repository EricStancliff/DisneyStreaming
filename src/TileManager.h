#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Tile.h"

struct Row
{
	std::string setId;
	std::string title;
	std::vector<Tile> _tiles;
	bool isRefSet{ false };
};

struct Grid
{
	std::vector<Row> _rows;
};

class TileManager
{
public:
	TileManager();
	void update(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager, std::vector<std::shared_ptr<vkl::RenderObject>>& renderObjects);

private:
	void parse(const nlohmann::json& json);

	std::string _jsonString;
	nlohmann::json _mainPageJson;
	Grid _grid;
	glm::vec2 _screenOffset{ 0,0 };
	bool _init = false;
};
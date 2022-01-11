#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Tile.h"
#include "TextBox.h"

struct Row
{
	std::string setId;
	std::string title;
	std::vector<Tile> _tiles;
	bool isRefSet{ false };
	std::shared_ptr<TextBox> textBox;
};

struct Grid
{
	std::vector<Row> _rows;
};

class TileManager
{
public:
	TileManager();
	void update(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager, std::vector<std::shared_ptr<vkl::RenderObject>>& renderObjects, const vkl::Window& window);

private:
	void parse(const nlohmann::json& json);

	bool isTileVisible(int yOffset, int xOffset, int x, int y);
	void loadRefSet(Row& load);

	std::string _jsonString;
	nlohmann::json _mainPageJson;
	Grid _grid;
	int _screenOffset = 0;
	bool _init = false;
	glm::ivec2 _highlighted{ 0 ,0 };
};
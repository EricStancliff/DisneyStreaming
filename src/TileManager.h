#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Tile.h"
#include "TextBox.h"
#include <future>

struct Row
{
	std::string setId;
	std::string title;
	std::vector<Tile> _tiles;
	bool isRefSet{ false };
	std::shared_ptr<TextBox> textBox;
	int offset = 0;
	float animatedOffset = 0.f;

	std::future<std::string> _futureJSON;

	void updateAnimation();
	void resetAnimation(float multiplier);

	Row() = default;
	~Row() = default;
	Row& operator=(Row&&) noexcept = default;
	Row(Row&&) = default;
	Row& operator=(const Row&) = delete;
	Row(const Row&) = delete;
private:
	std::chrono::steady_clock::time_point _animationBegin;
	float _animationOffsetBegin = 0.f;
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

	bool isRowVisible(int yOffset, int y);

	bool loadRefSet(Row& load);
	void fixOffset(Row& row);
	void fixOffset();
	void updateAnimation();
	void resetAnimation(float multiplier);

	std::chrono::steady_clock::time_point _animationBegin;
	float _animationOffsetBegin = 0.f;


	std::string _jsonString;
	nlohmann::json _mainPageJson;
	Grid _grid;
	int _screenOffset = 0;
	float _animatedOffset = 0.f;
	glm::ivec2 _highlighted{ 0 ,0 };

	std::shared_ptr<TextBox> _popup;
};
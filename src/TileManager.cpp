#include "TileManager.h"
#include "RemoteAccess.h"
#include <iostream>
#include <vkl/Window.h>
#include <vkl/Event.h>

namespace {
	constexpr const char* JSONHomePage = "https://cd-static.bamgrid.com/dp-117731241344/home.json";
	constexpr const char* RefPrefix = "https://cd-static.bamgrid.com/dp-117731241344/sets/";

	constexpr std::string_view RowClassName = "CuratedSet";
	constexpr std::string_view RowClassNameRefSet = "SetRef";
	constexpr std::string_view SeriesClassName = "DmcSeries";
	constexpr std::string_view VideoClassName = "DmcVideo";
	static const std::string ClassTypeName = "type";

	void _parse(const nlohmann::json& json, Grid& grid, Row* row)
	{
		if (json.contains(ClassTypeName))
		{
			std::string classType = json[ClassTypeName].get<std::string>();
			if (classType == RowClassName)
			{
				Row row_inner;
				row_inner.title = json["text"]["title"]["full"]["set"]["default"]["content"]; //not permanant
				for (auto&& child : json)
					_parse(child, grid, &row_inner);
				grid._rows.push_back(row_inner);
				return;
			}
			if (classType == RowClassNameRefSet)
			{
				Row row_inner;
				row_inner.title = json["text"]["title"]["full"]["set"]["default"]["content"]; //not permanant
				row_inner.isRefSet = true;
				row_inner.setId = json["refId"];
				row_inner.setId = std::string(RefPrefix) + row_inner.setId + ".json";
				grid._rows.push_back(row_inner);
				return;
			}
			else if (classType == SeriesClassName)
			{
				Tile tile_inner;
				tile_inner.data().title = json["text"]["title"]["full"]["series"]["default"]["content"]; //not permanant
				const auto& tileObj = json["image"]["tile"];
				//if (tileObj.contains("0.75"))
				//{
				//	tile_inner.data().imageURL = tileObj["0.75"]["series"]["default"]["url"];
				//}
				//else if (!tileObj.empty())
				//{
				//	tile_inner.data().imageURL = tileObj.front()["series"]["default"]["url"];
				//}
				if (!tileObj.empty())
				{
					tile_inner.data().imageURL = tileObj.back()["series"]["default"]["url"];
				}

				if (row)
					row->_tiles.push_back(std::move(tile_inner));
				return;
			}
			else if (classType == VideoClassName)
			{
				Tile tile_inner;
				tile_inner.data().title = json["text"]["title"]["full"]["program"]["default"]["content"]; //not permanant
				const auto& tileObj = json["image"]["tile"];
				//if (tileObj.contains("0.75"))
				//{
				//	tile_inner.data().imageURL = tileObj["0.75"]["program"]["default"]["url"];
				//}
				//else if (!tileObj.empty())
				//{
				//	tile_inner.data().imageURL = tileObj.front()["program"]["default"]["url"];
				//}
				if (!tileObj.empty())
				{
					tile_inner.data().imageURL = tileObj.back()["program"]["default"]["url"];
				}
				if (row)
					row->_tiles.push_back(std::move(tile_inner));
				return;
			}
		}

		if (!json.is_object() && !json.is_array())
			return;

		for (auto&& child : json.items())
			_parse(child.value(), grid, row);

	} 
}

TileManager::TileManager()
{
	_jsonString = receiveStringResource(JSONHomePage);
	_mainPageJson = nlohmann::json::parse(_jsonString);
	parse(_mainPageJson);
	for (auto&& row : _grid._rows)
	{
		std::cout << "Row: " << row.title << ": " << std::endl;
		for (auto&& tile : row._tiles)
		{
			std::cout << "Title: " << tile.data().title << ", ";
		}
		std::cout << std::endl;
	}
}

void TileManager::update(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager, std::vector<std::shared_ptr<vkl::RenderObject>>& renderObjects, const vkl::Window& window)
{
	for (auto&& event : window.events())
	{
		if (event->getType() == vkl::EventType::KEY_DOWN)
		{
			auto keyDown = static_cast<const vkl::KeyDownEvent*>(event.get());
			switch (keyDown->key)
			{
			case vkl::Key::KEY_DOWN:
				_highlighted.y = std::min(std::max(0, _highlighted.y + 1), (int)_grid._rows.size());
				if (_grid._rows.size() > _highlighted.y)
				{
					_highlighted.x = std::min(std::max(0, _highlighted.x), (int)_grid._rows[_highlighted.y]._tiles.size());
				}
				break;
			case vkl::Key::KEY_UP:
				_highlighted.y = std::min(std::max(0, _highlighted.y - 1), (int)_grid._rows.size());
				if (_grid._rows.size() > _highlighted.y)
				{
					_highlighted.x = std::min(std::max(0, _highlighted.x), (int)_grid._rows[_highlighted.y]._tiles.size());
				}
				break;
			case vkl::Key::KEY_LEFT:
			{
				if (_grid._rows.size() > _highlighted.y)
				{
					_highlighted.x = std::min(std::max(0, _highlighted.x - 1), (int)_grid._rows[_highlighted.y]._tiles.size());
				}
			}
			break;
			case vkl::Key::KEY_RIGHT:
			{
				if (_grid._rows.size() > _highlighted.y)
				{
					_highlighted.x = std::min(std::max(0, _highlighted.x + 1), (int)_grid._rows[_highlighted.y]._tiles.size());
				}
			}
			break;
			}
		}
	}


	float y = -1.f + TileData::tile_gap_vertical;

	int y_pos = 0;

	for (auto& row : _grid._rows)
	{
		if (row._tiles.empty())
		{
			if (isTileVisible(_screenOffset, 0, 0, y_pos))
			{
				loadRefSet(row);
			}
		}
		if (row._tiles.empty())
		{
			continue;
		}

		float x = -1.f + TileData::tile_gap_horizontal;
		int x_pos = 0;
		if (!row.title.empty())
		{
			if (!row.textBox)
			{
				row.textBox = std::make_shared<TextBox>(device, swapChain, pipelines, bufferManager);
				row.textBox->setText(row.title);
				row.textBox->setPosition({ x,y });
				renderObjects.push_back(row.textBox);
			}
			row.textBox->update({ 0,0, window.getWindowSize().width, window.getWindowSize().height });
			y += TileData::tile_gap_horizontal; //on purpose;
		}

		for (auto&& tile : row._tiles)
		{
			if(tile._imagePlane)
				tile._imagePlane->setSelected(_highlighted.x == x_pos && _highlighted.y == y_pos);
			tile.update(device, swapChain, pipelines, bufferManager, true, { x,y });
			x += TileData::tile_width + TileData::tile_gap_horizontal;
			x_pos++;
		}
		y += TileData::tile_height + TileData::tile_gap_vertical;
		y_pos++;
	}

	if (!_init)
	{
		for (auto&& row : _grid._rows)
		{
			for (auto&& tile : row._tiles)
			{
				renderObjects.push_back(tile._imagePlane);
			}
		}
		_init = true;
	}
}

void TileManager::parse(const nlohmann::json& json)
{
	_parse(json, _grid, nullptr);
}

bool TileManager::isTileVisible(int yOffset, int xOffset, int x, int y)
{
	if (x >= xOffset && x <= xOffset + TileData::visible_tiles)
		return true;
	if (y >= yOffset && y <= yOffset + TileData::visible_tiles)
		return true;
	return false;
}

void TileManager::loadRefSet(Row& load)
{
	if (load.setId.empty())
		return;
	auto refSetJsonStr = receiveStringResource(load.setId.c_str());
	if (refSetJsonStr.empty())
		return;
	auto refSetJson = nlohmann::json::parse(refSetJsonStr);
	_parse(refSetJson["data"]["CuratedSet"]["items"], _grid, &load);
	load.setId = "";
}


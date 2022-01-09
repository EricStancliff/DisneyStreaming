#include "TileManager.h"
#include "RemoteAccess.h"
#include <iostream>

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

void TileManager::update(const vkl::Device& device, const vkl::SwapChain& swapChain, const vkl::PipelineManager& pipelines, vkl::BufferManager& bufferManager, std::vector<std::shared_ptr<vkl::RenderObject>>& renderObjects)
{
	float y = -1.f + TileData::tile_gap_vertical;

	for (auto&& row : _grid._rows)
	{
		if (row._tiles.empty())
			continue;
		float x = -1.f + TileData::tile_gap_horizontal;
		for (auto&& tile : row._tiles)
		{
			tile.update(device, swapChain, pipelines, bufferManager, true, { x,y });
			x += TileData::tile_width + TileData::tile_gap_horizontal;
		}
		y += TileData::tile_height + TileData::tile_gap_vertical;
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


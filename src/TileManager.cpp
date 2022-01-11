#include "TileManager.h"
#include "RemoteAccess.h"
#include <iostream>
#include <vkl/Window.h>
#include <vkl/Event.h>

namespace {
	constexpr const char* JSONHomePage = "https://cd-static.bamgrid.com/dp-117731241344/home.json";
	constexpr const char* RefPrefix = "https://cd-static.bamgrid.com/dp-117731241344/sets/";

	constexpr std::string_view RowClassName = "CuratedSet";
	constexpr std::string_view RowClassNameTrending = "TrendingSet";
	constexpr std::string_view RowClassNameRefSet = "SetRef";
	constexpr std::string_view SeriesClassName = "DmcSeries";
	constexpr std::string_view VideoClassName = "DmcVideo";
	constexpr std::string_view CollectionClassName = "StandardCollection";
	static const std::string ClassTypeName = "type";

	void _parse(const nlohmann::json& json, Grid& grid, Row* row)
	{
		if (json.contains(ClassTypeName))
		{
			std::string classType = json[ClassTypeName].get<std::string>();
			if (classType == RowClassName || classType == RowClassNameTrending || classType == "PersonalizedCuratedSet")
			{
				Row row_inner;
				row_inner.title = json["text"]["title"]["full"]["set"]["default"]["content"]; //not permanant
				for (auto&& child : json)
					_parse(child, grid, &row_inner);
				grid._rows.push_back(std::move(row_inner));
				return;
			}
			else if (classType == RowClassNameRefSet)
			{
				Row row_inner;
				row_inner.title = json["text"]["title"]["full"]["set"]["default"]["content"]; //not permanant
				row_inner.isRefSet = true;
				row_inner.setId = json["refId"];
				row_inner.setId = std::string(RefPrefix) + row_inner.setId + ".json";
				grid._rows.push_back(std::move(row_inner));
				return;
			}
			else if (classType == SeriesClassName)
			{
				Tile tile_inner;
				tile_inner.data().title = json["text"]["title"]["full"]["series"]["default"]["content"]; //not permanant
				const auto& tileObj = json["image"]["tile"];
				if (!tileObj.empty())
				{
					tile_inner.data().imageURL = tileObj.back()["series"]["default"]["url"];
				}

				tile_inner.data().type = "Series";
				tile_inner.data().language = json["text"]["title"]["full"]["series"]["default"]["language"];
				if(json["ratings"].size() > 0)
					tile_inner.data().rating = json["ratings"].front()["value"];

				if (row)
					row->_tiles.push_back(std::move(tile_inner));
				return;
			}
			else if (classType == VideoClassName)
			{
				Tile tile_inner;
				tile_inner.data().title = json["text"]["title"]["full"]["program"]["default"]["content"]; //not permanant
				const auto& tileObj = json["image"]["tile"];
				if (!tileObj.empty())
				{
					tile_inner.data().imageURL = tileObj.back()["program"]["default"]["url"];
				}
				tile_inner.data().type = "Movie";
				tile_inner.data().language = json["text"]["title"]["full"]["program"]["default"]["language"];
				if (json["ratings"].size() > 0)
					tile_inner.data().rating = json["ratings"].front()["value"];

				if (row)
					row->_tiles.push_back(std::move(tile_inner));
				return;
			}
			else if (classType == CollectionClassName && !json.contains("containers"))
			{
				Tile tile_inner;
				tile_inner.data().title = json["text"]["title"]["full"]["collection"]["default"]["content"]; //not permanant

				const auto& tileObj = json["image"]["tile"];
				if (!tileObj.empty())
				{
					tile_inner.data().imageURL = tileObj.back()["default"]["default"]["url"];
				}
				tile_inner.data().type = "Collection";
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
			{
				int currentOffset = _grid._rows[_highlighted.y].offset;
				_highlighted.y = std::min(std::max(0, _highlighted.y + 1), (int)_grid._rows.size()-1);
				_highlighted.x = std::min(std::max(0, _highlighted.x), (int)_grid._rows[_highlighted.y]._tiles.size()) - (currentOffset - _grid._rows[_highlighted.y].offset);
				fixOffset();
			}
			break;
			case vkl::Key::KEY_UP:
			{
				int currentOffset = _grid._rows[_highlighted.y].offset;
				_highlighted.y = std::min(std::max(0, _highlighted.y - 1), (int)_grid._rows.size()-1);
				_highlighted.x = std::min(std::max(0, _highlighted.x), (int)_grid._rows[_highlighted.y]._tiles.size()) - (currentOffset - _grid._rows[_highlighted.y].offset);
				fixOffset();
			}
			break;
			case vkl::Key::KEY_LEFT:
			{
				if (_grid._rows.size() > _highlighted.y)
				{
					auto& row = _grid._rows[_highlighted.y];
					_highlighted.x = std::max(0, std::min(_highlighted.x - 1, (int)_grid._rows[_highlighted.y]._tiles.size()-1));
					fixOffset(row);
				}
			}
			break;
			case vkl::Key::KEY_RIGHT:
			{
				if (_grid._rows.size() > _highlighted.y)
				{
					auto& row = _grid._rows[_highlighted.y];
					_highlighted.x = std::max(0, std::min(_highlighted.x + 1, (int)_grid._rows[_highlighted.y]._tiles.size() - 1));
					fixOffset(row);
				}
			}
			break;
			case vkl::Key::KEY_ENTER:
			{
				if (!_popup)
				{
					std::string text;
					const auto& tile = _grid._rows[_highlighted.y]._tiles[_highlighted.x];
					text += "Title: \n";
					text += tile.data().title + "\n";
					text += "Type: \n";
					text += tile.data().type + "\n";
					text += "Language: \n";
					text += tile.data().language + "\n";
					text += "Rating: \n";
					text += tile.data().rating + "\n";

					_popup = std::make_shared<TextBox>(device, swapChain, pipelines, bufferManager);
					_popup->setBackground({ 0,0,0,1 });
					_popup->setPosition({ -.5, -.5 });
					_popup->setText(text);
					renderObjects.push_back(_popup);
				}
			}
			break;
			case vkl::Key::KEY_ESCAPE:
			{
				if (_popup)
				{
					renderObjects.erase(
						std::remove(renderObjects.begin(), renderObjects.end(), _popup), renderObjects.end());
					_popup = nullptr;
				}
			}
			break;
			}
		}
	}

	updateAnimation();

	float y = -1.f + TileData::tile_gap_vertical - (_screenOffset * (TileData::tile_height + (TileData::tile_gap_vertical+TileData::tile_gap_horizontal))) + _animatedOffset;

	int y_pos = 0;

	for (auto& row : _grid._rows)
	{
		if (row._tiles.empty())
		{
			if (isRowVisible(_screenOffset, y_pos))
			{
				if (loadRefSet(row))
				{
					std::cout << "Populated Ref Set" << std::endl;
				}
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
				renderObjects.push_back(row.textBox);
			}
			row.textBox->setPosition({ x,y });
			row.textBox->update({ 0,0, window.getWindowSize().width, window.getWindowSize().height });
			y += TileData::tile_gap_horizontal; //on purpose;
		}

		row.updateAnimation();
		x -= (row.offset * (TileData::tile_width + TileData::tile_gap_horizontal)) - row.animatedOffset;

		for (auto&& tile : row._tiles)
		{
			if(tile._imagePlane)
				tile._imagePlane->setSelected(_highlighted.x == x_pos && _highlighted.y == y_pos);
			tile.update(device, swapChain, pipelines, bufferManager, { x,y }, renderObjects);
			x += TileData::tile_width + TileData::tile_gap_horizontal;
			x_pos++;
		}
		y += TileData::tile_height + TileData::tile_gap_vertical;
		y_pos++;
	}

	if (_popup)
	{
		_popup->update({ 0,0, window.getWindowSize().width, window.getWindowSize().height });
	}
}

void TileManager::parse(const nlohmann::json& json)
{
	_parse(json, _grid, nullptr);
}

bool TileManager::isRowVisible(int yOffset, int y)
{
	if (y >= yOffset && y < yOffset + TileData::visible_tiles)
		return true;
	return false;
}

bool TileManager::loadRefSet(Row& load)
{
	if (load.setId.empty())
		return false;

	if (!load._futureJSON.valid())
	{
		load._futureJSON = std::move(std::async(std::launch::async | std::launch::deferred, [url= load.setId]() -> std::string {
			return receiveStringResource(url.c_str());
			}));
		return false;
	}
	
	if (load._futureJSON.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
		return false;

	load.setId = "";

	std::string refSetJsonStr = load._futureJSON.get();
	load._futureJSON = {};

	if (refSetJsonStr.empty())
		return false;
	auto refSetJson = nlohmann::json::parse(refSetJsonStr);
	if(refSetJson["data"].contains("CuratedSet"))
		_parse(refSetJson["data"]["CuratedSet"]["items"], _grid, &load);
	else if (refSetJson["data"].contains("TrendingSet"))
		_parse(refSetJson["data"]["TrendingSet"]["items"], _grid, &load);
	else if (refSetJson["data"].contains("PersonalizedCuratedSet"))
		_parse(refSetJson["data"]["PersonalizedCuratedSet"]["items"], _grid, &load);
	assert(load._tiles.size());
	return true;
}

void TileManager::fixOffset(Row& row)
{
	auto oldOffset = row.offset;
	if (_highlighted.x - row.offset > TileData::visible_tiles_horizontal - 2)
		row.offset = std::min(row.offset + 1, (int)row._tiles.size() - TileData::visible_tiles_horizontal + 1);
	else if (_highlighted.x <= row.offset)
		row.offset = std::max(_highlighted.x, 0);
	if (row.offset != oldOffset)
		row.resetAnimation(row.offset > oldOffset ? 1.0f : -1.0f);
}

void TileManager::fixOffset()
{
	auto oldOffset = _screenOffset;
	if (_highlighted.y - _screenOffset > TileData::visible_tiles - 2)
		_screenOffset = std::min(_screenOffset + 1, (int)_grid._rows.size() - TileData::visible_tiles + 1);
	else if (_highlighted.y <= _screenOffset)
		_screenOffset = std::max(_highlighted.y, 0);
	if (_screenOffset != oldOffset)
		resetAnimation(_screenOffset > oldOffset ? 1.0f : -1.0f);
}

void TileManager::updateAnimation()
{
	constexpr const float animationTime = 300;
	float delta = (float)std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - _animationBegin)).count();
	if (delta >= animationTime)
	{
		_animatedOffset = 0.f;
		return;
	}


	_animatedOffset =
		glm::mix(_animationOffsetBegin, 0.f, delta / animationTime);
}

void TileManager::resetAnimation(float multiplier)
{
	constexpr const float animationOffsetBegin = (TileData::tile_height + TileData::tile_gap_vertical);
	_animatedOffset = animationOffsetBegin * multiplier;
	_animationBegin = std::chrono::steady_clock::now();
	_animationOffsetBegin = _animatedOffset;
}


void Row::updateAnimation()
{
	constexpr const float animationTime = 300;	
	float delta = (float)std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::steady_clock::now() - _animationBegin)).count();
	if (delta >= animationTime)
	{
		animatedOffset = 0.f;
		return;
	}
		

	animatedOffset =
		glm::mix(_animationOffsetBegin, 0.f, delta / animationTime);
}

void Row::resetAnimation(float multiplier)
{
	constexpr const float animationOffsetBegin = (TileData::tile_width + TileData::tile_gap_horizontal);
	animatedOffset = animationOffsetBegin * multiplier;
	_animationBegin = std::chrono::steady_clock::now();
	_animationOffsetBegin = animatedOffset;
}

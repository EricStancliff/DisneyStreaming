set(vkl_include_dir ${CMAKE_SOURCE_DIR}/include)


add_executable(disney_streaming
Background.cpp
main.cpp
RemoteAccess.cpp
Tile.cpp
TileManager.cpp
Background.h
RemoteAccess.h
Tile.h
TileManager.h
)


target_link_libraries(disney_streaming PUBLIC vkl vxt CURL::libcurl ZLIB::ZLIB nlohmann_json nlohmann_json::nlohmann_json)

target_include_directories(disney_streaming PUBLIC ${vkl_include_dir})

target_compile_definitions(disney_streaming PRIVATE -DVKL_DATA_DIR="${VKL_DATA_DIR}")

Configure_App(disney_streaming)
#pragma once
#include <string>
#include <vector>

std::string receiveStringResource(const char* uri);

std::vector<unsigned char> receiveImageData(const char* uri);
#include "Console.h"
#include <iostream>
#include <chrono>

static const char* lastProgressTitle = nullptr;
std::chrono::high_resolution_clock::time_point lastOutput;

void Console::info(const std::string& text)
{
	if (PrintInfo)
		write("INF: " + text);
}

void Console::warning(const std::string& text)
{
	if (PrintWarning)
		write("WAR: " + text);
}

void Console::error(const std::string& text)
{
	if (PrintError)
		write("ERR: " + text);
}

void Console::progress(const char* what, size_t curCount, size_t totalCount)
{
	if (!PrintInfo) return;

	// print finished message
	if(curCount == totalCount)
	{
		write("INF: " + std::string(what) + " completed");
		lastProgressTitle = nullptr;
		return;
	}

	const auto now = std::chrono::high_resolution_clock::now();
	if(lastProgressTitle != what || std::chrono::duration_cast<std::chrono::seconds>(now - lastOutput).count() > 2)
	{
		lastOutput = now;
		lastProgressTitle = what;
		write((std::string("INF: ") + what) + " " + std::to_string(curCount) + "/" + std::to_string(totalCount));
	}
}

void Console::write(std::string text)
{
	static std::string lastText;
	static size_t repeatCount = 0;

	if(text == lastText)
	{
		if (repeatCount == 1) // print repeat count prefix
			std::cerr << " x";
		else // remove old repeat count
			for (size_t i = 0, count = std::to_string(repeatCount).length(); i != count; ++i)
				std::cerr << '\b';

		++repeatCount;
		std::cerr << std::to_string(repeatCount);
	}
	else
	{
		lastText = std::move(text);
		repeatCount = 1;
		std::cerr << '\n' << lastText;
	}
}

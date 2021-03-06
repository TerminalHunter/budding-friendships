#pragma once

#include <string>

namespace bf
{
	enum Direction
	{
		Up,
		Down,
		Left,
		Right
	};

	const std::string strDirection( Direction d );
}

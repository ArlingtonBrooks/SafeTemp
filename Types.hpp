#ifndef SAFETEMP_TYPES_HPP
#define SAFETEMP_TYPES_HPP

#include <cmath>
#include <type_traits>

/** @brief A data structure containing a user's UI selection */
struct Selection {
	int Row = 0;
	int Col = 0;
};

template <typename DataType>
struct Point
{
	static_assert(std::is_arithmetic<DataType>::value);
	DataType x;
	DataType y;
};

using WinSize = Point<int>;

template <typename DataType>
struct Rect
{
	static_assert(std::is_arithmetic<DataType>::value);
	DataType x, y; //lower left corner
	DataType w, h;
};

template <typename DataType>
struct Line
{
	Point<DataType> pt1;
	Point<DataType> pt2;

	DataType Length() {
		DataType len = (DataType)(std::pow(pt2.x - pt1.x,2) + std::pow(pt2.y-pt1.y,2));
		return sqrt(len);
	}
};

#endif //SAFETEMP_TYPES_HPP

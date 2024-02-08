#ifndef SAFETEMP_TYPES_HPP
#define SAFETEMP_TYPES_HPP

#include <cmath>
#include <ctime>
#include <string>
#include <type_traits>

struct TempPair {
	std::string Name;
	float Temp;
};

struct SensorDetailLine {
	time_t Time;
	std::string FriendlyName;
	std::string Command;
	TempPair TempData;
	float CritTemp;
};

class SensorPreferences {
private:
	std::string m_SensorName = "";
	std::string m_FriendlyName = "";
	std::string m_Command;
	float m_CriticalTemp;
public:
	SensorPreferences(std::string const &RawName) {
		m_SensorName = RawName;
		m_FriendlyName = m_SensorName;
		m_Command = "";
		m_CriticalTemp = -273.15;
	}
	void SetFriendlyName(std::string const &NewName) {
		m_FriendlyName = NewName;
	}
	std::string GetFriendlyName() const {
		return m_FriendlyName;
	}
	void SetCommand(std::string const &Cmd) {
		m_Command = Cmd;
	}
	std::string GetCommand() const {
		return m_Command;
	}
	void SetCriticalTemp(float Temp) {
		m_CriticalTemp = Temp;
	}
	float GetCriticalTemp() const {
		return m_CriticalTemp;
	}
};

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

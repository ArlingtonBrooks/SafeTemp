#ifndef SAFETEMP_TYPES_HPP
#define SAFETEMP_TYPES_HPP

#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>
#include <type_traits>

//TODO: I should make a separate structure for just temperature and time; otherwise, I'm storing too much extra detail for each temperature reading;
/** @brief A simple temperature + name structure */
struct TempPair {
	std::string Name; ///<Non-friendly sensor name
	float Temp;       ///<Sensor temperature
};

/** @brief A data structure for a point on the chart to be plotted */
struct SensorDetailLine {
	std::time_t Time;          ///<Time of current reading
	std::string FriendlyName;  ///<Friendly name of the sensor under consideration
	std::string Command;       ///<Command to be executed if the line exceeds the critical temperature
	TempPair TempData;         ///<Temperature data
	float CritTemp;            ///<The critical temperature
	char Symbol;               ///<The symbol to print
	unsigned char Colour;      ///<The colour to be printed
};

/** @brief Get the maximum temperature in a SensorDetailLine vector */
template <typename Iter_Type>
float GetMaxTemp(Iter_Type const Begin, Iter_Type const End) {
	auto LessThan = [](SensorDetailLine const &L1, SensorDetailLine const &L2){
		return (L2.TempData.Temp > L1.TempData.Temp);
	};
	return std::max_element(Begin,End,LessThan)->TempData.Temp;
}

/** @brief Get the minimum temperature in a SensorDetailLine vector */
template <typename Iter_Type>
float GetMinTemp(Iter_Type const Begin, Iter_Type const End) {
	auto LessThan = [](SensorDetailLine const &L1, SensorDetailLine const &L2){
		return (L2.TempData.Temp > L1.TempData.Temp);
	};
	return std::min_element(Begin,End,LessThan)->TempData.Temp;
}

/** @brief Get the maximum time location for a SensorDetailLine vector */
template <typename Iter_Type>
std::time_t GetMaxTime(Iter_Type const Begin, Iter_Type const End) {
	auto LessThan = [](SensorDetailLine const &L1, SensorDetailLine const &L2){
		return (L2.Time > L1.Time);
	};
	return std::max_element(Begin,End,LessThan)->Time;
}

/** @brief Get the minimum time location for a SensorDetailLine vector */
template <typename Iter_Type>
std::time_t GetMinTime(Iter_Type const Begin, Iter_Type const End) {
	auto LessThan = [](SensorDetailLine const &L1, SensorDetailLine const &L2){
		return (L2.Time > L1.Time);
	};
	return std::min_element(Begin,End,LessThan)->Time;
}

/** @brief Storage of user's sensor preferences (friendly name, critical temp, etc)
 * This is preferable for tracking data in the UI rather than using the SensorDetailLine
 * @TODO Replace SensorDetailLine data with this 
 */
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

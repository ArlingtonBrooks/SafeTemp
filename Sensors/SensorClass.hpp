#ifndef SENSORCLASS_HPP_
#define SENSORCLASS_HPP_

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

#include "../Types.hpp"

/** @brief Interface for temperature sensors */
class temperature_sensor_set {
public:
	/** @brief Return a vector of all temperatures */
	virtual std::vector<TempPair> GetAllTemperatures() = 0;
	/** @brief Get a temperature for a specific sensor name (raw sensor name, not friendly) */
	virtual float GetTemperature(std::string const &SensorName) = 0;
	/** @brief Get the temperature of the sensor at the given index */
	virtual float GetTemperature(unsigned index) = 0;
	/** @brief Get the number of sensors stored in this object */
	virtual unsigned GetNumberOfSensors() const = 0;
	virtual ~temperature_sensor_set() = default;
};

/** @brief Test sensor class (reference implementation) */
class test_sensor : public temperature_sensor_set {
private:
	char const CharSet[96] = "`1234567890-=~!@#$%^&*()_+qwertyuiop[]\\asdfghjkl;'zxcvbnm,./ QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?";
	std::vector<std::string> SensorNames;
public:
	test_sensor(unsigned num) {
		const unsigned NCharSet = sizeof(CharSet) - 1;
        	for (unsigned j = 0; j != num; j++) {
			std::string SensorName = "";
		        for (unsigned i = 0; i != 12; i++) { //Generate random sensor names
		                unsigned idx = NCharSet % (i % (j+1) + 1);
		                idx += i % (j+1);
		                idx += (i + j + 1) % NCharSet;
		                idx = (idx >= NCharSet) ? NCharSet-1 : idx;
				SensorName += CharSet[NCharSet - idx];
		        }
			SensorNames.push_back(SensorName);
        	}
	}
	virtual std::vector<TempPair> GetAllTemperatures() override {
		std::vector<TempPair> ret;
		for (auto const &i : SensorNames) {
			TempPair TP;
			TP.Name = i;
			TP.Temp = 20.0;
			for (auto const &j : i) {
				TP.Temp += (double)j / 100.0;
			}
			ret.push_back(TP);
		}
		return ret;
	}
	virtual float GetTemperature(std::string const &SensorName) override {
		float ret = 20.0;
		for (auto const &j : SensorName) {
			ret += (double)j / 100.0;
		}
		return ret;
	}
	virtual float GetTemperature(unsigned index) override {
		return 20.0 + (double)index;
	}
	virtual unsigned GetNumberOfSensors() const override {
		return SensorNames.size();
	}
};

//TODO: put nvidia sensors here;

/** @brief A holder class for lm_sensors temperature chips */
class lm_sensor : public temperature_sensor_set {
private:
	/** @brief A data structure for an lm_sensors sensor */
	struct lm_SensorPair {
		std::string Name;
		sensors_chip_name const *Chip;
		sensors_feature const *Feature;
		sensors_subfeature const *SubFeature;
	};
	std::vector<lm_SensorPair> Chips;                      ///<lm_sensors chip names
public:
	/** @brief Initialize lm_sensors using the configuration file located at 'file' */
	lm_sensor(const char* file) {
		FILE* F = nullptr;
		if (file != nullptr) { F = fopen(file,"r"); }
		int Errnum = sensors_init(F);
		if (F != nullptr) { fclose(F); }
		if (Errnum != 0) { std::runtime_error("Unable to initialize sensors library."); }

	        sensors_chip_name const* Chip;
	        sensors_feature const* feat;
	        sensors_subfeature const* subfeat;

                int FeatNo = 0;
		int ChipNo = 0;
		while ((Chip = sensors_get_detected_chips(NULL,&ChipNo)) != 0) {
			FeatNo = 0;
			while ((feat = sensors_get_features(Chip,&FeatNo)) != 0) {
				if (feat->type == SENSORS_FEATURE_TEMP) {
					lm_SensorPair SP;
					subfeat = sensors_get_subfeature(Chip,feat,SENSORS_SUBFEATURE_TEMP_INPUT);
					SP.Name = std::string(sensors_get_label(Chip,feat));
					//Prevent overwriting chip names by adding chip and feature numbers
					SP.Name += std::string(" ") + std::to_string(ChipNo) + std::string(":") + std::to_string(FeatNo);
					SP.Chip = Chip;
					SP.Feature = feat;
					SP.SubFeature = subfeat;
					Chips.push_back(SP);
				}
			}
		}
	}
	~lm_sensor() {
		Chips.clear();
		sensors_cleanup();
	}

	/** @brief Get the number of sensors that were probed from lm_sensors */
	virtual unsigned GetNumberOfSensors() const override {
		return Chips.size();
	}

	/** @brief Get the name of a sensor at index */
	std::string GetSensorName(unsigned index) const {
		return Chips.at(index).Name;
	}

	/** @brief Get the temperature of a sensor of a given name */
	virtual float GetTemperature(std::string const &Name) override {
		double ret;
		auto Eq = [&Name](lm_SensorPair const &SP) {
			return SP.Name == Name;
		};
		auto IT = std::find_if(Chips.begin(),Chips.end(),Eq);
		if (IT == Chips.end()) { throw std::runtime_error("Failed to find chip by name."); }
		sensors_get_value(IT->Chip,IT->SubFeature->number,&ret);
		return (float)ret;
	}

	virtual float GetTemperature(unsigned index) override {
		double ret;
		sensors_get_value(Chips.at(index).Chip,Chips.at(index).SubFeature->number,&ret);
		return (float)ret;
	}

	/** @brief Get all temperatures */
	virtual std::vector<TempPair> GetAllTemperatures() override {
		std::vector<TempPair> ret;
		for (auto &i : Chips) {
			TempPair TP;
			double TempT;
			TP.Name = i.Name;
			sensors_get_value(i.Chip,i.SubFeature->number,&TempT);
			TP.Temp = (float)TempT;
			ret.push_back(TP);
		}
		return ret;
	}
};

/** @brief Creates an empty SensorDetailLine structure */
SensorDetailLine CreateEmptySensorData(std::string const &Name) {
	SensorDetailLine ret;
	ret.Time = std::time(0);
	ret.FriendlyName = Name;
	ret.Command = "";
	ret.CritTemp = -273.15;
	ret.Symbol = '*';
	ret.Colour = 0;
	return ret;
}

/** @brief Gets all sensor readings from the sensors in the specified vector
 * @param Sensors          Vector of all sensors which are to be considered
 * @param BasicSensorMap   Map of all sensor names to their respective detail lines (translate name to object)
 * @returns Vector of sensor details at the current time
 */
std::vector<SensorDetailLine> GetAllSensorDetails(std::vector<std::shared_ptr<temperature_sensor_set>> const Sensors, std::unordered_map<std::string,SensorDetailLine> const &BasicSensorMap) {
	std::vector<SensorDetailLine> ret;
	std::time_t tTime;
	time(&tTime);
	for (auto const &i : Sensors) {
		std::vector<TempPair> TP = Sensors.back()->GetAllTemperatures();
		for (auto const &j : TP) {
			auto const IT = BasicSensorMap.find(j.Name);
			if (IT == BasicSensorMap.end()) {
				SensorDetailLine SLine = CreateEmptySensorData(j.Name);
				SLine.TempData = j;
				SLine.Time = tTime;
				ret.push_back(SLine);
			} else {
				SensorDetailLine SLine = IT->second;
				SLine.TempData = j;
				SLine.Time = tTime;
				ret.push_back(SLine);
				//SLine.Time = std::time(0);
			}
		}
	}
	return ret;
}

/** @brief Update UI temperature given sensor detail line vector */
bool UpdateSensorPreferences(std::vector<SensorDetailLine> const &SDL, std::vector<SensorPreferences> &Prefs)
{
	if (SDL.size() != Prefs.size())
		return false;
	for (unsigned i = 0; i != SDL.size(); i++) {
		Prefs[i].SetTempData(SDL[i].TempData);
	}
	return true;
}

/** @brief Build a vector of user preferennces using a vector of sensors
 * @note: does not load preferences from file
 */
std::vector<SensorPreferences> BuildPreferences(std::vector<std::shared_ptr<temperature_sensor_set>> const Sensors, std::unordered_map<std::string,SensorDetailLine> const &BasicSensorMap) {
	std::vector<SensorPreferences> ret;
	for (auto const &i : Sensors) {
		std::vector<TempPair> TP = Sensors.back()->GetAllTemperatures();
		for (auto const &j : TP) {
			auto const IT = BasicSensorMap.find(j.Name);
			if (IT == BasicSensorMap.end()) {
				ret.emplace_back(j.Name);
				ret.back().SetTempData(j);
			} else {
				ret.emplace_back(IT->second.TempData.Name);
				ret.back().SetFriendlyName(IT->second.FriendlyName);
				ret.back().SetTempData(j);
			}
		}
	}
	return ret;
}

#endif //SENSORCLASS_HPP_

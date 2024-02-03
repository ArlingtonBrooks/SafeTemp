#ifndef SENSORCLASS_HPP_
#define SENSORCLASS_HPP_

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

/** @brief A data structure for an lm_sensors sensor */
struct SensorPair {
	std::string Name;
	sensors_chip_name const *Chip;
	sensors_feature const *Feature;
	sensors_subfeature const *SubFeature;
};

struct TempPair {
	std::string Name;
	float Temp;
};

/** @brief A holder class for lm_sensors temperature chips */
class TempSensor {
private:
	std::vector<SensorPair> Chips;                      ///<lm_sensors chip names
public:
	/** @brief Initialize lm_sensors using the configuration file located at 'file' */
	TempSensor(const char* file) {
		FILE* F = fopen(file,"r");
		int Errnum = sensors_init(F);
		fclose(F);
		if (Errnum != 0) { std::runtime_error("Unable to initialize sensors library."); }

	        sensors_chip_name const* Chip;
	        sensors_feature const* feat;
	        sensors_subfeature const* subfeat;

                int FeatNo = 0;
		while ((feat = sensors_get_features(Chip,&FeatNo)) != 0)
		{
			if (feat->type == SENSORS_FEATURE_TEMP)
			{
				SensorPair SP;
				subfeat = sensors_get_subfeature(Chip,feat,SENSORS_SUBFEATURE_TEMP_INPUT);
				SP.Name = std::string(sensors_get_label(Chip,feat));
				SP.Chip = Chip;
				SP.Feature = feat;
				SP.SubFeature = subfeat;
				Chips.push_back(SP);
			}
		}
	}
	~TempSensor() {
		Chips.clear();
		sensors_cleanup();
	}

	/** @brief Get the number of sensors that were probed from lm_sensors */
	unsigned GetNumberOfSensors() const {
		return Chips.size();
	}

	/** @brief Get the name of a sensor at index */
	std::string GetSensorName(unsigned index) const {
		return Chips.at(index).Name;
	}

	/** @brief Get the temperature of a sensor of a given name */
	float GetTemperature(std::string const &Name) {
		double ret;
		auto Eq = [&Name](SensorPair const &SP) {
			return SP.Name == Name;
		};
		auto IT = std::find_if(Chips.begin(),Chips.end(),Eq);
		if (IT == Chips.end()) { throw std::runtime_error("Failed to find chip by name."); }
		sensors_get_value(IT->Chip,IT->SubFeature->number,&ret);
		return (float)ret;
	}

	/** @brief Get all temperatures */
	std::vector<TempPair> GetAllTemperatures() {
		std::vector<TempPair> ret;
		for (auto &i : Chips) {
			TempPair TP;
			double TempT;
			TP.Name = i.Name;
			sensors_get_value(i.Chip,i.SubFeature->number,&TempT);
			TP.Temp = (float)TempT;
			ret.push_back(TP);
		}
	}
};

#endif //SENSORCLASS_HPP_

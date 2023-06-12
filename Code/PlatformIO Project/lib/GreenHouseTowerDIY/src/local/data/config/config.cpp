#include "config.hpp"

GreenHouseConfig::GreenHouseConfig(ProjectConfig& projectConfig)
    : projectConfig(projectConfig) {}

GreenHouseConfig::~GreenHouseConfig() {}

void GreenHouseConfig::initConfig() {
  this->config.mqtt = {"", 0, "", "", {}};
}

//**********************************************************************************************************************
//*
//!                                                Load
//*
//**********************************************************************************************************************

void GreenHouseConfig::load() {
  loadMQTT();
  loadFeatures();
}

void GreenHouseConfig::loadMQTT() {
  this->config.mqtt.broker.assign(
      projectConfig.getString("mqtt_broker").c_str());
  this->config.mqtt.port = projectConfig.getInt("mqtt_port", 1886);
  this->config.mqtt.username.assign(
      projectConfig.getString("mqtt_username", "").c_str());
  this->config.mqtt.password.assign(
      projectConfig.getString("mqtt_password", "").c_str());

  for (auto it = this->config.mqtt.topics.begin();
       it != this->config.mqtt.topics.end(); ++it) {
    std::string key =
        "mqtt_topic_" +
        std::to_string(std::distance(this->config.mqtt.topics.begin(), it));
    it->assign(projectConfig.getString(key.c_str()).c_str());
  }
}

void GreenHouseConfig::loadFeatures() {
  this->config.enabled_features.humidity_Features =
      (HumidityFeatures_t)projectConfig.getInt("humidity_features", 0);
  this->config.enabled_features.ldr_Features =
      (LDRFeatures_t)projectConfig.getInt("ldr_features", 0);
  this->config.enabled_features.water_Level_Features =
      (WaterLevelFeatures_t)projectConfig.getInt("water_level_features", 0);
  this->config.enabled_features.dht_type.assign(
      projectConfig.getString("dht_type", "").c_str());
  this->config.enabled_features.dht_pin = projectConfig.getInt("dht_pin");
}

//**********************************************************************************************************************
//*
//!                                                Save
//*
//**********************************************************************************************************************

void GreenHouseConfig::save() {
  saveMQTT();
  saveFeatures();
}

void GreenHouseConfig::saveMQTT() {
  projectConfig.putString("mqtt_broker", this->config.mqtt.broker.c_str());
  projectConfig.putInt("mqtt_port", this->config.mqtt.port);
  projectConfig.putString("mqtt_username", this->config.mqtt.username.c_str());
  projectConfig.putString("mqtt_password", this->config.mqtt.password.c_str());

  for (auto it = this->config.mqtt.topics.begin();
       it != this->config.mqtt.topics.end(); ++it) {
    std::string key =
        "mqtt_topic_" +
        std::to_string(std::distance(this->config.mqtt.topics.begin(), it));
    projectConfig.putString(key.c_str(), it->c_str());
  }
}

void GreenHouseConfig::saveFeatures() {
  projectConfig.putInt("humidity_features",
                       this->config.enabled_features.humidity_Features);
  projectConfig.putInt("ldr_features",
                       this->config.enabled_features.ldr_Features);
  projectConfig.putInt("water_features",
                       this->config.enabled_features.water_Level_Features);

  projectConfig.putInt("dht_pin", this->config.enabled_features.dht_pin);
  projectConfig.putString("dht_type",
                          this->config.enabled_features.dht_type.c_str());
}

//**********************************************************************************************************************
//*
//!                                                ToRepresentation
//*
//**********************************************************************************************************************

std::string GreenHouseConfig::toRepresentation() {
  //* Features Section
  std::string hum;
  std::string ldr;
  std::string water;

  // convert enums to strings
  switch (this->config.enabled_features.humidity_Features) {
    case HumidityFeatures_t::NONE_HUMIDITY:
      hum.assign("None");
      break;
    case HumidityFeatures_t::DHT11:
      hum.assign("DHT11");
      break;
    case HumidityFeatures_t::DHT22:
      hum.assign("DHT22");
      break;
    case HumidityFeatures_t::DHT21:
      hum.assign("DHT21");
      break;
    case HumidityFeatures_t::SHT31:
      hum.assign("SHT31");
      break;
    case HumidityFeatures_t::SHT31x2:
      hum.assign("SHT31x2");
      break;
    case HumidityFeatures_t::ALL_HUMIDITY:
      hum.assign("All");
      break;
    default:
      hum.assign("None");
      break;
  }

  switch (this->config.enabled_features.ldr_Features) {
    case LDRFeatures_t::NONE_LDR:
      ldr.assign("None");
      break;
    case LDRFeatures_t::LDR:
      ldr.assign("LDR");
      break;
    case LDRFeatures_t::BH1750:
      ldr.assign("BH1750");
      break;
    case LDRFeatures_t::ALL_LDR:
      ldr.assign("All");
      break;
    default:
      ldr.assign("None");
      break;
  }

  switch (this->config.enabled_features.water_Level_Features) {
    case WaterLevelFeatures_t::NONE_WATER_LEVEL:
      water.assign("None");
      break;
    case WaterLevelFeatures_t::WATER_LEVEL_UC:
      water.assign("Water Level UltraSonic");
      break;
    case WaterLevelFeatures_t::WATER_LEVEL_PRESSURE:
      water.assign("Water Level Pressure");
      break;
    case WaterLevelFeatures_t::WATER_LEVEL_IR:
      water.assign("Water Level IR");
      break;
    case WaterLevelFeatures_t::ALL_WATER_LEVEL:
      water.assign("All");
      break;
    default:
      water.assign("None");
      break;
  }

  std::string features_json = Helpers::format_string(
      "\"features\": {\"humidity\": \"%s\", \"ldr\": \"%s\", \"water_level\": "
      "\"%s\"}",
      hum.c_str(), ldr.c_str(), water.c_str());

  //* MQTT Section
  std::string mqtt_json = Helpers::format_string(
      "\"deviceData\": {\"broker\": \"%s\", \"port\": \"%d\", \"username\": "
      "\"%s\", \"password\": \"%s\"}",
      this->config.mqtt.broker.c_str(), this->config.mqtt.port,
      this->config.mqtt.username.c_str(), this->config.mqtt.password.c_str());

  //* Return formatted json string
  return Helpers::format_string("{%s, %s, %s}", mqtt_json.c_str(),
                                features_json.c_str());
}

void GreenHouseConfig::setMQTTConfig(const std::string& broker,
                                     const std::string& username,
                                     const std::string& password,
                                     uint16_t port) {
  this->config.mqtt.username.assign(username);
  this->config.mqtt.password.assign(password);
  this->config.mqtt.port = port;
  this->config.mqtt.broker.assign(broker);
}

void GreenHouseConfig::setMQTTBroker(const std::string& broker, uint16_t port) {
  this->config.mqtt.port = port;
  this->config.mqtt.broker.assign(broker);
}

//**********************************************************************************************************************
//*
//!                                                GetMethods
//*
//**********************************************************************************************************************

Project_Config::MQTTConfig_t& GreenHouseConfig::getMQTTConfig() {
  return this->config.mqtt;
}

/**
 * @brief Get the MQTT Broker IP object
 */
IPAddress GreenHouseConfig::getBroker() {
  IPAddress broker_ip;
  Project_Config::MQTTConfig_t& mqttConfig = getMQTTConfig();
  if (!mqttConfig.broker.empty()) {
    log_d("[mDNS responder started]: Setting up Broker...");
    return broker_ip.fromString(mqttConfig.broker.c_str());
  }
  log_d("[mDNS responder failed]: Using hardcoded IP...");
  return broker_ip.fromString(MQTT_BROKER);
}

Project_Config::EnabledFeatures_t& GreenHouseConfig::getEnabledFeatures() {
  return this->config.enabled_features;
}

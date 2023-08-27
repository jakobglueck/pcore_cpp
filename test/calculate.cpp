#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include "Data.h"

class CalculateTest : public ::testing::Test {
 protected:
  std::vector<std::string> allPaths;
  void readAllFiles(std::string path) {
    std::vector<std::string> paths = CalculateTest::selectJsonFiles(path);
    for (size_t i = 0; i < paths.size(); i++) {
      if (paths[i].find(".json") != std::string::npos) {
        this->allPaths.push_back(paths[i]);
      } else {
        this->readAllFiles(paths[i]);
      }
    }
  }

  std::vector<std::string> static selectJsonFiles(std::string directory) {
    std::string jsonPath(directory);
    std::vector<std::string> allJsonFiles;
    for (const auto& entry : std::filesystem::directory_iterator(jsonPath)) {
      std::filesystem::path file = entry;
      std::string stringFile = file.generic_string();
      if (std::filesystem::path(stringFile).filename() == ".DS_Store") {
        int a = 1;
      } else {
        allJsonFiles.push_back(stringFile);
      }
    }
    return allJsonFiles;
  }

  Json::Value static readFromJson(std::string filePath) {
    std::ifstream jsonFile(filePath);
    Json::Value json;
    jsonFile >> json;
    return json;
  }

  std::string static createPathJsonFile(std::string directory) {
    std::string jsonPaths;
    jsonPaths = directory + ".json";
    return jsonPaths;
  }

  void static writeJson(std::string path, Data data) {
    Json::Value pcoreJson = data.toJson(DataForm::ABSOLUTE);
    std::fstream jsonFile(path, std::ios::out | std::ios::trunc | std::ios::binary);
    Json::FastWriter writer;
    jsonFile << writer.write(pcoreJson);
    jsonFile.close();
  }

  std::string static createPathPcoreFile(std::string directory) {
    std::string pcorePaths;
    pcorePaths = directory + ".pcore";
    return pcorePaths;
  }

  void static writePcore(ProtobufData protobufData, std::string pcorePath) {
    std::fstream writeFile(pcorePath, std::ios::out | std::ios::trunc | std::ios::binary);
    protobufData.SerializeToOstream(&writeFile);
    writeFile.close();
  }
  ProtobufData static readFromPcoreFile(std::string pcorePath) {
    ProtobufData data;
    std::fstream input(pcorePath, std::ios::in | std::ios::binary);
    data.ParseFromIstream(&input);
    return data;
  }
};

TEST_F(CalculateTest, HeartBeats) {
  this->readAllFiles("/Users/jakobglueck/Desktop/TEST_FILES/heartbeats/pcoreJsonHeartbeats/");
  std::vector<std::string> allJsonPaths = this->allPaths;
  std::vector<std::string> pcorePaths;
  for (size_t i = 0; i < allJsonPaths.size(); i++) {
    std::string path = allJsonPaths[i];
    std::string pcorePath = std::regex_replace(path, std::regex("pcoreJsonHeartbeats"), "pcoreheartbeats");
    std::string jsonPath = std::regex_replace(path, std::regex("pcoreJsonHeartbeats"), "heartbeatsJson");

    Data pcoreData = Data(this->readFromJson(path)["data"]);
    ProtobufData protobufData;
    pcoreData.serialize(&protobufData);

    this->writePcore(protobufData, this->createPathPcoreFile(pcorePath));

    Data readData = Data(this->readFromPcoreFile(this->createPathPcoreFile(pcorePath)));

    this->writeJson(this->createPathJsonFile(jsonPath), readData);
    Json::Value inputJson = this->readFromJson(path)["data"];
    Json::Value outputJson = this->readFromJson(this->createPathJsonFile(jsonPath))["data"];

    EXPECT_EQ(inputJson["data"]["header"]["time_zone_offset_min"].asUInt(), outputJson["data"]["header"]["time_zone_offset_min"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["major"].asUInt(), outputJson["data"]["header"]["version"]["major"].asUInt64());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["minor"].asUInt(), outputJson["data"]["header"]["version"]["minor"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["patch"].asUInt(), outputJson["data"]["header"]["version"]["patch"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["data_form"].asString(), outputJson["data"]["header"]["data_form"].asString());
    EXPECT_EQ(inputJson["data"]["raw"]["sensors"].size(), outputJson["data"]["raw"]["sensors"].size());
    for (Json::Value::ArrayIndex i = 0; i < inputJson["data"]["raw"]["sensors"].size(); i++) {
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"].size(), outputJson["data"]["raw"]["sensors"][i]["channels"].size());
      for (Json::Value::ArrayIndex j = 0; j < inputJson["data"]["raw"]["sensors"][i]["channels"].size(); j++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["color"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["color"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["wavelength_nm"].asInt(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["wavelength_nm"].asInt());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["norm"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["norm"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["coordinate"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["coordinate"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size());
        for (Json::Value::ArrayIndex k = 0; k < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size();
             k++) {
          EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"][k].asInt(),
                    outputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"][k].asInt());
        }
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size());
        for (Json::Value::ArrayIndex l = 0; l < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size(); l++) {
          EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size(),
                    outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size());
          for (Json::Value::ArrayIndex m = 0;
               m < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size(); l++) {
            EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"][m].asInt(),
                      outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"][m].asInt());
          }
        }
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size());
      for (Json::Value::ArrayIndex n = 0; n < inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size();
           n++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"][n].asUInt64(),
                  outputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"][n].asUInt64());
      }

      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["first_timestamp_ms"].asUInt64(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["first_timestamp_ms"].asUInt64());
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size());
      for (Json::Value::ArrayIndex o = 0;
           o < inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size(); o++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"][o].asUInt(),
                  outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"][o].asUInt());
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size());
      for (Json::Value::ArrayIndex p = 0;
           p < inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size(); p++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"][p].asUInt(),
                  outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"][p].asUInt());
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["sensor_type"].asString(), outputJson["data"]["raw"]["sensors"][i]["sensor_type"].asString());
    }
  }
}

TEST_F(CalculateTest, Polar) {
  this->readAllFiles("/Users/jakobglueck/Desktop/TEST_FILES/polar/pcoreJsonsPolar/");
  std::vector<std::string> allJsonPaths = this->allPaths;
  std::vector<std::string> pcorePaths;
  for (size_t i = 0; i < allJsonPaths.size(); i++) {
    std::string path = allJsonPaths[i];
    std::string pcorePath = std::regex_replace(path, std::regex("pcoreJsonsPolar"), "pcorePolar");
    std::string jsonPath = std::regex_replace(path, std::regex("pcoreJsonsPolar"), "polarJson");

    Data pcoreData = Data(this->readFromJson(path)["data"]);
    ProtobufData protobufData;
    pcoreData.serialize(&protobufData);

    this->writePcore(protobufData, this->createPathPcoreFile(pcorePath));

    Data readData = Data(this->readFromPcoreFile(this->createPathPcoreFile(pcorePath)));

    this->writeJson(this->createPathJsonFile(jsonPath), readData);
    Json::Value inputJson = this->readFromJson(path)["data"];
    Json::Value outputJson = this->readFromJson(this->createPathJsonFile(jsonPath))["data"];

    EXPECT_EQ(inputJson["data"]["header"]["time_zone_offset_min"].asUInt(), outputJson["data"]["header"]["time_zone_offset_min"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["major"].asUInt(), outputJson["data"]["header"]["version"]["major"].asUInt64());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["minor"].asUInt(), outputJson["data"]["header"]["version"]["minor"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["patch"].asUInt(), outputJson["data"]["header"]["version"]["patch"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["data_form"].asString(), outputJson["data"]["header"]["data_form"].asString());
    EXPECT_EQ(inputJson["data"]["raw"]["sensors"].size(), outputJson["data"]["raw"]["sensors"].size());
    for (Json::Value::ArrayIndex i = 0; i < inputJson["data"]["raw"]["sensors"].size(); i++) {
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"].size(), outputJson["data"]["raw"]["sensors"][i]["channels"].size());
      for (Json::Value::ArrayIndex j = 0; j < inputJson["data"]["raw"]["sensors"][i]["channels"].size(); j++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["color"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["color"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["wavelength_nm"].asInt(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["wavelength_nm"].asInt());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["norm"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["norm"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["coordinate"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["coordinate"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size());
        for (Json::Value::ArrayIndex k = 0; k < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size();
             k++) {
          EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"][k].asInt(),
                    outputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"][k].asInt());
        }
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size());
        for (Json::Value::ArrayIndex l = 0; l < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size(); l++) {
          EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size(),
                    outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size());
          for (Json::Value::ArrayIndex m = 0;
               m < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size(); l++) {
            EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"][m].asInt(),
                      outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"][m].asInt());
          }
        }
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size());
      for (Json::Value::ArrayIndex n = 0; n < inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size();
           n++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"][n].asUInt64(),
                  outputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"][n].asUInt64());
      }

      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["first_timestamp_ms"].asUInt64(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["first_timestamp_ms"].asUInt64());
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size());
      for (Json::Value::ArrayIndex o = 0;
           o < inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size(); o++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"][o].asUInt(),
                  outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"][o].asUInt());
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size());
      for (Json::Value::ArrayIndex p = 0;
           p < inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size(); p++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"][p].asUInt(),
                  outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"][p].asUInt());
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["sensor_type"].asString(), outputJson["data"]["raw"]["sensors"][i]["sensor_type"].asString());
    }
  }
}

TEST_F(CalculateTest, Corsano) {
  this->readAllFiles("/Users/jakobglueck/Desktop/TEST_FILES/Corsano/pcoreJsonCorsano/");
  std::vector<std::string> allJsonPaths = this->allPaths;
  std::vector<std::string> pcorePaths;
  for (size_t i = 0; i < allJsonPaths.size(); i++) {
    std::string path = allJsonPaths[i];
    std::string pcorePath = std::regex_replace(path, std::regex("pcoreJsonCorsano"), "pcoreCorsano");
    std::string jsonPath = std::regex_replace(path, std::regex("pcoreJsonCorsano"), "corsanoJson");

    Data pcoreData = Data(this->readFromJson(path)["data"]);
    ProtobufData protobufData;
    pcoreData.serialize(&protobufData);

    this->writePcore(protobufData, this->createPathPcoreFile(pcorePath));

    Data readData = Data(this->readFromPcoreFile(this->createPathPcoreFile(pcorePath)));

    this->writeJson(this->createPathJsonFile(jsonPath), readData);
    Json::Value inputJson = this->readFromJson(path)["data"];
    Json::Value outputJson = this->readFromJson(this->createPathJsonFile(jsonPath))["data"];

    EXPECT_EQ(inputJson["data"]["header"]["time_zone_offset_min"].asUInt(), outputJson["data"]["header"]["time_zone_offset_min"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["major"].asUInt(), outputJson["data"]["header"]["version"]["major"].asUInt64());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["minor"].asUInt(), outputJson["data"]["header"]["version"]["minor"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["version"]["patch"].asUInt(), outputJson["data"]["header"]["version"]["patch"].asUInt());
    EXPECT_EQ(inputJson["data"]["header"]["data_form"].asString(), outputJson["data"]["header"]["data_form"].asString());
    EXPECT_EQ(inputJson["data"]["raw"]["sensors"].size(), outputJson["data"]["raw"]["sensors"].size());
    for (Json::Value::ArrayIndex i = 0; i < inputJson["data"]["raw"]["sensors"].size(); i++) {
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"].size(), outputJson["data"]["raw"]["sensors"][i]["channels"].size());
      for (Json::Value::ArrayIndex j = 0; j < inputJson["data"]["raw"]["sensors"][i]["channels"].size(); j++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["color"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["color"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["wavelength_nm"].asInt(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["ppg_metadata"]["wavelength_nm"].asInt());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["norm"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["norm"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["coordinate"].asString(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["acc_metadata"]["coordinate"].asString());
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size());
        for (Json::Value::ArrayIndex k = 0; k < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"].size();
             k++) {
          EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"][k].asInt(),
                    outputJson["data"]["raw"]["sensors"][i]["channels"][j]["absolute_block"]["absolute_values"][k].asInt());
        }
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size(),
                  outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size());
        for (Json::Value::ArrayIndex l = 0; l < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"].size(); l++) {
          EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size(),
                    outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size());
          for (Json::Value::ArrayIndex m = 0;
               m < inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"].size(); l++) {
            EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"][m].asInt(),
                      outputJson["data"]["raw"]["sensors"][i]["channels"][j]["differential_blocks"][l]["differential_values"][m].asInt());
          }
        }
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size());
      for (Json::Value::ArrayIndex n = 0; n < inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"].size();
           n++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"][n].asUInt64(),
                  outputJson["data"]["raw"]["sensors"][i]["absolute_timestamps_container"]["unix_timestamps_ms"][n].asUInt64());
      }

      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["first_timestamp_ms"].asUInt64(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["first_timestamp_ms"].asUInt64());
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size());
      for (Json::Value::ArrayIndex o = 0;
           o < inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"].size(); o++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"][o].asUInt(),
                  outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["block_intervals_ms"][o].asUInt());
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size(),
                outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size());
      for (Json::Value::ArrayIndex p = 0;
           p < inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"].size(); p++) {
        EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"][p].asUInt(),
                  outputJson["data"]["raw"]["sensors"][i]["differential_timestamps_container"]["timestamps_intervals_ms"][p].asUInt());
      }
      EXPECT_EQ(inputJson["data"]["raw"]["sensors"][i]["sensor_type"].asString(), outputJson["data"]["raw"]["sensors"][i]["sensor_type"].asString());
    }
  }
}

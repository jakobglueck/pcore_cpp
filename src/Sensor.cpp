/*

Created by Jakob Glück 2023

Copyright © 2023 PREVENTICUS GmbH

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "Sensor.h"

Sensor::Sensor(std::vector<Channel> channels,
               DifferentialTimestampsContainer differentialTimestampsContainer,
               ProtobufSensorType protobufSensorType)
    : protobufSensorType(protobufSensorType), channels(channels),
      differentialTimestampsContainer(differentialTimestampsContainer) {
  this->absoluteTimestampsContainer =
      this->calculateAbsoluteTimestamps(differentialTimestampsContainer);
}

Sensor::Sensor(std::vector<Channel> channels,
               AbsoluteTimestampsContainer absoluteTimestampsContainer,
               ProtobufSensorType protobufSensorType)
    : protobufSensorType(protobufSensorType), channels(channels),
      absoluteTimestampsContainer(absoluteTimestampsContainer) {
  std::vector<size_t> blockIdx = this->findBlocksIdxs();
  this->differentialTimestampsContainer = this->calculateDifferentialTimestamps(
      absoluteTimestampsContainer, blockIdx);
}

Sensor::Sensor(Json::Value &sensor, DataForm dataForm) {
  if (dataForm == DataForm::DIFFERENTIAL) {
    Json::Value jsonChannels = sensor["channels"];
    std::vector<Channel> channels;
    channels.reserve(jsonChannels.size());
    auto n = jsonChannels.size();
    auto sensorType = sensor["sensor_type"];
    for (Json::Value::ArrayIndex i = 0; i < n; i++) {
      channels.push_back(Channel(jsonChannels[i], sensorType));
    }
    this->channels = channels;

    DifferentialTimestampsContainer differentialTimestampsContainer =
        DifferentialTimestampsContainer(
            sensor["differential_timestamps_container"]);
    this->differentialTimestampsContainer = differentialTimestampsContainer;
    this->absoluteTimestampsContainer =
        this->calculateAbsoluteTimestamps(differentialTimestampsContainer);
  }
  if (dataForm == DataForm::ABSOLUTE) {
    AbsoluteTimestampsContainer absoluteTimestampsContainer =
        AbsoluteTimestampsContainer(sensor["absolute_timestamps_container"]);
    this->absoluteTimestampsContainer = absoluteTimestampsContainer;
    std::vector<size_t> blockIdx = this->findBlocksIdxs();
    this->differentialTimestampsContainer =
        this->calculateDifferentialTimestamps(absoluteTimestampsContainer,
                                              blockIdx);
    Json::Value jsonChannels = sensor["channels"];
    auto n = jsonChannels.size();
    auto sensorType = sensor["sensor_type"];
    std::vector<Channel> channels;
    channels.reserve(n);
    for (Json::Value::ArrayIndex i = 0; i < n; i++) {
      channels.push_back(Channel(jsonChannels[i], sensorType, blockIdx));
    }
    this->channels = channels;
  }
  if (sensor["sensor_type"].asString() == "SENSOR_TYPE_PPG") {
    this->protobufSensorType = ProtobufSensorType::SENSOR_TYPE_PPG;
  }
  if (sensor["sensor_type"].asString() == "SENSOR_TYPE_ACC") {
    this->protobufSensorType = ProtobufSensorType::SENSOR_TYPE_ACC;
  }
}

Sensor::Sensor(const ProtobufSensor &protobufSensor) {
  this->deserialize(protobufSensor);
}

Sensor::Sensor() {
  this->channels = std::vector<Channel>{};
  this->absoluteTimestampsContainer = AbsoluteTimestampsContainer();
  this->differentialTimestampsContainer = DifferentialTimestampsContainer();
  this->protobufSensorType = ProtobufSensorType::SENSOR_TYPE_NONE;
}

ProtobufSensorType Sensor::getSensorType() { return this->protobufSensorType; }

std::vector<Channel> Sensor::getChannels() { return this->channels; }

DifferentialTimestampsContainer Sensor::getDifferentialTimestamps() {
  return this->differentialTimestampsContainer;
}

AbsoluteTimestampsContainer Sensor::getAbsoluteTimestamps() {
  return this->absoluteTimestampsContainer;
}

bool Sensor::isEqual(Sensor &sensor) {
  if (this->channels.size() != sensor.channels.size()) {
    return false;
  }
  for (size_t i = 0; i < this->channels.size(); i++) {
    if (!this->channels[i].isEqual(sensor.channels[i])) {
      return false;
    }
  }
  return this->protobufSensorType == sensor.protobufSensorType &&
         this->differentialTimestampsContainer.isEqual(
             sensor.differentialTimestampsContainer) &&
         this->absoluteTimestampsContainer.isEqual(
             sensor.absoluteTimestampsContainer);
}

void Sensor::serialize(ProtobufSensor *protobufSensor) {
  if (protobufSensor == nullptr) {
    throw std::invalid_argument(
        "Error in serialize: protobufSensor is a null pointer");
  }
  for (auto &channel : this->channels) {
    channel.serialize(protobufSensor->add_channels());
  }
  protobufSensor->set_sensor_type(this->protobufSensorType);
  ProtobufDifferentialTimestampContainer protobufTimestampContainer;
  this->differentialTimestampsContainer.serialize(&protobufTimestampContainer);
  protobufSensor->mutable_differential_timestamps_container()->CopyFrom(
      protobufTimestampContainer);
}

uint64_t Sensor::getFirstTimestamp() {
  return this->differentialTimestampsContainer.getFirstTimestamp();
}

uint64_t Sensor::getLastTimestamp() {
  const std::vector<uint32_t> timestampIntervals_ms =
      this->differentialTimestampsContainer.getTimestampsIntervals();
  const std::vector<uint32_t> blockIntervals_ms =
      this->differentialTimestampsContainer.getBlockIntervals();
  std::vector<DifferentialBlock> firstChannelBlocks =
      this->channels[0].getDifferentialBlocks();
  const size_t nLastBlock = firstChannelBlocks[firstChannelBlocks.size() - 1]
                                .getDifferentialValues()
                                .size();
  long h = 0;
  for (auto &BlockIntervals : blockIntervals_ms) {
    h += BlockIntervals;
  }
  return this->getFirstTimestamp() + h +
         timestampIntervals_ms[timestampIntervals_ms.size() - 1] *
             (nLastBlock - 1);
}

uint64_t Sensor::getDuration() {
  return this->getLastTimestamp() - this->getFirstTimestamp();
}

std::vector<size_t> Sensor::findBlocksIdxs() {
  std::vector<size_t> blocksIdxs = {};
  if (this->absoluteTimestampsContainer.getUnixTimestamps().size() == 0) {
    return blocksIdxs;
  }
  uint64_t referenceTimeDifference = 0;
  bool isNewBlock = true;
  blocksIdxs.push_back(0);
  std::vector<uint64_t> absoluteUnixTimestamps =
      this->absoluteTimestampsContainer.getUnixTimestamps();
  auto n = absoluteUnixTimestamps.size();
  for (size_t i = 1; i < n; i++) {
    uint64_t timeDifference =
        absoluteUnixTimestamps[i] - absoluteUnixTimestamps[i - 1];
    if (isNewBlock) {
      referenceTimeDifference = timeDifference;
      isNewBlock = false;
    }
    if (timeDifference != referenceTimeDifference) {
      blocksIdxs.push_back(i);
      isNewBlock = true;
    }
  }
  return blocksIdxs;
}

AbsoluteTimestampsContainer Sensor::calculateAbsoluteTimestamps(
    DifferentialTimestampsContainer differentialTimestamps) {
  std::vector<uint64_t> unixTimestamps_ms;

  std::vector<DifferentialBlock> firstChannelBlocks =
      this->channels[0].getDifferentialBlocks();
  uint64_t firstTimestamp_ms = differentialTimestamps.getFirstTimestamp();
  std::vector<uint32_t> timestamps_intervals_ms =
      differentialTimestamps.getTimestampsIntervals();
  std::vector<uint32_t> blockIntervals_ms =
      differentialTimestamps.getBlockIntervals();

  uint64_t sumTimeStampIntervals = firstTimestamp_ms;

  size_t h = 0;
  for (auto f : firstChannelBlocks) {
    h += f.getDifferentialValues().size();
  }

  unixTimestamps_ms.reserve(h);
  auto m = timestamps_intervals_ms.size();
  for (size_t i = 0; i < m; i++) {
    sumTimeStampIntervals += blockIntervals_ms[i];
    auto t = timestamps_intervals_ms[i];
    size_t n = firstChannelBlocks[i].getDifferentialValues().size();
    for (uint64_t j = 0; j < n; j++) {
      unixTimestamps_ms.push_back(sumTimeStampIntervals + j * t);
    }
  }
  return AbsoluteTimestampsContainer(unixTimestamps_ms);
}

DifferentialTimestampsContainer Sensor::calculateDifferentialTimestamps(
    AbsoluteTimestampsContainer absoluteTimestamps,
    std::vector<size_t> blocksIdxs) {
  DifferentialTimestampsContainer differentialTimestampsContainer;
  size_t n = blocksIdxs.size();
  std::vector<uint64_t> absoluteUnixTimestamps =
      absoluteTimestamps.getUnixTimestamps();
  std::vector<uint32_t> blockIntervals_ms = {};
  std::vector<uint32_t> timestampIntervals_ms = {};
  if (n == 0) {
    uint64_t firstTimestamp_ms = 0;
    differentialTimestampsContainer = DifferentialTimestampsContainer(
        firstTimestamp_ms, blockIntervals_ms, timestampIntervals_ms);
    return differentialTimestampsContainer;
  }
  uint64_t firstTimestamp_ms = absoluteTimestamps.getUnixTimestamps()[0];
  if (n == 1) {
    int32_t timestampInterval = absoluteUnixTimestamps[1] - firstTimestamp_ms;
    timestampIntervals_ms.push_back(
        absoluteUnixTimestamps.size() == 1 ? 0 : timestampInterval);
    blockIntervals_ms.push_back(0);
    differentialTimestampsContainer = DifferentialTimestampsContainer(
        firstTimestamp_ms, blockIntervals_ms, timestampIntervals_ms);
    return differentialTimestampsContainer;
  }
  blockIntervals_ms.push_back(0);
  timestampIntervals_ms.push_back(absoluteUnixTimestamps[1] -
                                  firstTimestamp_ms);
  auto m = blocksIdxs.size() - 1;
  for (size_t i = 1; i < m; i++) {
    const size_t prevIdx = blocksIdxs[i - 1];
    const size_t idx = blocksIdxs[i];
    timestampIntervals_ms.push_back(absoluteUnixTimestamps[idx + 1] -
                                    absoluteUnixTimestamps[idx]);
    blockIntervals_ms.push_back(absoluteUnixTimestamps[idx] -
                                absoluteUnixTimestamps[prevIdx]);
  }
  blockIntervals_ms.push_back(absoluteUnixTimestamps[blocksIdxs[n - 1]] -
                              absoluteUnixTimestamps[blocksIdxs[n - 2]]);
  timestampIntervals_ms.push_back(
      absoluteUnixTimestamps.size() - 1 == blocksIdxs[n - 1]
          ? 0
          : absoluteUnixTimestamps[blocksIdxs[n - 1] + 1] -
                absoluteUnixTimestamps[blocksIdxs[n - 1]]);
  differentialTimestampsContainer = DifferentialTimestampsContainer(
      firstTimestamp_ms, blockIntervals_ms, timestampIntervals_ms);
  return differentialTimestampsContainer;
}

Json::Value Sensor::toJson(DataForm dataForm) {
  Json::Value sensor;
  Json::Value channels(Json::arrayValue);
  Json::Value sensorType(Json::stringValue);
  if (dataForm == DataForm::ABSOLUTE) {
    for (auto &channel : this->channels) {
      channels.append(channel.toJson(dataForm, this->protobufSensorType));
    }
    if (this->protobufSensorType == ProtobufSensorType::SENSOR_TYPE_ACC) {
      sensorType = "SENSOR_TYPE_ACC";
    }
    if (this->protobufSensorType == ProtobufSensorType::SENSOR_TYPE_PPG) {
      sensorType = "SENSOR_TYPE_PPG";
    }
    sensor["channels"] = channels;
    sensor["absolute_timestamps_container"] =
        this->absoluteTimestampsContainer.toJson();
    sensor["sensor_type"] = sensorType;
  }
  if (dataForm == DataForm::DIFFERENTIAL) {
    for (auto &channel : this->channels) {
      channels.append(channel.toJson(dataForm, this->protobufSensorType));
    }
    if (this->protobufSensorType == ProtobufSensorType::SENSOR_TYPE_ACC) {
      sensorType = "SENSOR_TYPE_ACC";
    }
    if (this->protobufSensorType == ProtobufSensorType::SENSOR_TYPE_PPG) {
      sensorType = "SENSOR_TYPE_PPG";
    }
    sensor["channels"] = channels;
    sensor["differential_timestamps_container"] =
        this->differentialTimestampsContainer.toJson();
    sensor["sensor_type"] = sensorType;
  }
  return sensor;
}

void Sensor::deserialize(const ProtobufSensor &protobufSensor) {
  std::vector<Channel> channels;
  channels.reserve(protobufSensor.channels().size());
  for (auto &channel : protobufSensor.channels()) {
    channels.push_back(Channel(channel));
  }
  this->channels = channels;
  this->protobufSensorType = protobufSensor.sensor_type();
  this->differentialTimestampsContainer = DifferentialTimestampsContainer(
      protobufSensor.differential_timestamps_container());
  this->absoluteTimestampsContainer =
      this->calculateAbsoluteTimestamps(this->differentialTimestampsContainer);
}
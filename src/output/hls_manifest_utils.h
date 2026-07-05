#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <string>

namespace Mist{
  namespace HLSManifest{
    uint32_t targetDurationSeconds(const std::deque<uint64_t> &durations,
                                   uint32_t fallbackTargetDuration);

    void trimLiveWindow(std::deque<std::string> &lines, std::deque<uint64_t> &durations,
                        std::deque<uint64_t> &segmentStarts, std::deque<uint64_t> &segmentIndexes,
                        uint32_t targetDuration, uint64_t listLimit, size_t &skippedLines,
                        uint64_t &totalDuration);

    uint64_t mediaSequence(uint64_t fragmentSequence, uint64_t firstSegmentStartTime,
                           bool noEndList);

    bool shouldWriteEndList(bool isLive, uint64_t totalDuration, bool noEndList);

    std::string renditionQuery(const std::string &token, bool includeToken,
                               const std::map<std::string, std::string> &params);
  }// namespace HLSManifest
}// namespace Mist

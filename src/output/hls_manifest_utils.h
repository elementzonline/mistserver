#pragma once

#include <cstdint>
#include <deque>
#include <string>

namespace Mist{
  namespace HLSManifest{
    uint32_t targetDurationSeconds(const std::deque<uint64_t> &durations,
                                   uint32_t fallbackTargetDuration);

    void trimLiveWindow(std::deque<std::string> &lines, std::deque<uint64_t> &durations,
                        uint32_t targetDuration, uint64_t listLimit, size_t &skippedLines,
                        uint64_t &totalDuration);

    bool shouldWriteEndList(bool isLive, uint64_t totalDuration, bool noEndList);
  }// namespace HLSManifest
}// namespace Mist

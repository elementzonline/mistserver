#include "hls_manifest_utils.h"

#include <algorithm>
#include <limits>

namespace Mist{
  namespace HLSManifest{
    uint32_t targetDurationSeconds(const std::deque<uint64_t> &durations, uint32_t fallbackTargetDuration){
      if (durations.empty()){return fallbackTargetDuration;}
      uint64_t maxDuration = *std::max_element(durations.begin(), durations.end());
      uint64_t targetDuration = std::max<uint64_t>(1, (maxDuration + 999) / 1000);
      if (targetDuration > std::numeric_limits<uint32_t>::max()){
        return std::numeric_limits<uint32_t>::max();
      }
      return (uint32_t)targetDuration;
    }

    void trimLiveWindow(std::deque<std::string> &lines, std::deque<uint64_t> &durations,
                        std::deque<uint64_t> &segmentStarts, std::deque<uint64_t> &segmentIndexes,
                        uint32_t targetDuration, uint64_t listLimit, size_t &skippedLines,
                        uint64_t &totalDuration){
      if (!listLimit){return;}
      uint64_t keepDuration = (uint64_t)targetDuration * 4000;
      while (lines.size() > listLimit && durations.size() &&
             (totalDuration - durations.front()) > keepDuration){
        lines.pop_front();
        totalDuration -= durations.front();
        durations.pop_front();
        if (segmentStarts.size()){segmentStarts.pop_front();}
        if (segmentIndexes.size()){segmentIndexes.pop_front();}
        ++skippedLines;
      }
    }

    uint64_t mediaSequence(uint64_t fragmentSequence, uint64_t firstSegmentStartTime, bool noEndList){
      (void)firstSegmentStartTime;
      (void)noEndList;
      return fragmentSequence;
    }

    bool shouldWriteEndList(bool isLive, uint64_t totalDuration, bool noEndList){
      if (!totalDuration){return true;}
      if (noEndList){return false;}
      return !isLive;
    }
  }// namespace HLSManifest
}// namespace Mist

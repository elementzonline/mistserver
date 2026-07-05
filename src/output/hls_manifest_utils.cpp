#include "hls_manifest_utils.h"

#include <algorithm>
#include <limits>

namespace Mist{
  namespace HLSManifest{
    namespace{
      void appendQueryParam(std::string &query, const std::string &key, const std::string &value){
        if (value.empty()){return;}
        if (query.size()){query += "&";}else{query = "?";}
        query += key + "=" + value;
      }

      bool truthyTargetParam(const std::map<std::string, std::string> &params, const std::string &key){
        std::map<std::string, std::string>::const_iterator it = params.find(key);
        return it != params.end() && it->second != "0" && it->second != "false" && it->second != "False";
      }
    }// namespace

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

    std::string renditionQuery(const std::string &token, bool includeToken,
                               const std::map<std::string, std::string> &params,
                               uint64_t liveWindowAnchorMs){
      std::string query;
      if (includeToken && token.size()){appendQueryParam(query, "tkn", token);}

      bool noEndList = truthyTargetParam(params, "noendlist");
      std::map<std::string, std::string>::const_iterator startIt = params.find("start");
      bool hasStart = startIt != params.end();
      if (startIt != params.end()){
        appendQueryParam(query, "start", startIt->second);
      }else if (!noEndList){
        std::map<std::string, std::string>::const_iterator startUnixIt = params.find("startunix");
        if (startUnixIt != params.end()){appendQueryParam(query, "startunix", startUnixIt->second);}
      }

      std::map<std::string, std::string>::const_iterator durationIt = params.find("duration");
      if (noEndList && durationIt != params.end()){
        appendQueryParam(query, "duration", durationIt->second);
      }else{
        std::map<std::string, std::string>::const_iterator stopIt = params.find("stop");
        if (stopIt != params.end()){
          appendQueryParam(query, "stop", stopIt->second);
        }else if (!noEndList){
          std::map<std::string, std::string>::const_iterator stopUnixIt = params.find("stopunix");
          if (stopUnixIt != params.end()){appendQueryParam(query, "stopunix", stopUnixIt->second);}
        }
      }

      if (noEndList){
        std::map<std::string, std::string>::const_iterator noEndIt = params.find("noendlist");
        if (noEndIt != params.end()){appendQueryParam(query, "noendlist", noEndIt->second);}
      }
      if (noEndList && hasStart && durationIt != params.end() && liveWindowAnchorMs){
        appendQueryParam(query, "hlswindow", "1");
        appendQueryParam(query, "hlsanchor", std::to_string(liveWindowAnchorMs));
      }

      return query;
    }

    uint64_t slidingWindowStart(uint64_t baseStart, uint64_t anchorUnixMs, uint64_t nowUnixMs){
      if (!anchorUnixMs || nowUnixMs <= anchorUnixMs){return baseStart;}
      return baseStart + (nowUnixMs - anchorUnixMs);
    }

    bool shouldSkipInitialLiveSegments(bool noEndList, bool hasDuration){
      return !(noEndList && hasDuration);
    }
  }// namespace HLSManifest
}// namespace Mist

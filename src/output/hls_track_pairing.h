#pragma once

#include <mist/defines.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace Mist{
  struct HLSTrackTiming{
    size_t trackId;
    uint64_t firstMs;
    uint64_t lastMs;
    size_t sourceId;
  };

  inline uint64_t hlsTimestampDistance(uint64_t lhs, uint64_t rhs){
    return lhs > rhs ? lhs - rhs : rhs - lhs;
  }

  inline size_t selectHLSAudioTrack(const HLSTrackTiming &video,
                                    const std::vector<HLSTrackTiming> &videoTracks,
                                    const std::vector<HLSTrackTiming> &audioTracks){
    if (videoTracks.size() == audioTracks.size() && !audioTracks.empty()){
      for (size_t i = 0; i < videoTracks.size(); ++i){
        if (videoTracks[i].trackId == video.trackId){return audioTracks[i].trackId;}
      }
    }

    size_t selected = INVALID_TRACK_ID;
    uint64_t bestTimelineDistance = std::numeric_limits<uint64_t>::max();
    uint64_t bestSourceDistance = std::numeric_limits<uint64_t>::max();

    for (std::vector<HLSTrackTiming>::const_iterator it = audioTracks.begin();
         it != audioTracks.end(); ++it){
      const uint64_t firstDistance = hlsTimestampDistance(video.firstMs, it->firstMs);
      const uint64_t lastDistance = hlsTimestampDistance(video.lastMs, it->lastMs);
      const uint64_t timelineDistance =
          firstDistance > std::numeric_limits<uint64_t>::max() - lastDistance
              ? std::numeric_limits<uint64_t>::max()
              : firstDistance + lastDistance;
      const uint64_t sourceDistance = hlsTimestampDistance(video.sourceId, it->sourceId);

      if (timelineDistance < bestTimelineDistance ||
          (timelineDistance == bestTimelineDistance && sourceDistance < bestSourceDistance) ||
          (timelineDistance == bestTimelineDistance && sourceDistance == bestSourceDistance &&
           (selected == INVALID_TRACK_ID || it->trackId < selected))){
        selected = it->trackId;
        bestTimelineDistance = timelineDistance;
        bestSourceDistance = sourceDistance;
      }
    }
    return selected;
  }
}// namespace Mist

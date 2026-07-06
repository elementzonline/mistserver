#include "../src/output/hls_manifest_utils.h"

#include <cassert>
#include <deque>
#include <map>
#include <string>

int main(){
  std::deque<uint64_t> twoSecondDurations;
  std::deque<uint64_t> segmentStarts;
  std::deque<uint64_t> segmentIndexes;
  std::deque<std::string> lines;
  uint64_t totalDuration = 0;
  for (size_t i = 0; i < 20; ++i){
    twoSecondDurations.push_back(2002);
    segmentStarts.push_back(48000000 + (i * 2002));
    segmentIndexes.push_back(3000 + i);
    lines.push_back("segment");
    totalDuration += 2002;
  }

  uint32_t targetDuration = Mist::HLSManifest::targetDurationSeconds(twoSecondDurations, 8130);
  assert(targetDuration == 3);

  size_t skippedLines = 0;
  Mist::HLSManifest::trimLiveWindow(lines, twoSecondDurations, segmentStarts, segmentIndexes,
                                    targetDuration, 8, skippedLines, totalDuration);
  assert(lines.size() == 8);
  assert(twoSecondDurations.size() == 8);
  assert(segmentStarts.size() == 8);
  assert(segmentIndexes.size() == 8);
  assert(skippedLines == 12);
  assert(totalDuration == 16016);
  assert(segmentStarts.front() == 48024024);
  assert(segmentIndexes.front() == 3012);

  assert(Mist::HLSManifest::mediaSequence(3012, 48024024, true) == 3012);
  assert(Mist::HLSManifest::mediaSequence(4, 48774028, false) == 4);
  assert(Mist::HLSManifest::mediaSequence(12, 0, true) == 12);

  assert(!Mist::HLSManifest::shouldWriteEndList(false, totalDuration, true));
  assert(Mist::HLSManifest::shouldWriteEndList(false, totalDuration, false));
  assert(!Mist::HLSManifest::shouldWriteEndList(true, totalDuration, false));
  assert(Mist::HLSManifest::shouldWriteEndList(true, 0, true));

  std::map<std::string, std::string> relativeUnixParams;
  relativeUnixParams["startunix"] = "-120";
  relativeUnixParams["start"] = "123456";
  relativeUnixParams["duration"] = "60";
  relativeUnixParams["noendlist"] = "1";
  std::string query = Mist::HLSManifest::renditionQuery("tok", true, relativeUnixParams);
  assert(query == "?tkn=tok&start=123456&duration=60&noendlist=1");
  assert(query.find("startunix") == std::string::npos);

  std::string slidingQuery =
      Mist::HLSManifest::renditionQuery("tok", true, relativeUnixParams, 1234567890000ull);
  assert(slidingQuery ==
         "?tkn=tok&start=123456&duration=60&noendlist=1&hlswindow=1&hlsanchor=1234567890000");
  assert(Mist::HLSManifest::slidingWindowStart(123456, 100000, 102500) == 125956);
  assert(Mist::HLSManifest::slidingWindowStart(123456, 102500, 100000) == 123456);
  assert(Mist::HLSManifest::shouldSkipInitialLiveSegments(false, false));
  assert(!Mist::HLSManifest::shouldSkipInitialLiveSegments(true, true));
  assert(Mist::HLSManifest::shouldApplyLivePlaylistRules(false, true, true));
  assert(Mist::HLSManifest::shouldApplyLivePlaylistRules(true, false, false));
  assert(!Mist::HLSManifest::shouldApplyLivePlaylistRules(false, false, true));

  std::deque<uint64_t> tailDurations;
  std::deque<uint64_t> tailStarts;
  std::deque<uint64_t> tailIndexes;
  std::deque<std::string> tailLines;
  uint64_t tailTotal = 0;
  for (size_t i = 0; i < 5; ++i){
    tailDurations.push_back(i == 4 ? 286 : 2002);
    tailStarts.push_back(900000 + (i * 2002));
    tailIndexes.push_back(700 + i);
    tailLines.push_back("tail-segment");
    tailTotal += tailDurations.back();
  }
  size_t trimmedTail = Mist::HLSManifest::trimTrailingPartialSegments(
      tailLines, tailDurations, tailStarts, tailIndexes, tailTotal);
  assert(trimmedTail == 1);
  assert(tailLines.size() == 4);
  assert(tailDurations.back() == 2002);
  assert(tailStarts.back() == 906006);
  assert(tailIndexes.back() == 703);
  assert(tailTotal == 8008);

  tailDurations.clear();
  tailStarts.clear();
  tailIndexes.clear();
  tailLines.clear();
  tailTotal = 0;
  const uint64_t consecutiveTailDurations[] = {2002, 2002, 2002, 900, 340};
  for (size_t i = 0; i < 5; ++i){
    tailDurations.push_back(consecutiveTailDurations[i]);
    tailStarts.push_back(910000 + (i * 2002));
    tailIndexes.push_back(800 + i);
    tailLines.push_back("tail-segment");
    tailTotal += tailDurations.back();
  }
  trimmedTail = Mist::HLSManifest::trimTrailingPartialSegments(
      tailLines, tailDurations, tailStarts, tailIndexes, tailTotal);
  assert(trimmedTail == 2);
  assert(tailLines.size() == 3);
  assert(tailDurations.back() == 2002);
  assert(tailTotal == 6006);

  return 0;
}

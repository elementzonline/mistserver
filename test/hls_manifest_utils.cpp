#include "../src/output/hls_manifest_utils.h"

#include <cassert>
#include <deque>
#include <string>

int main(){
  std::deque<uint64_t> twoSecondDurations;
  std::deque<std::string> lines;
  uint64_t totalDuration = 0;
  for (size_t i = 0; i < 20; ++i){
    twoSecondDurations.push_back(2002);
    lines.push_back("segment");
    totalDuration += 2002;
  }

  uint32_t targetDuration = Mist::HLSManifest::targetDurationSeconds(twoSecondDurations, 8130);
  assert(targetDuration == 3);

  size_t skippedLines = 0;
  Mist::HLSManifest::trimLiveWindow(lines, twoSecondDurations, targetDuration, 8, skippedLines,
                                    totalDuration);
  assert(lines.size() == 8);
  assert(twoSecondDurations.size() == 8);
  assert(skippedLines == 12);
  assert(totalDuration == 16016);

  assert(!Mist::HLSManifest::shouldWriteEndList(false, totalDuration, true));
  assert(Mist::HLSManifest::shouldWriteEndList(false, totalDuration, false));
  assert(!Mist::HLSManifest::shouldWriteEndList(true, totalDuration, false));
  assert(Mist::HLSManifest::shouldWriteEndList(true, 0, true));

  return 0;
}

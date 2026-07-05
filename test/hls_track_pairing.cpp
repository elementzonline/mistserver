#include "src/output/hls_track_pairing.h"

#include <iostream>
#include <vector>

int main(){
  const std::vector<Mist::HLSTrackTiming> audioTracks = {
      {0, 1000, 7000, 10},
      {1, 1005, 5005, 11},
  };

  const Mist::HLSTrackTiming lowVideo = {2, 1000, 5000, 12};
  const size_t paired = Mist::selectHLSAudioTrack(lowVideo, audioTracks);
  if (paired != 1){
    std::cerr << "selected audio track " << paired
              << ", expected timeline-aligned track 1" << std::endl;
    return 1;
  }

  const std::vector<Mist::HLSTrackTiming> tiedAudioTracks = {
      {0, 1000, 5000, 20},
      {2, 1000, 5000, 22},
  };
  const Mist::HLSTrackTiming tiedVideo = {3, 1000, 5000, 23};
  const size_t tiePaired = Mist::selectHLSAudioTrack(tiedVideo, tiedAudioTracks);
  if (tiePaired != 2){
    std::cerr << "selected tied audio track " << tiePaired
              << ", expected nearest source ID track 2" << std::endl;
    return 1;
  }

  if (Mist::selectHLSAudioTrack(lowVideo, {}) != INVALID_TRACK_ID){
    std::cerr << "empty audio set did not return INVALID_TRACK_ID" << std::endl;
    return 1;
  }

  return 0;
}

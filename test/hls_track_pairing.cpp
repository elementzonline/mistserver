#include "src/output/hls_track_pairing.h"

#include <iostream>
#include <vector>

int main(){
  const std::vector<Mist::HLSTrackTiming> videoTracks = {
      {0, 1000, 7000, 10},
      {2, 1000, 5000, 12},
  };
  const std::vector<Mist::HLSTrackTiming> audioTracks = {
      // Track 1 is deliberately closer in time to the second video. Equal
      // video/audio counts must still produce stable ordinal sibling pairs.
      {1, 1000, 5000, 11},
      {3, 1000, 7000, 13},
  };

  const size_t paired = Mist::selectHLSAudioTrack(videoTracks[1], videoTracks, audioTracks);
  if (paired != 3){
    std::cerr << "selected audio track " << paired
              << ", expected stable sibling track 3" << std::endl;
    return 1;
  }

  const Mist::HLSTrackTiming lowVideo = {2, 1000, 5000, 12};
  const std::vector<Mist::HLSTrackTiming> singleVideo = {lowVideo};
  const std::vector<Mist::HLSTrackTiming> tiedAudioTracks = {
      {0, 1000, 5000, 20},
      {2, 1000, 5000, 22},
  };
  const Mist::HLSTrackTiming tiedVideo = {3, 1000, 5000, 23};
  const size_t tiePaired = Mist::selectHLSAudioTrack(tiedVideo, singleVideo, tiedAudioTracks);
  if (tiePaired != 2){
    std::cerr << "selected tied audio track " << tiePaired
              << ", expected nearest source ID track 2" << std::endl;
    return 1;
  }

  if (Mist::selectHLSAudioTrack(lowVideo, singleVideo, {}) != INVALID_TRACK_ID){
    std::cerr << "empty audio set did not return INVALID_TRACK_ID" << std::endl;
    return 1;
  }

  return 0;
}

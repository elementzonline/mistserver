#include "src/input/input_hls.h"

#include <iostream>
#include <mist/config.h>

class TestInputHLS : public Mist::InputHLS{
public:
  explicit TestInputHLS(Util::Config *cfg) : Mist::InputHLS(cfg){}

  uint64_t mapPacketTime(uint64_t packetTime, uint64_t trackId, uint64_t playlistId){
    return getPacketTime(packetTime, trackId, playlistId);
  }

  void permitRemap(){allowRemap = true;}
};

int main(){
  Util::Config config("hls_timestamp_test");
  TestInputHLS input(&config);

  // Playlist 0 uses track 1 and is far ahead of playlist 1. The track ID
  // intentionally collides with playlist 1's ID to reproduce the bug.
  input.permitRemap();
  input.mapPacketTime(10000, 1, 0);

  // Playlist 1 must establish its own 20 ms packet interval.
  input.permitRemap();
  input.mapPacketTime(2000, 2, 1);
  input.mapPacketTime(2020, 2, 1);

  // A source timestamp reset on playlist 1 should continue at 2040 ms.
  input.permitRemap();
  const uint64_t mapped = input.mapPacketTime(100, 2, 1);
  if (mapped != 2040){
    std::cerr << "playlist 1 mapped reset timestamp to " << mapped
              << ", expected 2040" << std::endl;
    return 1;
  }
  return 0;
}

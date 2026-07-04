#include <cstdlib>
#include <iostream>
#include <string>
#include <mist/http_parser.h>

int main(){
  const char *inRaw = getenv("T_SHIFT");
  const char *expRaw = getenv("T_STARTUNIX");
  std::string input = inRaw ? inRaw : "";
  std::string expected = expRaw ? expRaw : "";
  std::string got = HTTP::flussonicShiftToStartunix(input);
  if (got != expected){
    std::cerr << "flussonicShiftToStartunix(\"" << input << "\") = \"" << got
              << "\", expected \"" << expected << "\"" << std::endl;
    return 1;
  }
  return 0;
}

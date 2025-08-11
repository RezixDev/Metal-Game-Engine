#pragma once
#include <unordered_set>
#include <cstdint>

class InputManager {
public:
  static InputManager& instance();
  void keyDown(uint16_t k);
  void keyUp(uint16_t k);
  bool down(uint16_t k) const;
private:
  std::unordered_set<uint16_t> s;
};

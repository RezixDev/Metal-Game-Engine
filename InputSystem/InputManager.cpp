#include "InputManager.h"
InputManager& InputManager::instance(){ static InputManager i; return i; }
void InputManager::keyDown(uint16_t k){ s.insert(k); }
void InputManager::keyUp(uint16_t k){ s.erase(k); }
bool InputManager::down(uint16_t k) const{ return s.count(k)!=0; }

#include "client/Camera.hpp"

// CameraController is intentionally implemented in the header because its
// small deterministic transform is shared by the headless parity tests and
// the SDL client without introducing a second stateful camera object.

#pragma once

namespace freertos {
// Native journal tests run in ordinary host threads.
inline bool is_inside_interrupt() { return false; }
} // namespace freertos

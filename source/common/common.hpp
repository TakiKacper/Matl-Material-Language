#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>

#define rethrow_error() if (error != "") return
#define throw_error(condition, _error) if (condition) { error = _error; return; }
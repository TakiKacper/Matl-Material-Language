#pragma once

#define rethrow_error() if (error != "") return
#define throw_error(condition, _error) if (condition) { error = _error; return; }
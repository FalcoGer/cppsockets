#pragma once

#include "TypeWrapper.h"
#include <string>

namespace CPPSockets
{

class NetAddress :
        public TypeWrapper<std::string>
{};

} // namespace CPPSockets

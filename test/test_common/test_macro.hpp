#pragma once

#include <boost/ut.hpp>

#define UNEXPECT(message) boost::ut::expect(false) << message

#define EXPECT(message) boost::ut::expect(false)
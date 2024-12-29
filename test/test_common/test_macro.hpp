#pragma once

#include <boost/ut.hpp>

#define UNEXPECT(message) boost::ut::expect(false) << message

#define EXPECT(v) boost::ut::expect(v)

#define TEST(message) message##_test
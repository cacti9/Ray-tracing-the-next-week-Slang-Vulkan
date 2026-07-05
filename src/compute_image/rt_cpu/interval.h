#pragma once

#include "renderer_types.h"

struct Interval {
  precision_type min;
  precision_type max;

  Interval() : min(INF), max(-INF) {}

  Interval(precision_type min_, precision_type max_) : min(min_), max(max_) {}

  Interval(const Interval& a, const Interval& b) {
    min = a.min <= b.min ? a.min : b.min;
    max = a.max >= b.max ? a.max : b.max;
  }

  precision_type size() const {
    return max - min;
  }

  bool contains(precision_type x) const {
    return min <= x && x <= max;
  }

  bool surrounds(precision_type x) const {
    return min < x && x < max;
  }

  precision_type clamp(precision_type x) const {
    if (x < min) {
      return min;
    }
    if (x > max) {
      return max;
    }
    return x;
  }

  Interval expand(precision_type delta) const {
    const precision_type padding = delta / 2;
    return {min - padding, max + padding};
  }

  static const Interval empty;
  static const Interval universe;
};

inline const Interval Interval::empty = Interval(INF, -INF);
inline const Interval Interval::universe = Interval(-INF, INF);

inline Interval operator+(const Interval& interval, precision_type displacement) {
  return {interval.min + displacement, interval.max + displacement};
}

inline Interval operator+(precision_type displacement, const Interval& interval) {
  return interval + displacement;
}

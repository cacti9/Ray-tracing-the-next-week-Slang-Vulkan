#pragma once

#include "interval.h"
#include "ray.h"

struct Aabb {
  Interval x;
  Interval y;
  Interval z;

  Aabb() = default;

  Aabb(const Interval& x_, const Interval& y_, const Interval& z_) : x(x_), y(y_), z(z_) {}

  Aabb(const Aabb& box0, const Aabb& box1) : x(box0.x, box1.x), y(box0.y, box1.y), z(box0.z, box1.z) {}

  Aabb(const glm::vec<3, precision_type>& a, const glm::vec<3, precision_type>& b) {
    x = (a[0] <= b[0]) ? Interval(a[0], b[0]) : Interval(b[0], a[0]);
    y = (a[1] <= b[1]) ? Interval(a[1], b[1]) : Interval(b[1], a[1]);
    z = (a[2] <= b[2]) ? Interval(a[2], b[2]) : Interval(b[2], a[2]);
  }

  const Interval& axis_interval(int n) const {
    if (n == 1) {
      return y;
    }
    if (n == 2) {
      return z;
    }
    return x;
  }

  int longest_axis() const {
    if (x.size() > y.size()) {
      return x.size() > z.size() ? 0 : 2;
    }
    return y.size() > z.size() ? 1 : 2;
  }
};

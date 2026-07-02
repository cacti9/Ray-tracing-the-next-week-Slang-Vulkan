implementing rt;

public struct interval {
  public precision_type min;
  public precision_type max;

  static interval empty() {
    interval i;
    i.min = INF;
    i.max = -INF;
    return i;
  }

  static interval universe() {
    interval i;
    i.min = -INF;
    i.max = INF;
    return i;
  }

  precision_type size() {
    return max - min;
  }

  bool contains(precision_type x) {
    return min <= x && x <= max;
  }

  bool surrounds(precision_type x) {
    return min < x && x < max;
  }
};

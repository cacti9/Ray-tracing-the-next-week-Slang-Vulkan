implementing rt;

public struct ConstantMedium {
  public uint boundary_hittable_index;
  public precision_type neg_inv_density;
  public uint phase_function_material_index; // isotropic


  internal struct BoundaryTraversalEntry {
    internal uint hittable_index;
    internal ray local_ray;
  };

  private static void record_two_earliest_hits(
    hit_record candidate,
    inout bool found_first,
    inout bool found_second,
    inout hit_record first_hit,
    inout hit_record second_hit
  ) {
    if (!found_first || candidate.t < first_hit.t) {
      if (found_first && first_hit.t > candidate.t + 0.0001) {
        second_hit = first_hit;
        found_second = true;
      }
      first_hit = candidate;
      found_first = true;
    } else if (candidate.t > first_hit.t + 0.0001 && (!found_second || candidate.t < second_hit.t)) {
      second_hit = candidate;
      found_second = true;
    }
  }

  internal static bool hit_boundary_pair(
    StructuredBuffer<Hittable> world,
    uint root_hittable_index,
    ray r,
    interval ray_t,
    out hit_record rec1,
    out hit_record rec2
  ) {
    bool found_first = false;
    bool found_second = false;
    precision_type second_so_far = ray_t.max;
    BoundaryTraversalEntry stack[64];
    uint stack_size = 0;

    BoundaryTraversalEntry root;
    root.hittable_index = root_hittable_index;
    root.local_ray = r;
    stack[stack_size++] = root;

    while (stack_size > 0) {
      BoundaryTraversalEntry entry = stack[--stack_size];
      uint hittable_index = entry.hittable_index;

      switch (world[hittable_index].type) {
      // case HittableType::BvhNode: // bvh must be built at last on scene building. this should not match.
      //   break;
      case HittableType::HittableList:
        if (world[hittable_index].bbox.hit(entry.local_ray, interval(ray_t.min, second_so_far))) {
          HittableList list = reinterpret<HittableList>(world[hittable_index].data);
          const uint pushed_count = min(list.count, 64 - stack_size);
          for (uint child_offset = 0; child_offset < pushed_count; child_offset++) {
            BoundaryTraversalEntry child_entry = entry;
            child_entry.hittable_index = list.first + child_offset;
            stack[stack_size++] = child_entry;
          }
        }
        break;
      case HittableType::Translate:
        if (stack_size < 64) {
          Translate translate = reinterpret<Translate>(world[hittable_index].data);
          entry.hittable_index = translate.hittable_index;
          entry.local_ray = Translate::inverse_translate_ray(entry.local_ray, translate.offset);
          stack[stack_size++] = entry;
        }
        break;
      case HittableType::RotateY:
        if (stack_size < 64) {
          RotateY rotate = reinterpret<RotateY>(world[hittable_index].data);
          entry.hittable_index = rotate.hittable_index;
          entry.local_ray = RotateY::inverse_rotate_y_ray(entry.local_ray, rotate.sin_theta, rotate.cos_theta);
          stack[stack_size++] = entry;
        }
        break;
      case HittableType::Sphere:
        {
          Sphere sphere = reinterpret<Sphere>(world[hittable_index].data);
          hit_record boundary_hit;
          if (sphere.hit(entry.local_ray, interval(ray_t.min, second_so_far), boundary_hit)) {
            record_two_earliest_hits(boundary_hit, found_first, found_second, rec1, rec2);
            if (found_second)
              second_so_far = rec2.t;
            if (sphere.hit(entry.local_ray, interval(boundary_hit.t + 0.0001, second_so_far), boundary_hit)) {
              record_two_earliest_hits(boundary_hit, found_first, found_second, rec1, rec2);
              if (found_second)
                second_so_far = rec2.t;
            }
          }
        }
        break;
      case HittableType::Quad:
        {
          Quad quad = reinterpret<Quad>(world[hittable_index].data);
          hit_record boundary_hit;
          if (quad.hit(entry.local_ray, interval(ray_t.min, second_so_far), boundary_hit)) {
            record_two_earliest_hits(boundary_hit, found_first, found_second, rec1, rec2);
            if (found_second)
              second_so_far = rec2.t;
          }
        }
        break;
      default:
        break;
      }
    }

    return found_second;
  }
};


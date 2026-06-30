implementing rt;

public struct hittable_list<T : hittable, let N : int> : hittable
{
    T objects[N];
    uint object_count;

    [mutating]
    public void clear()
    {
        object_count = 0;
    }

    [mutating]
    public void add(T object)
    {
        objects[object_count] = object;
        object_count++;
    }

    public bool hit(
        ray r,
        interval ray_t,
        inout hit_record rec)
    {
        hit_record temp_rec;
        bool hit_anything = false;
        precision_type closest_so_far = ray_t.max;

        for (uint i = 0; i < object_count; i++)
        {
          if (objects[i].hit(r, {ray_t.min, closest_so_far}, temp_rec))
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }
}
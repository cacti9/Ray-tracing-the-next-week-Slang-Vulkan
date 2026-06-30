implementing rt;

public struct sphere : hittable {
     bool hit(const ray r, interval ray_t, inout hit_record rec) {
        vec3 oc = center - r.orig;
        precision_type a = dot(r.dir, r.dir);
        precision_type h = dot(r.dir, oc);
        precision_type c = dot(oc, oc) - radius*radius;

        precision_type discriminant = h*h - a*c;
        if (discriminant < 0)
            return false;

        precision_type sqrtd = sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        precision_type root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root)) 
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        rec.normal = (rec.p - center) / radius;
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);

        return true;
    }

  public point3 center;
  public precision_type radius;
};
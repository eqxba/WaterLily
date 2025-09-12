#ifndef MATH_H
#define MATH_H

uint hash(uint seed)
{
    seed = (seed + 0x7ed55d16) + (seed << 12);
    seed = (seed ^ 0xc761c23c) ^ (seed >> 19);
    seed = (seed + 0x165667b1) + (seed << 5);
    seed = (seed + 0xd3a2646c) ^ (seed << 9);
    seed = (seed + 0xfd7046c5) + (seed << 3);
    seed = (seed ^ 0xb55a4f09) ^ (seed >> 16);

    return seed;
}

vec4 hashToColor(uint hash)
{
    vec3 color = vec3(float(hash & 255), float((hash >> 8) & 255), float((hash >> 16) & 255)) / 255.0;
    return vec4(color, 1.0);
}

vec3 rotateQuat(vec3 v, vec4 q)
{
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

// Adapted version of https://gist.github.com/JarkkoPFC/1186bc8a861dae3c8339b0cda4e6cdb3
// Negated all entries of center.z (except squares) as original implementation assumes camera looks in +z
// Negated P11 (as by default we flip Y with it) to make this function return this kind of NDC:
//        +Y
//         ^
//         |
// -X <----+----> X+
//         |
//         v
//        -Y
bool sphereNdcExtents(vec3 center, float radius, float p00, float p11, float near, out vec4 lbrt)
{
    if (center.z + radius > -near) // Sphere must be completely in front of the camera
    {
        return false;
    }

    float rad2 = radius * radius, d = -center.z * radius;
    
    float hv = sqrt(center.x * center.x + center.z * center.z - rad2);
    float ha = center.x * hv, hb = center.x * radius, hc = -center.z * hv;
    lbrt.x = (ha - d) * p00 / (hc + hb); // left
    lbrt.z = (ha + d) * p00 / (hc - hb); // right
    
    float vv = sqrt(center.y * center.y + center.z * center.z - rad2);
    float va = center.y * vv, vb = center.y * radius, vc = -center.z * vv;
    lbrt.y = (va - d) * -p11 / (vc + vb); // bottom
    lbrt.w = (va + d) * -p11 / (vc - vb); // top
    
    return true;
}

#endif
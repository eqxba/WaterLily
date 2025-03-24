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

#endif
#version 450 core

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

uniform sampler3D cubes_tex;
uniform mat4 camera_view_mat;

void swap(inout float a, inout float b)
{
	float d = a;
	a = b;
	b = d;
}

vec3 perspective_ray(in vec3 center_dir, in vec2 normalized_pixel_coord)
{
	return normalize(vec3(center_dir.xy + (normalized_pixel_coord * 2.0 - 1.0), center_dir.z * 10));
}

bool is_inside(ivec3 pos)
{
	ivec3 size = textureSize(cubes_tex, 0);
	return pos.x >= 0 && pos.y >= 0 && pos.z >= 0 && pos.x < size.x && pos.y < size.y && pos.z < size.z;
}

bool is_inside(vec3 pos)
{
	ivec3 size = textureSize(cubes_tex, 0);
	return pos.x >= 0 && pos.y >= 0 && pos.z >= 0 && pos.x < size.x && pos.y < size.y && pos.z < size.z;
}

void intersect_aabb(in vec3 ray_pos, in vec3 ray_dir, in vec3 bottom_left, in vec3 top_right, out bool intersected, out vec3 intersect_pos)
{
	intersected = false;

	float tmin = (bottom_left.x - ray_pos.x) / ray_dir.x;
	float tmax = (top_right.x - ray_pos.x) / ray_dir.x;

	if (tmin > tmax) swap(tmin, tmax);

	float tymin = (bottom_left.y - ray_pos.y) / ray_dir.y;
	float tymax = (top_right.y - ray_pos.y) / ray_dir.y;

	if (tymin > tymax) swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (bottom_left.z - ray_pos.z) / ray_dir.z;
	float tzmax = (top_right.z - ray_pos.z) / ray_dir.z;

	if (tzmin > tzmax) swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	intersected = true;
	intersect_pos = ray_pos + ray_dir * tmin;
}

void intersect_cubes(in ivec3 inside_cube, in vec3 ray_pos, in vec3 ray_dir,
	out bool intersected, out ivec3 intersect_cube, out vec3 intersect_pos, out vec3 intersect_normal)
{
	intersected = false;
	
	int wall_x = inside_cube.x + (ray_dir.x < 0 ? 0 : 1);
	int wall_y = inside_cube.y + (ray_dir.y < 0 ? 0 : 1);
	int wall_z = inside_cube.z + (ray_dir.z < 0 ? 0 : 1);

	float t_x = ray_dir.x / (ray_pos.x - wall_x);
	float t_y = ray_dir.y / (ray_pos.y - wall_y);
	float t_z = ray_dir.z / (ray_pos.z - wall_z);

	// needs glsl 4.4 or higher
	if (t_x < 0) t_x = 1.0 / 0.0;
	if (t_y < 0) t_y = 1.0 / 0.0;
	if (t_z < 0) t_z = 1.0 / 0.0;

	float t_min;
	if (t_x < t_y && t_x < t_z) {
		t_min = t_x;
		intersected = true;
		intersect_normal = vec3(-sign(ray_dir.x), 0, 0);
		intersect_cube = ivec3(inside_cube - intersect_normal);
	} else if (t_y < t_z) {
		t_min = t_y;
		intersected = true;
		intersect_normal = vec3(0, -sign(ray_dir.y), 0);
		intersect_cube = ivec3(inside_cube - intersect_normal);
	} else {
		t_min = t_z;
		intersected = true;
		intersect_normal = vec3(0, 0, -sign(ray_dir.z));
		intersect_cube = ivec3(inside_cube - intersect_normal);
	}

	intersect_pos = ray_pos + ray_dir * t_min;
}

void main() {
	ivec2 dims = imageSize(img_output);
	ivec3 cubes_dims = textureSize(cubes_tex, 0);
  ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
	vec2 centered_coord = pixel_coord - dims / 2.0;
	vec2 normalized_pixel_coord = vec2(pixel_coord) / dims;

	vec3 ray_origin = (camera_view_mat * vec4((pixel_coord + centered_coord) / 2.f / 16.f, 0, 1)).xyz;
	vec3 ray_dir = normalize( (camera_view_mat * vec4(0, 0, -1, 1)).xyz );

	bool store = false;
	vec4 pixel;

	{
		bool aabb_intersected;
		vec3 aabb_intersect_pos;
		intersect_aabb(ray_origin, ray_dir, vec3(0, 0, 0), vec3(cubes_dims), aabb_intersected, aabb_intersect_pos);

		if (aabb_intersected) {
			vec3 cubes_intersect_pos;
			vec3 cubes_intersect_normal;
			ivec3 inside_cube = ivec3(floor(aabb_intersect_pos));
			float cube = texture(cubes_tex, aabb_intersect_pos / cubes_dims).r;

			if (cube > 0) {
				store = true;
				pixel = vec4(0, 0, 0, 1);
			} else {
				for (int i = 0; i < 100; ++i) {
					bool intersected;
					ivec3 intersect_cube;
					vec3 intersect_pos;
					vec3 intersect_normal;
					intersect_cubes(inside_cube, ray_origin, ray_dir, intersected, intersect_cube, intersect_pos, intersect_normal);

					if (intersected && is_inside(intersect_cube)) {
						cube = texture(cubes_tex, vec3(intersect_cube) / cubes_dims).r;

						if (cube > 0) {
							store = true;
							pixel = vec4(cube * vec3(0, 1, 0) * clamp(dot(ray_dir, -intersect_normal), .2, 1), 1);
							break;
						}
						inside_cube = intersect_cube;
					} else {
						store = true;
						pixel = vec4(0, 0, 1, 1);
						break;
					}
				}
			}

			/* for (int i = 0; i < 10; i++) { */
			/* 	intersect_cubes(aabb_intersect_pos, ray_dir, cur_cube, cubes_intersect_pos); */
			/* 	cur_cube = ivec3(floor(cubes_intersect_pos)); */
			/* 	if (is_inside(cubes_intersect_pos)) { */
			/* 		float cube = texture(cubes_tex, cubes_intersect_pos).r; */
			/* 		if (cube > 0) { */
			/* 			store = true; */
			/* 			cubes_intersected = true; */
			/* 			pixel = vec4(cube, 0, 0, 1); */
			/* 			break; */
			/* 		} */
			/* 	} */
			/* } */
		}
	}

  
	imageStore(img_output, pixel_coord, store ? pixel : vec4(0, 0, .5, 1));
}

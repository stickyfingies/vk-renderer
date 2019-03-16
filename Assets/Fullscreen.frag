#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv_coords;

layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform sampler2D raytraced_image;

layout(set = 0, binding = 1) uniform sampler2D prev_raytraced_image;

const float inv_size = 1.0 / 2048.0;

void main()
{
	const vec2 uv = { uv_coords.s, 1.0 - uv_coords.t };

	const vec3 color_0 = texture(raytraced_image, uv).rgb;
	const vec3 color_1 = texture(prev_raytraced_image, uv).rgb;

	const vec3 N = texture(raytraced_image, uv + vec2(0.0, 1.0)  * inv_size).rgb;
	const vec3 S = texture(raytraced_image, uv + vec2(0.0, -1.0) * inv_size).rgb;
	const vec3 E = texture(raytraced_image, uv + vec2(1.0, 0.0)  * inv_size).rgb;
	const vec3 W = texture(raytraced_image, uv + vec2(-1.0, 0.0) * inv_size).rgb;

	const float vT = dot(color_0 - color_1, color_0 - color_1); // Temporal variance

	const vec3 final = (vT > 0.0005 ? (N + S + E + W) / 4.0 : color_0);

	frag_color = vec4(final, 1.0);
}

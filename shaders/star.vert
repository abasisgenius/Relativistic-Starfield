layout (location = 0) in vec3 position;
layout (location = 1) in float rest_wavelength;
layout (location = 2) in float rest_intensity;

uniform mat4 viewProjection;
uniform vec3 ship_position;
uniform float beta;
uniform float point_size;

out float apparent_wavelength;
out float apparent_intensity;
out float star_distance;

void main() {
    float gamma = 1.0 / sqrt(max(0.000001, 1.0 - beta * beta));

    vec3 raw_r_vec = position - ship_position;
    float box_size = 2000.0;
    float half_box = box_size * 0.5;
    vec3 r_vec = mod(raw_r_vec + half_box, box_size) - half_box;

    float r = max(length(r_vec), 0.000001);
    star_distance = r;

    float cos_theta = clamp(-r_vec.z / r, -1.0, 1.0);
    float cos_theta_prime = (cos_theta + beta) / (1.0 + beta * cos_theta);
    cos_theta_prime = clamp(cos_theta_prime, -1.0, 1.0);
    float sin_theta = sqrt(max(0.0, 1.0 - cos_theta * cos_theta));
    float sin_theta_prime = sqrt(max(0.0, 1.0 - cos_theta_prime * cos_theta_prime));
    float xy_ratio = sin_theta_prime / max(sin_theta, 0.000001);

    vec3 apparent_r_vec = vec3(r_vec.x * xy_ratio, r_vec.y * xy_ratio, r * -cos_theta_prime);
    vec3 apparent_position = ship_position + apparent_r_vec;

    float doppler = gamma * (1.0 - beta * cos_theta);
    apparent_wavelength = rest_wavelength * doppler;

    float frequency_shift = 1.0 / max(doppler, 0.000001);
    apparent_intensity = rest_intensity * pow(frequency_shift, 4.0);

    gl_Position = viewProjection * vec4(apparent_position, 1.0);
    gl_PointSize = point_size;
}

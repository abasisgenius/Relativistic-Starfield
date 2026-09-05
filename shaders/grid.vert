layout (location = 0) in vec3 position;

uniform mat4 viewProjection;
uniform float beta;

void main() {
    float r = max(length(position), 0.000001);

    // Relativistic Aberration for motion along -Z
    float cos_theta = clamp(-position.z / r, -1.0, 1.0);
    float cos_theta_prime = clamp((cos_theta + beta) / (1.0 + beta * cos_theta), -1.0, 1.0);
    float sin_theta = sqrt(max(0.0, 1.0 - cos_theta * cos_theta));
    float sin_theta_prime = sqrt(max(0.0, 1.0 - cos_theta_prime * cos_theta_prime));

    float xy_ratio = sin_theta_prime / max(sin_theta, 0.000001);

    vec3 apparent_position = vec3(
        position.x * xy_ratio,
        position.y * xy_ratio,
        -r * cos_theta_prime
    );

    gl_Position = viewProjection * vec4(apparent_position, 1.0);
}

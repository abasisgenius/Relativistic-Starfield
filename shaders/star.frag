in float apparent_wavelength;
in float apparent_intensity;
in float star_distance;

uniform float exposure;
uniform float fog_start;
uniform float fog_end;

out vec4 FragColor;

vec3 wavelengthToRGB(float W) {
    float R = 0.0, G = 0.0, B = 0.0;
    if (W >= 380.0 && W < 440.0) { R = -(W - 440.0) / 60.0; B = 1.0; }
    else if (W >= 440.0 && W < 490.0) { G = (W - 440.0) / 50.0; B = 1.0; }
    else if (W >= 490.0 && W < 510.0) { G = 1.0; B = -(W - 510.0) / 20.0; }
    else if (W >= 510.0 && W < 580.0) { R = (W - 510.0) / 70.0; G = 1.0; }
    else if (W >= 580.0 && W < 645.0) { R = 1.0; G = -(W - 645.0) / 65.0; }
    else if (W >= 645.0 && W <= 750.0) { R = 1.0; }
    return vec3(R, G, B);
}

void main() {
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float disc = dot(p, p);
    if (disc > 1.0) discard;

    vec3 color = wavelengthToRGB(apparent_wavelength);
    float alpha = 1.0;

    if (apparent_wavelength < 380.0 || apparent_wavelength > 750.0) alpha = 0.0;
    else if (apparent_wavelength < 400.0) alpha = (apparent_wavelength - 380.0) / 20.0;
    else if (apparent_wavelength > 700.0) alpha = (750.0 - apparent_wavelength) / 50.0;

    float bloom = clamp((apparent_intensity - 5.0) / 10.0, 0.0, 1.0);
    alpha = max(alpha, bloom);
    color = mix(color, vec3(1.0), bloom);

    vec3 final_color = color * apparent_intensity * exposure;
    final_color = final_color / (final_color + vec3(1.0));

    float fog_fade = 1.0 - smoothstep(fog_start, fog_end, star_distance);
    alpha *= fog_fade;

    FragColor = vec4(final_color, alpha);
}

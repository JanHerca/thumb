#extension GL_ARB_texture_rectangle : enable

uniform samplerRect L_map;
uniform samplerRect R_map;
uniform float       cycle;

varying vec3 L_phase;
varying vec3 R_phase;
uniform vec2 k;
uniform vec2 d;

void main()
{
    vec2 p = (gl_FragCoord.xy + d) * k;

    const vec4 L = textureRect(L_map, p);
    const vec4 R = textureRect(R_map, p);

    vec3 Lk = step(vec3(cycle), fract(L_phase));
    vec3 Rk = step(vec3(cycle), fract(R_phase));

    gl_FragColor = vec4(max(L.rgb * Lk, R.rgb * Rk), 1.0);
}

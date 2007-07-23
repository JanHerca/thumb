#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect cyl;
uniform sampler2D     normal;

uniform vec3 axis;

void main()
{
    vec4 c = texture2DRect(cyl, gl_FragCoord.xy);
    vec2 t = (gl_TextureMatrix[1] * vec4(c.xy, 0.0, 1.0)).xy;

    vec2 a = step(vec2(0.0), t) * step(t, vec2(1.0));

    vec3 N = normalize(vec3(c.zw, sqrt(1.0 - c.z * c.z - c.w * c.w)));

    mat3 T;

    T[0] = cross(axis, N);
    T[1] = cross(N, T[0]);
    T[2] = N;

    vec3 n = T * (texture2D(normal, t).xyz * 2.0 - 1.0);

//  gl_FragColor = vec4((n + 1.0) * 0.5, a.x * a.y);
    gl_FragColor = vec4((N + 1.0) * 0.5, 1.0);
}

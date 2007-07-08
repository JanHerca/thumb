#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect src;
uniform sampler2DRect lut;
uniform float         siz;

void main()
{
    const float pi = 3.14159265358979323844;

    // Look up texture coordinates of parent points.

    vec2 k  = texture2DRect(lut, vec2(gl_FragCoord.x, 0.5)).xw * 65535.0;;
    vec2 t0 = texture2DRect(src, vec2(k.x, gl_FragCoord.y)).xy;
    vec2 t1 = texture2DRect(src, vec2(k.y, gl_FragCoord.y)).xy;

    // Evaluate the haversine geodesic midpoint.

    vec3 v = vec3(t0.y, t1.y, t1.x - t0.x);

    vec3 c = cos(v);
    vec3 s = sin(v);

    vec2 b = vec2(c.x + c.y * c.z,
                        c.y * s.z);

    vec2 y = vec2(b.y, s.x + s.y);
    vec2 x = vec2(b.x, length(b));

    vec2 m = atan(y, x) + vec2(t0.x, 0.0);

    // Compute normalized [0,1] lat/lon.

    vec2 off = vec2(pi, 0.5 * pi);
    vec2 scl = vec2(2.0 * pi, pi);

    gl_FragColor = vec4(m, (m + off) / scl);
}
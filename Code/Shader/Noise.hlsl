// White noise
float random2(in float2 vec) {
    return frac(sin(dot(vec.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

float2 random22(in float2 vec) {
    vec = float2(dot(vec, float2(127.1, 311.7)),
                 dot(vec, float2(269.5, 183.3)));

    return frac(sin(vec) * 43758.5453123);
}

// Value noise
/*

a------------b
|            |
|            |
|            |
|            |
|            |
c------------d
4点のみ計算
内側のピクセルは補完により求める

*/
float valueNoise2(in float2 vec) {
    float2 ivec = floor(vec);
    float2 fvec = frac(vec);
    
    float a = random2(ivec + float2(0.0, 0.0));
    float b = random2(ivec + float2(1.0, 0.0));
    float c = random2(ivec + float2(0.0, 1.0));
    float d = random2(ivec + float2(1.0, 1.0));
    
    fvec = smoothstep(0.0f, 1.0f, fvec);
    
    return lerp(lerp(a, b, fvec.x), lerp(c, d, fvec.x), fvec.y);
}

// Perlin noise
float perlinNoise2(in float2 vec) {
    float2 ivec = floor(vec);
    float2 fvec = frac(vec);
    
    float a = dot(random22(ivec + float2(0.0, 0.0)) * 2.0 - 1.0, fvec - float2(0.0, 0.0));
    float b = dot(random22(ivec + float2(1.0, 0.0)) * 2.0 - 1.0, fvec - float2(1.0, 0.0));
    float c = dot(random22(ivec + float2(0.0, 1.0)) * 2.0 - 1.0, fvec - float2(0.0, 1.0));
    float d = dot(random22(ivec + float2(1.0, 1.0)) * 2.0 - 1.0, fvec - float2(1.0, 1.0));
    
    fvec = smoothstep(0.0f, 1.0f, fvec);
    
    return lerp(lerp(a, b, fvec.x), lerp(c, d, fvec.x), fvec.y); // -1 ~ 1
}

// Fractal perlin noise
float fbm2(in float2 vec, int octave) {
    float value = 0.0;
    float amplitude = 0.5;
    
    for (int i = 0; i < octave; i++) {
        value += amplitude * perlinNoise2(vec);
        vec *= 2.0; // x0.5 Texture size per loop
        amplitude *= 0.5;
    }

    return value; // -1 ~ 1
}
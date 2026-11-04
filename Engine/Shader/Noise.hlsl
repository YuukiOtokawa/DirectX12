// White noise
float random2(in float2 vec) {
    return frac(sin(dot(vec.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

float2 random22(in float2 vec) {
    vec = float2(dot(vec, float2(127.1, 311.7)),
                 dot(vec, float2(269.5, 183.3)));

    return frac(sin(vec) * 43758.5453123);
}

float3 random33(in float3 vec) {
    vec = float3(dot(vec, float3(127.1, 311.7, 326.4)),
                 dot(vec, float3(269.5, 183.3, 138.2)),
                 dot(vec, float3(753.1, 432.7, 612.2)));
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

float voronoi2(in float2 vec) {
    float2 ivec = floor(vec);
    float2 fvec = frac(vec);
    
    float value = 1.0;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float2 offset = float2(x, y);
            
            float2 position = random22(ivec + offset);

            float dist = distance(position + offset, fvec);

            value = min(value, dist);

        }

    }
    
    return value;
}

float perlinNoise3(in float3 vec) {
    float3 ivec = floor(vec);
    float3 fvec = frac(vec);
    
    float a = dot(random33(ivec + float3(0.0, 0.0, 1.0)) * 2.0 - 1.0, fvec - float3(0.0, 0.0, 0.0));
    float b = dot(random33(ivec + float3(1.0, 0.0, 0.0)) * 2.0 - 1.0, fvec - float3(1.0, 0.0, 0.0));
    float c = dot(random33(ivec + float3(0.0, 1.0, 0.0)) * 2.0 - 1.0, fvec - float3(0.0, 1.0, 0.0));
    float d = dot(random33(ivec + float3(1.0, 1.0, 0.0)) * 2.0 - 1.0, fvec - float3(1.0, 1.0, 0.0));
    
    float e = dot(random33(ivec + float3(0.0, 0.0, 1.0)) * 2.0 - 1.0, fvec - float3(0.0, 0.0, 1.0));
    float f = dot(random33(ivec + float3(1.0, 0.0, 1.0)) * 2.0 - 1.0, fvec - float3(1.0, 0.0, 1.0));
    float g = dot(random33(ivec + float3(0.0, 1.0, 1.0)) * 2.0 - 1.0, fvec - float3(0.0, 1.0, 1.0));
    float h = dot(random33(ivec + float3(1.0, 1.0, 1.0)) * 2.0 - 1.0, fvec - float3(1.0, 1.0, 1.0));
    
    fvec = smoothstep(0.0, 1.0, fvec);
    
    float v1 = lerp(lerp(a, b, fvec.x), lerp(c, d, fvec.x), fvec.y);
    float v2 = lerp(lerp(f, g, fvec.x), lerp(g, h, fvec.x), fvec.y);
    
    return lerp(v1, v2, fvec.z);

}

float fbm3(in float3 vec, int octave) {
    float value = 0.0;
    float amplitude = 0.5;
    
    for (int i = 0; i < octave; i++) {
        value += amplitude * perlinNoise3(vec);
        vec *= 2.0; // x0.5 Texture size per loop
        amplitude *= 0.5;
    }

    return value; // -1 ~ 1
}
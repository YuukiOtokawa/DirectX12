// 疑似乱数
float random2(in float2 vec) {
    return frac(sin(dot(vec.xy, float2(12.9898, 78.233))) * 43758.5453123);
}
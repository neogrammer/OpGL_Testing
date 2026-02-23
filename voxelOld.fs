#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
flat in float TexLayer;

uniform sampler2DArray texArray;
uniform vec3 lightDir = normalize(vec3(0.3, 0.8, 0.2));

void main() {
    vec4 tex = texture(texArray, vec3(TexCoord, TexLayer));
    float ndl = max(dot(normalize(Normal), normalize(lightDir)), 0.0);
    float ambient = 0.25;
    vec3 lit = tex.rgb * (ambient + ndl * 0.75);
    FragColor = vec4(lit,1.0);    //(lit, tex.a);
}
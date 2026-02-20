#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    vec3 nrml = Normal;
    vec4 color1 = texture(texture1, TexCoord);
    vec4 color2 = texture(texture2, TexCoord);
    float alpha = color2.a;   // use second texture alpha
    vec3 finalColor = mix(color1.rgb, color2.rgb, 0.75);
    FragColor = color1; //vec4(finalColor, 1.0); // or alpha if desired
}
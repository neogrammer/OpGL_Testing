#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	 vec4 color1 = texture(texture1, TexCoord);
    vec4 color2 = texture(texture2, TexCoord);

    float alpha = color2.a;   // use second texture alpha
    vec3 finalColor = mix(color2.rgb, color1.rgb, alpha);

    FragColor = vec4(finalColor,1.0);
}
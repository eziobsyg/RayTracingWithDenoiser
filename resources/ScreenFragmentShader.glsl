#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform bool spatialDenoiser;
uniform float texelWidth; // 纹理的宽度分量（每个纹素的宽度）
uniform float texelHeight; // 纹理的高度分量（每个纹素的高度）
uniform sampler2D screenTexture;
uniform sampler2D historyluminance1Texture;
uniform sampler2D historyluminance2Texture;

float gaussian[5] = float[](1.0/16.0, 1.0/4.0, 3.0/8.0, 1.0/4.0, 1.0/16.0);

void main() {
	vec3 col = vec3(0.0);
	if(spatialDenoiser){
		col = vec3(0.0);
		float luminance1 = texture(historyluminance1Texture, TexCoords).r;
		float luminance2 = texture(historyluminance2Texture, TexCoords).r;
		float var = luminance2 - luminance1 * luminance1;
		// 计算高斯权重
		for (int x = -2; x <= 2; x++) {
			for (int y = -2; y <= 2; y++) {
				float weight = gaussian[x + 2] * gaussian[y + 2];
				vec2 offset = vec2(x * texelWidth, y * texelHeight);
				col += texture(screenTexture, TexCoords + offset).rgb * weight;
			}
		}
		float k = 50 * var;
		col = (1 - k) * texture(screenTexture, TexCoords).rgb + k * col;
		// col = vec3(k);
	}
	else{
		col = texture(screenTexture, TexCoords).rgb;
	}

	FragColor = vec4(col, 1.0);
}


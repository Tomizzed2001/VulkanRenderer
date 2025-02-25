#version 450
#extension GL_EXT_nonuniform_qualifier : require

#define PI		3.1415926535897932384626433832795
#define E		2.7182818284590452353602874713526
#define EPSILON 0.0000000000000000000000000000001

// Bring in the values from the vertex shader
layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in vec3 inPosition;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) flat in int inMatID;
layout (location = 5) in vec4 inLightDir;

// The world view uniform
layout(set = 0, binding = 0) uniform worldView
{
	mat4 projectionCameraMatrix;
	vec3 cameraPosition;
} view;

// The texture uniforms (Indexed by material ID)
layout (set = 1, binding = 0) uniform sampler2D textureColour[];
layout (set = 1, binding = 1) uniform sampler2D textureSpecular[];
layout (set = 1, binding = 2) uniform sampler2D textureNormalMap[];

// The lighting uniform
layout(set = 2, binding = 0, std140) uniform LightBuffer { 
    mat4 lightDirection;
    vec3 lightPosition;
	vec3 lightColour;
} light;

layout (set = 3, binding = 0) uniform sampler2DShadow textureShadow;

// The screen colour output
layout (location = 0) out vec4 outColour;

float GeometricShadowing(vec3 x, float alpha, vec3 h, vec3 n, float charFunction){
	float xDh = max(0, dot(x, h));
	float xDn = max(0, dot(x, n));
	float tan2 = (1 - xDn * xDn) / (xDn * xDn);

	return charFunction * (xDn / xDh) * (2 / (1 + sqrt(1 + alpha * tan2)));
}

void main()
{
	// Roughness
	float roughness = texture(textureSpecular[inMatID], inTexCoord).g * texture(textureSpecular[inMatID], inTexCoord).g;

	// Metalness
	float metalness = texture(textureSpecular[inMatID], inTexCoord).b;

	// Diffuse
	vec3 diffuse = texture(textureColour[inMatID], inTexCoord).rgb;

	// Normal map (Textures are BC5 (2-Channel))
	vec2 normal2D = texture(textureNormalMap[inMatID], inTexCoord).rg;
	// Map from the 0-1 range
	normal2D = normal2D * 2.0 - 1.0;
	// Construct the Z component and finish the normal
	vec3 normal = vec3(normal2D.x, normal2D.y, sqrt(1 - normal2D.x * normal2D.x - normal2D.y * normal2D.y));

	// Create the TBN matrix
	vec3 T = normalize(inTangent.xyz);
	vec3 N = normalize(inNormal);
	vec3 B = normalize(inTangent.w * (cross(N, T)));
	mat3 TBN = mat3(T,B,N);

	// Use the normal map for the normal
	normal = normalize(TBN * normal);

	// Light direction
	vec3 lightDirection = normalize(light.lightPosition - inPosition);

	// View direction
	vec3 viewDirection = normalize(view.cameraPosition - inPosition);

	// Half vector
	vec3 halfVector = normalize(0.5 * (viewDirection + lightDirection));

	// Ambient
	vec3 ambient = vec3(0.02,0.02,0.02) * diffuse;

	// Positive characteristic function
	float X = (1 + sign(roughness)) / 2;

	// Geometric term 
	float nDh = max(0, dot(normal, halfVector));
	float nDv = max(0, dot(normal, viewDirection));
	float nDl = max(0, dot(normal, lightDirection));
	//float G = min(1, min(2 * ((nDh * nDv) / (dot(viewDirection, halfVector) + EPSILON)) , 2 * ((nDh * nDl) / (dot(viewDirection, halfVector) + EPSILON))));
	float G = GeometricShadowing(viewDirection, roughness, halfVector, normal, X) * GeometricShadowing(lightDirection, roughness, halfVector, normal, X);

	// Normal distribution function
	float D = X * nDh * ( roughness / ( PI * ( (nDh * nDh) * (roughness - 1) + 1 ) * ( (nDh * nDh) * (roughness - 1) + 1 )));

	// Fresnel (Schlick approximation)
	vec3 F0 = (1-metalness) * vec3(0.04, 0.04, 0.04) + (metalness * diffuse);
	vec3 F = F0 + (1 - F0) * pow((1 - dot(halfVector, viewDirection)), 5);

	// Diffuse (Adjusted)
	vec3 lDiffuse = (diffuse / PI) * (vec3(1, 1, 1) - F) * (1 - metalness);

	// Specular term
	vec3 brdf = lDiffuse + ( ( D * F * G ) / ( 4 * nDv * nDl + EPSILON) ) ;

	// Create dynamic bias to prevent shadow acne
	float bias = max(0.01 * (1.0 - dot(inNormal, lightDirection)), 0.005);
	// How far from camera?
	float hitDepth = (inLightDir.xyz / inLightDir.w).z;	
	// 3x3 PCF
	float shadow = 0.0;
	vec2 offset = 1.0 / textureSize(textureShadow, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			vec4 pos = inLightDir + vec4(x * offset.x, y * offset.y, 0, 0);
			float firstDepth = textureProj(textureShadow, pos, 0.0);
			shadow += 1.f;
			if (hitDepth - bias > firstDepth){
				shadow -= 1.0;
			}    
		}    
	}
	shadow /= 9.0;

	// Rendering equation 
	// Out = Emissive + Ambient + BRDF * Light Colour * clampedDot(normal, light direction)
	vec3 pixelColour = ambient + (brdf * light.lightColour * nDl * shadow);

	// Output the colour
	outColour = vec4(pixelColour, 1.f);
}
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
    vec3 lightPosition;
	vec3 lightColour;
} light;

// The screen colour output
layout (location = 0) out vec4 outColour;

void main()
{
	// Roughness
	float roughness = texture(textureSpecular[inMatID], inTexCoord).g * texture(textureSpecular[inMatID], inTexCoord).g;

	// Metalness
	float metalness = texture(textureSpecular[inMatID], inTexCoord).b;

	// Diffuse
	vec3 diffuse = texture(textureColour[inMatID], inTexCoord).rgb;

	// Normal map
	vec3 normal = texture(textureNormalMap[inMatID], inTexCoord).rgb;
	// Map to the 0-1 range
	normal = normal * 2.0 - 1.0;
	// Flip the green channel as texture is in directX format
	normal.g = normal.g * -1;

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
	vec3 ambient = vec3(0.2,0.2,0.2) * diffuse;

	// Geometric term 
	float nDh = max(0, dot(normal, halfVector));
	float nDv = max(0, dot(normal, viewDirection));
	float nDl = max(0, dot(normal, lightDirection));
	float G = min(1, min(2 * ((nDh * nDv) / (dot(viewDirection, halfVector) + EPSILON)) , 2 * ((nDh * nDl) / (dot(viewDirection, halfVector) + EPSILON))));
	
	// Normal distribution function
	float top = pow(E, (nDh * nDh - 1) / ((roughness * roughness) * (nDh * nDh)));
	float bot = PI * (roughness * roughness) * pow(nDh, 4);
	float D = top / (bot + EPSILON);

	// Fresnel (Schlick approximation)
	vec3 F0 = (1-metalness) * vec3(0.04, 0.04, 0.04) + (metalness * diffuse);
	vec3 F = F0 + (1 - F0) * pow((1 - dot(halfVector, viewDirection)), 5);

	// Diffuse (Adjusted)
	vec3 lDiffuse = (diffuse / PI) * (vec3(1, 1, 1) - F) * (1 - metalness);

	// Specular term
	vec3 brdf = lDiffuse + ( ( D * F * G ) / ( 4 * nDv * nDl + EPSILON) ) ;

	// Rendering equation 
	// Out = Emissive + Ambient + BRDF * Light Colour * clampedDot(normal, light direction)
	vec3 pixelColour = ambient + (brdf * light.lightColour * nDl);

	// Output the colour
	outColour = vec4(pixelColour, 1.f);
}

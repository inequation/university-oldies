static const char TERRAIN_VS[] = STRINGIFY(
// properties constant over the entire terrain: x - heightmap size,
// y - height scale
uniform vec2 constParams;
// patch-specific properties: xy - uv bias, z - scale
uniform vec3 patchParams;
varying float fogFactor;
varying float height;

void main() {
	// calculate texture coordinates - offset and bias
	gl_TexCoord[0] = vec4(patchParams.z * gl_MultiTexCoord0.xy + patchParams.xy,
		gl_MultiTexCoord0.zw);

	// calculate vertex positions
	mat4 mvmat = mat4(
		// column 1
		patchParams.z * constParams.x,
		0.0,
		0.0,
		0.0,
		// column 2
		0.0,
		constParams.y,
		0.0,
		0.0,
		// column 3
		0.0,
		0.0,
		patchParams.z * constParams.x,
		0.0,
		// column 4
		(patchParams.x - 0.5) * constParams.x,
		0.0,
		(patchParams.y - 0.5) * constParams.x,
		1.0
	);
	gl_Position = gl_ModelViewProjectionMatrix * mvmat * gl_Vertex;

	// fog stuff
	vec3 vVertex = vec3(gl_ModelViewMatrix * mvmat * gl_Vertex);
	const float LOG2 = 1.442695;
	gl_FogFragCoord = length(vVertex);
	fogFactor = exp2(-gl_Fog.density * gl_Fog.density
		* gl_FogFragCoord * gl_FogFragCoord * LOG2);
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	// height - for noise calculations
	height = gl_Vertex.y;
}
);

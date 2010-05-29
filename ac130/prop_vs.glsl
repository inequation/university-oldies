static const char PROP_VS[] = STRINGIFY(
void main() {
	gl_TexCoord[0] = gl_MultiTexCoord0;
	// gl_MultiTexCoord1 holds the prop coordinates (xyz)
	// gl_MultiTexCoord2 holds the scales along the axes (xyz) and the angle (w)
	float c = cos(gl_MultiTexCoord2.w);
	float s = sin(gl_MultiTexCoord2.w);
	mat4 instance = mat4(
		// column 1
		gl_MultiTexCoord2.x * c,
		0.0,
		gl_MultiTexCoord2.z * -s,
		0.0,
		// column 2
		0.0,
		gl_MultiTexCoord2.y;
		0.0,
		0.0,
		// column 3
		gl_MultiTexCoord2.x * s,
		0.0,
		gl_MultiTexCoord2.z * c,
		0.0
		// column 4
		gl_MultiTexCoord1.xyz,
		1.0
	);
	gl_Position = gl_ModelViewProjectionMatrix * instance * gl_Vertex;
}
);

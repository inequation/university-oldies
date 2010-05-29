static const char PROP_FS[] = STRINGIFY(
uniform sampler2D propTex;
varying float fogFactor;

void main() {
	vec4 clear = texture2D(propTex, gl_TexCoord[0].st);
	vec4 fogged = mix(gl_Fog.color, clear, fogFactor);
	gl_FragColor = mix(fogged, clear, smoothstep(0.7, 0.8, clear.r));
	//gl_FragColor = vec4(0.0, 0.0, 0.0, fogged.a);
}
);

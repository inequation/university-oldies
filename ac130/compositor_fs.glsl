const char *COMPOSITOR_FS = STRINGIFY(
uniform sampler2D overlay;
uniform sampler2D frames[6];	// MUST be kept in sync with 1 + FRAME_TRACE!!!

vec4 get_view(vec2 st) {
	//return texture2D(frames[6], gl_TexCoord[0].st);
	return 0.705 * texture2D(frames[5], gl_TexCoord[0].st)
		+ 0.105 * texture2D(frames[4], gl_TexCoord[0].st)
		+ 0.085 * texture2D(frames[3], gl_TexCoord[0].st)
		+ 0.065 * texture2D(frames[2], gl_TexCoord[0].st)
		+ 0.045 * texture2D(frames[1], gl_TexCoord[0].st)
		+ 0.025 * texture2D(frames[0], gl_TexCoord[0].st);
}

void main() {
	vec4 ov = texture2D(overlay, gl_TexCoord[0].st);
	vec4 view = get_view(gl_TexCoord[0].st);
	gl_FragColor = vec4(ov.rgb * ov.a + view.rgb * (1.0 - ov.a), 1.0);
}
);

const char *COMPOSITOR_FS = STRINGIFY(
uniform sampler2D overlay;
uniform sampler2D frames[6];	// MUST be kept in sync with 1 + FRAME_TRACE!!!

vec4 get_view(vec2 st) {
	//return texture2D(frames[6], gl_TexCoord[0].st);
	return 0.705 * texture2D(frames[5], st)
		+ 0.105 * texture2D(frames[4], st)
		+ 0.085 * texture2D(frames[3], st)
		+ 0.065 * texture2D(frames[2], st)
		+ 0.045 * texture2D(frames[1], st)
		+ 0.025 * texture2D(frames[0], st);
}

const float pixOfs = 0.001;
vec4 get_overlay(vec2 st) {
	// left part of an order 4 Gaussian blur kernel
	return 0.0052 * (
		// row 1
		texture2D(overlay, st + vec2(-2.0 * pixOfs, -2.0 * pixOfs))
		+ 4.0 * texture2D(overlay, st + vec2(-pixOfs, -2.0 * pixOfs))
		+ 4.0 * texture2D(overlay, st + vec2(0.0, -2.0 * pixOfs))
		// row 2
		+ 4.0 * texture2D(overlay, st + vec2(-2.0 * pixOfs, -pixOfs))
		+ 16.0 * texture2D(overlay, st + vec2(-pixOfs, -pixOfs))
		+ 24.0 * texture2D(overlay, st + vec2(0.0, -pixOfs))
		// row 3
		+ 6.0 * texture2D(overlay, st + vec2(-2.0 * pixOfs, 0.0))
		+ 24.0 * texture2D(overlay, st + vec2(-pixOfs, 0.0))
		+ 36.0 * texture2D(overlay, st + vec2(0.0, 0.0))
		// row 4
		+ 4.0 * texture2D(overlay, st + vec2(-2.0 * pixOfs, pixOfs))
		+ 16.0 * texture2D(overlay, st + vec2(-pixOfs, pixOfs))
		+ 24.0 * texture2D(overlay, st + vec2(0.0, pixOfs))
		// row 5
		+ texture2D(overlay, st + vec2(-2.0 * pixOfs, 2.0 * pixOfs))
		+ 4.0 * texture2D(overlay, st + vec2(-pixOfs, 2.0 * pixOfs))
		+ 4.0 * texture2D(overlay, st + vec2(0.0, 2.0 * pixOfs))
		);
}

void main() {
	vec4 ov = get_overlay(gl_TexCoord[0].st);
	vec4 view = get_view(gl_TexCoord[0].st);
	gl_FragColor = vec4(ov.rgb * ov.a + view.rgb * (1.0 - ov.a), 1.0);
}
);

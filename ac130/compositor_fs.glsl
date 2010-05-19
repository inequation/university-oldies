const char *COMPOSITOR_FS = STRINGIFY(
uniform sampler2D overlay;
uniform sampler2D frames[6];	// MUST be kept in sync with 1 + FRAME_TRACE!!!
uniform float negative;

vec4 get_view(vec2 st) {
	vec4 p = 0.705 * texture2D(frames[5], st)
		+ 0.105 * texture2D(frames[4], st)
		+ 0.085 * texture2D(frames[3], st)
		+ 0.065 * texture2D(frames[2], st)
		+ 0.045 * texture2D(frames[1], st)
		+ 0.025 * texture2D(frames[0], st);
	vec4 n = vec4(1.0) - p;
	return (1.0 - negative) * p + negative * n;
}

const float blurSize = 1.0 / 1024.0;
vec4 get_overlay(vec2 st) {
	vec4 sum = vec4(0.0);
	sum += texture2D(overlay, vec2(st.x - 8.0 * blurSize, st.y)) * 0.022;
	sum += texture2D(overlay, vec2(st.x - 7.0 * blurSize, st.y)) * 0.044;
	sum += texture2D(overlay, vec2(st.x - 6.0 * blurSize, st.y)) * 0.066;
	sum += texture2D(overlay, vec2(st.x - 5.0 * blurSize, st.y)) * 0.088;
	sum += texture2D(overlay, vec2(st.x - 4.0 * blurSize, st.y)) * 0.111;
	sum += texture2D(overlay, vec2(st.x - 3.0 * blurSize, st.y)) * 0.133;
	sum += texture2D(overlay, vec2(st.x - 2.0 * blurSize, st.y)) * 0.155;
	sum += texture2D(overlay, vec2(st.x - blurSize, st.y)) * 0.177;
	sum += texture2D(overlay, vec2(st.x, st.y)) * 0.2;
	return sum;
}

void main() {
	vec4 ov = get_overlay(gl_TexCoord[0].st);
	vec4 view = get_view(gl_TexCoord[0].st);
	gl_FragColor = vec4(ov.rgb * ov.a + view.rgb * (1.0 - ov.a), 1.0);
}
);

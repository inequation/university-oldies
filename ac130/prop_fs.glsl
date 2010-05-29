static const char PROP_FS[] = STRINGIFY(
uniform sampler2D propTex;
void main() {
	gl_FragColor = texture2D(propTex, gl_TexCoord[0].st);
}
);

uniform sampler2D texture;

void main() {
	float a=texture2D(texture,gl_TexCoord[0].xy).a;
	gl_FragColor=gl_Color*vec4(1.0f,1.0f,1.0f,a);
}


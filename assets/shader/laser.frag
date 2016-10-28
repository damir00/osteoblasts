uniform sampler2D texture;
uniform float time;
uniform float y_scale;

void main() {
	vec4 color=texture2D(texture,vec2(gl_TexCoord[0].x,gl_TexCoord[0].y*y_scale-time*2.0));
	gl_FragColor=gl_Color*color;
}


uniform sampler2D texture;
uniform vec4 color;
uniform vec4 color_add;

void main() {
	gl_FragColor=texture2D(texture,gl_TexCoord[0].xy)*color+vec4(color_add.rgb*color.a,0.0);
}


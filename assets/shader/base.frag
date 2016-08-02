uniform vec4 color;
uniform vec4 color_add;

void main() {
	gl_FragColor=color+vec4(color_add.rgb*color.a,0.0);
}


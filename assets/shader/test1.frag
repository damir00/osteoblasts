uniform sampler2D texture;

void main() {
	//gl_FragColor=texture2D(texture,gl_TexCoord[0].xy);
	//return;
	
	vec4 color=texture2D(texture,gl_TexCoord[0].xy);
	//vec4 color2=vec4(1,0,0,1);
	vec4 color2=vec4(1.0f-color.rgb,color.a);

	//gl_FragColor=mix(color,color2,gl_TexCoord[0].x);
	gl_FragColor=color2;
}


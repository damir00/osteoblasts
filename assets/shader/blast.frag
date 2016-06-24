uniform sampler2D texture;
uniform float anim;

void main() {
	float dist=distance(anim*0.8f+0.2f,distance(vec2(0.5f),gl_TexCoord[0].xy));
	dist=1.0f-min(dist*10.0f,1.0f);

	vec2 offset=normalize(vec2(0.5f)-gl_TexCoord[0].xy)*(dist)*0.04f;

	vec4 tex=texture2D(texture,gl_TexCoord[0].xy+offset);
	gl_FragColor=tex+vec4(dist)*0.1f;
	gl_FragColor.a=1.0f;
}


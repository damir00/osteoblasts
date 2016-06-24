uniform sampler2D texture;
uniform float anim;

void main() {

	float w=0.05;
	float dist=distance(anim,distance(vec2(0.5f),gl_TexCoord[0].xy));

	vec2 offset=normalize(vec2(0.5f)-gl_TexCoord[0].xy)*dist;

	vec4 tex=texture2D(texture,gl_TexCoord[0].xy+offset);
	gl_FragColor=tex+vec4(dist);
	gl_FragColor.a=1.0f;
}


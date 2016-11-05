uniform sampler2D texture;
uniform vec2 resolution;
uniform float size;

void main() {

	vec2 position=vec2(0.5);
	float dist=distance(size,distance(position*resolution,gl_TexCoord[0].xy*resolution));

	dist/=resolution.x;

	dist=1.0-min(dist*10.0,1.0);

	vec2 offset=normalize(position*resolution-gl_TexCoord[0].xy*resolution)*dist*1.0;

	offset/=30.0;

	vec4 tex=texture2D(texture,gl_TexCoord[0].xy+offset);
	gl_FragColor=tex+vec4(dist)*0.1;
	gl_FragColor.a=1.0;
}


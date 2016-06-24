uniform sampler2D texture;
uniform float time;
uniform float amount;
uniform float freq;	//=30

void main() {

	vec2 offset=vec2(amount,0)*sin((gl_TexCoord[0].y+time)*freq);

	vec2 tex_r=texture2D(texture,gl_TexCoord[0].xy).ra;
	vec2 tex_g=texture2D(texture,gl_TexCoord[0].xy+offset).ga;
	vec2 tex_b=texture2D(texture,gl_TexCoord[0].xy-offset).ba;

	gl_FragColor=gl_Color*vec4(tex_r.x*tex_r.y,tex_g.x*tex_g.y,tex_b.x*tex_b.y, (tex_r.y+tex_g.y+tex_b.y)/3.0f );
}


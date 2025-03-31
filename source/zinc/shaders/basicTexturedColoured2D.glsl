varying vec2 fTextureCoords;
varying vec4 fColour;

#ifdef VERTEX
uniform vec2 gScreenSize;

attribute vec3 inPosition;
attribute vec2 inTextureCoords;
attribute vec4 inColour;

void main() {
	vec2 pointPos = (2.0 * (inPosition.xy / gScreenSize)) - vec2(1.0, 1.0);
	gl_Position = vec4(pointPos, 1.0, 1.0);
	fTextureCoords = inTextureCoords;
	fColour = inColour;
}
#endif

#ifdef FRAGMENT
uniform sampler2D gTexture;

void main() {
	gl_FragColor = fColour * texture2D(gTexture, fTextureCoords);
}
#endif

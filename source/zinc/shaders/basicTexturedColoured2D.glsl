varying vec2 fTextureCoords;
varying vec4 fColour;

#ifdef VERTEX
uniform vec2 gScreenSize;

attribute vec3 inPosition;
attribute vec2 inTextureCoords;
attribute vec4 inColour;

void main() {
	gl_Position = vec4(inPosition / (0.5 * vec3(gScreenSize, 0.0)), 1.0);
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

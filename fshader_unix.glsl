#version 130

in  vec4 color;
out vec4 f_color;

void main() {
	if(color.w == 0.0) {
		discard;
	}
    f_color = color;
}


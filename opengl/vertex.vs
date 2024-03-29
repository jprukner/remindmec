#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 rotation;
uniform mat4 translation;
uniform mat4 projection;

void main()
{
	gl_Position = projection * translation * rotation * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
	TexCoord = aTexCoord;
}

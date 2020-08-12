attribute highp vec4 vertCoord;
attribute lowp vec4 vertColor;
varying lowp vec4 color;

void main()
{
    gl_Position = vertCoord;
    color = vertColor;
}

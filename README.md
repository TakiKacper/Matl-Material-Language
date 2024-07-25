# MATL - Material Language  
### What is MATL
Matl is a simple language for programming materials, such as those in unreal engine, blender or unity - it allows for abstraction between shader code and user's logic. Using it's parser, matl can be easily translated into proper shaders 
(this repo only provides support for translation into opengl's glsl, but it is very simple to implement translators for other languages). Matl also provides tools to avoid code repetition - libraries for materials, and simple preprocessor for shader code.

### Syntax
Matl uses very simple, rust and python based syntax making it very easy to understand:
```javascript
using domain my_domain

func calc_color()
  return (1, 1, 1, 1)

let offset = (0, 0)

property color = calc_color()
property vertex_offset = offset
```
### Domains - shaders templates
As you can see example above use some "domain". Domain is a template-shader which, when combined with material code creates a functional shader. Domain defines properties - those are fields where user can provide their own value, eg. color, metallic, specularity etc.
By providing values / equations for each of the properties, we can create multiple shaders, while writing things like lighting code only once!

<details>
  <summary>Example GLSL domain:</summary>

```glsl
<expose>
    <property   vector4    color>
    <property   vector2    vertex_offset>
<end>

#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

<dump parameters>

<dump functions>
    <property vertex_offset>
<end>

void main()
{  
    <dump variables>
        <property vertex_offset>
    <end>

    gl_Position = vec4(aPos + <property vertex_offset>, 0, 1);
    TexCoord = aTexCoord;
}

<split>

#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

<dump parameters>

<dump functions>
    <property color>
<end>

void main()
{
    <dump variables>
        <property color>
    <end>

    FragColor = <property color>;
}
```
</details>  

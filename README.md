# MATL - Material Language  
Matl is a simple language for programming materials, such as those in unreal engine, blender or unity - it allows for abstraction between shader code and user's logic. Using it parser, matl can be easily translated into proper shaders (this repo for now only support translation into opengl's glsl, but it is very simple to implement translators for other languages).

Matl uses very simple, rust and python based syntax making it very easy to understand:
```javascript
using domain my_domain

func calc_color()
  return (1, 1, 1, 1)

let offset = (0, 0)

property color = calc_color()
property vertex_offset = offset
```
As you can see example above use some "domain". Domain is a template-shader which, when combined with material code creates a functional shader. Domain defines properties - those are fields where user can provide their own value, eg. color, metallic, specularity etc.
It can also define symbols (those are things that are declared in domain's code and which materials can use in their calculations eg. world position, texture) and functions.
Material to be valid must provide an value for every of domain's properties.

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
  
Matl also provides tools to avoid code repetition - substitute of #include in form of <dump insertion (...)> for domains code and libraries system for materials.
For more info look at matl documentation:
[TODO]

# Domain programming

## Creating Domain
First thing is create a base shader in the expected language. Lets look at this example glsl shader:  

```glsl
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0, 1);
    TexCoord = aTexCoord;
}
```
```glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D inTexture;

void main()
{
    FragColor = sample(inTexture, TexCoord);
}
```
This shaders will put vertices at the given positions and apply the ``inTexture`` on it.  
In this little tutorial we will parameterize this shader so the user can specify the vertex offset and color from the materials, like this:
```js
using domain my_domain

property color = sample(my_domain.texture, my_domain.texture_coords)
property vertex_offset = (0, 0)
```

## Adding Properties
Domain must be one source, so we will put both vertex and fragment shader in the same file. 
There is an option to put a breakpoint between sources in the domain but more on that later.  
  
To create properties we need to use ``<expose>`` block ended with ``<end>``:
```glsl
<expose>

<end>

#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

[...]
```
Once we have the expose block we can declare properties (parameters to be set in material):
```glsl
<expose>
  <property   vector4    color>
  <property   vector2    vertex_offset>
<end>

[...]
```
The indentation here is not necessary.
Code above creates two properties:   
```
property color of type vector4 (vec4 in glsl)  
property vertex_offset of type vector2 (vec2 in glsl)  
```
We can now use ``<property [name]>`` to put their values in appropriate places:
```glsl
void main()
{
    gl_Position = vec4(aPos + <property vertex_offset>, 0, 1);

  [...]
```
```glsl
void main()
{
    FragColor = <property color>
```
Now, when material uses our domain it must provide value or equation for each of these properties.

## Adding dump spots
Matl besides properties also provides variables and functions. We need to put appropriate directives so the translator know where to put their definitions.

```glsl
<dump functions>
    <property vertex_offset>
<end>

void main()
{  
    <dump variables>
        <property vertex_offset>
    <end>

    gl_Position = vec4(aPos + <property vertex_offset>, 0, 1);

[...]
```
(Do this also for color)  

We used to blocks: ``<dump functions>`` and ``<dump variables>``. In their places the translator will put the definitions of functions and variables.
Inside those blocks is a place for properties list; only functions and variables that are used by listed properties will be dumped:
```python
func useless_func()
    return 12         #not used, will not be dumped

func pi_func()
    return 3.14       #used, will be dumped

let pi = pi_func()    #used, will be dumped
property vertex_offset = (pi, pi)
```
You should always provide <dump ...> so they cover all properties - otherwise, you may end with a shader throwing unresolved symbol error on compilation.

## Dump Parameters
Parameters are additional uniforms/constants (name vary between languages and APIs) that are created from materials code.
```cpp
using property alpha = 0.5
```
Using ``<dump properites>`` we can mark a spot, when the translator should insert their definitions.  
In glsl it should be done between ``#version`` and ``void main()``:  
```
```glsl
<dump functions>
    <property vertex_offset>
<end>

<dump properties>

void main()
{  
    <dump variables>
        <property vertex_offset>
    <end>

    gl_Position = vec4(aPos + <property vertex_offset>, 0, 1);

[...]
```
Note that matl functions are transparent (they cannot use properties and domain's symbols) so it is fine to place ``<dump properties>`` after ``<dump functions>``.

## Spliting Shader
In opengl it is required to compile vertex shader, fragment shader and optionaly geometry shader in separate compile calls. That implies that the shader source must be splited into 2 or 3 respectively. 
We can achieve that using the ``<split>`` directive. When translator come across it, it ends writing to provious source, and creates another for the rest of the shader.
```glsl
    TexCoord = aTexCoord;
}    
//End of vertex shader

<split>

//Next source begin

#version 330 core    
out vec4 FragColor;
```
Now, matl api function ``matl::parse_material`` will return ``matl::parsed_material`` with not one, but two sources when parsing material using this domain.  
``<split>`` can be of course used as many times as you want.




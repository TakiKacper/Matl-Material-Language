# Domain programming

## Creating simple domain
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

## You now have a fully functional domain
This domain is complete, but there more things you can add.

## Exposing Symbols
Symbols are variables / constant that are, opposite to properties, passed from domain to material.
```glsl
<expose>
    <symbol     vector2    world_position = aPos>
<end>
```
The code above create symbol ``world_position`` of type ``vector2`` that will be translated into ``aPos``.  
Now it can be used inside the material, like this:
```js
let scaled = domain.world_position * 3.14.
```


Note that symbols are only preprocessor tool; what's written on left of the ``=`` is not checked.  
It is domain programmer resposibility to ensure that the symbols definitions are valid at the time of use i.e. ``<dump property x>`` and ``<dump variables> [...]``.  
  
Of course not all variables available for a shader are available for the next one, eg. variable created in vertex shader may not exist with the same name in fragment (pixel) shader.
The solutions is to use ``<redef>`` block:
```glsl
<redef>
    <symbol     vector2    world_position = aPos2>
<end>
```
When translator come across this directive it will switch ``world_position`` definition from ``aPos`` to ``aPos2``.
Note that ``<redef>`` cannot change type of existing symbols, and cannot create new ones.

## Exposing Functions
Exposing functions is similar to exposing symbols:
```glsl
<expose>
    <function   xyz = scalar lerp(scalar, scalar, scalar)>
<end>
```
The code above created function ``xyz`` that is than translated into the ``lerp`` function, which is builtin glsl function (of course, using user-definied functions also work, but user must ensure that those functions exist at the time of use). The ``xyz`` function is now 
Now ``xyz`` can be used in material like that:
```js
let c = domain.xyz(1, 2, 0.5)
```
Exposed functions, unlike matl functions, are not templates and cannot be used for other sets of types that those provided. Therefore it is allowed to provide multiple overloads for the same function:
```glsl
<expose>
    <function   xyz = scalar lerp(scalar, scalar, scalar)>
    <function   xyz = vector2 lerp(vector2, vector2, vector2)>
<end>
```
Now ``xyz`` can be used both for 3x scalar arguments and for 3x vector2 arguments. Also the return types are diffrent.  
The thing to note is that each overload must take the same amount of arguments.   

Functions unlike symbols cannot by redefinied with ``<redef>``.











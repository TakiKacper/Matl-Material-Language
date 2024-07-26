# Programming in Matl
## Contents
- [Boring Reference](#Boring-Reference)
    - [File types](#File-types)
    - [Data types](#Data-types)
    - [Scopes](#Scopes)
    - [Keywords](#Keywords)
    - [Expressions](#Expressions)
- [Explaining things](#Explaining-things)
    - [Using domain](#Using-domain)
    - [Properties](#Properties)
    - [Variables](#Variables)
    - [Parameters](#Parameters)
    - [Comments](#Comments)
    - [Functions](#Functions)
    - [Libraries](#Libraries)
- [Writing Materials](#Writing-Materials)
- [Writing Libraries](#Writing-Libraries)

## Boring Reference
### File types
In matl there are two types of files:  
```yaml
Materials   :  provides values for properties of some domain and is translated into a fully functional shader code
Libraries   :  can only be used to store functions
```

### Data Types
There are 6 data types in matl:
```yaml
bool      :   logical value, true or false
scalar    :   mathematical scalar, eg. 3.14, 2, 1024.64
vector2   :   mathematical vector with 2 dimensions
vector3   :   mathematical vector with 3 dimensions
vector4   :   mathematical vector with 4 dimensions
texture   :   2 dimensional texture
```
Metal is strongly typed and does not provide any implicit or explicit types conversions.

### Scopes
```python
using domain my_domain            # Material scope

func lerp(y0, y1, alpha)          # Material scope
    let delta = (y1 - y0)         # Function scope
    return y0 + delta * alpha     # Function scope

property color = (1, 1, 1, 1)     # Material scope
property vertex_offset = (0, 0)   # Material scope
```
There 3 scopes in matl:  
```yaml
material    :  global in materials (no indentation)
library     :  global in libraries (no indentation)
function    :  between func keyword and return keyword including return (requires consistent indentation)
```  
Depending on where the code is located other keywords are available.

### Keywords
Matl has only 5 keywords
```yaml
let [variable name] = [expression]                        : creates variable                                     (available in material and functions scopes)
property [property name] = [expression]                   : specify math equation for given property             (available in material scope)
func [function name] ( [arg1 name] , [arg2 name] , ... )  : creates function, opens the function scope           (available in material and library scopes)
return [expression]                                       : returns value from function, end the function scope  (available in function scope)
using [case] [arguments]                                  : do case dependant things                             (available in material and library scopes)
```
using keyword have 3 builtin cases:
```yaml
using domain [name]                         : loads the domain - since this line material can use property keyword (available in materials)
using library [name]                        : links library so it can be used in this material/library             (available in materials and libraries)
using parameter [name] = [default value]    : creates parameter                                                    (available in materials)
```
(the game engine/library You are using can provide extra ones - see its docs)

### Expressions
For mathematical expressions matl uses syntax similar to almost every other programming lanugage:
```js
let a = 2 * lerp(1, 2, 0.5)
```

**Arythmetic operators** 
  
In matl there are 5 arythmetic operators: 
```yaml
- : negation        (allowed:    -scalar, -vectorN)
+ : addition        (allowed:    scalar + scalar, vectorN + vectorN, vectorN + scalar, commutative)
- : subtraction     (allowed:    scalar - scalar, vectorN - vectorN, vectorN - scalar)
* : multiplication  (allowed:    scalar * scalar, vectorN * vectorN, vectorN * scalar, commutative)
/ : division        (allowed:    scalar / scalar, vectorN / vectorN, vectorN / scalar)
```
In all cases the result is the "bigger" type eg. scalar * vector3 returns in vector3.
Note you cannot multiply vectors of diffrent sizes.

**Logical operators**  
  
There are also 10 logical operators:
```yaml
not : negation                (allowed: bool)
and : coniunction             (allowed: bool and bool)
or  : alternative             (allowed: bool or bool)
xor : exclusive alternative   (allowed: bool xor bool)
==  : equality                (allowed: bool == bool, scalar == scalar)
!=  : not equaity             (allowed: bool != bool, scalar != scalar)
<   : less                    (allowed: scalar < scalar)
>   : greater                 (allowed: scalar > scalar)
<=  : less or equal           (allowed: scalar > scalar)
>=  : greater or equal        (allowed: scalar > scalar)
```
All of those return bool as result.

**Accessing vectors components**  
  
Programmer can access given vector components using following syntax:
```python
some_vec_4.g     # returns a scalar, the second vector component
some_vec_4.rgb   # return a vector3, the three first vector components
some_vec_4.brag  # returns a vector4 with the components of the oryginal vector in given order
```

Components names:
```yaml
x or r : first component
y or g : second component
z or b : third component
w or a : fourth component
```

**Constructing vectors**  
  
Vectors can be constructed using parentheses and comas:
```js
let v1 = (1, 2, 3, 4)

let a = 3
let v2 = (a, a, a)
```

**Functions**  
  
Functions can be called with following syntax:
```js
lerp(0.4, 1, 0.5)
library.lerp(0.4, 1, 0.5)
```
The return type will be deduced from the function equations.  

**Symbols**  
  
Domain's symbols can be accessed using following:
```js
domain.[symbol name]
```

**Parameters** 
  
Parameters can be accesed in expressions by their names:
```js
[parameter name]
```

**Conditions**  
  
Variables can be assigned diffrent values varying on conditions, using ``if`` and ``else``.
```js
let a = 3.14
let condition = a > 6.28

let b = if condition : a * 3
        if a == 3.14 : 13
        else           12
```
if using condtions, ``if`` must be the very first token after ``=``. Such code is not allowed:
```js
let b = 2 * (if condition : a * 3  # error
             if a == 3.14 : 13
             else           12)
```
You can have as many if's as you want, but note that there must always be an ``else`` case.  
Also note that all the cases must return the same data type. Such code is not allowed:
```js
let b = if condition : 2          # error
        else           (2, 2)     # error
```

## Explaining-things
### Using domain
The very first thing every material should do is to specify it's domain. Domain is, as mentioned in readme, a template for a shader. It contains all the stuff like lighting code, vertex projection.
When we write ``using domain opaque_color`` we are telling parser that it should create a shader using the ``opaque_color`` domain as a template.  
Domains are very likely to be predefinied by the game engine or any app You are using, so I recommend seeing it's docs.
In this guide, we will assume existence of a domain named ``opaque_color`` that exposes two properties: ``color`` and ``vertex_offset``.
```js
using domain opaque_color

property color = (1, 1, 1, 1)
property vertex_offset = (0, 0)
```
### Properties
But what are those properties?   
- Properties are something like domain's parameters - the ``opaque_color`` by itself does not know what color do You want your mesh to be, so it lets you specify in material - via properties.
```js
using domain opaque_color

property color = (1, 1, 1, 1)
property vertex_offset = (0, 0)
```
How do I know what properties does the domain exposes?  
- Again - see the engine docs. You can also try to save and use your material - matl parser will tell via errors what properties are Your material missing.  
  
Properties does have fixed types. You cannot assign ``1`` to opaque_color's property ``color``, because color is of type vector4.  
How do I know properties types?
- Engine docs or trial and error.

It is also worth noting that properties can be assigned with mathematical expressions and not only fixed values.
```
property color = (1, 1, 1, 1) * (0.1 / 0.2)
```

The material will parse successfully only if all the properties are assigned with a value/expression of a proper type.

### Variables
If Your material only use fixed values, then you don't really need variables.
But if want to do some math inside material, then variables may come handy.

Variables, despite their name are immutable.

```js
using domain opaque_color

let color_mod = 1 / 3.14      # valid, color_mod does not exist yet
let color_mod = 1             # error - override

property color = (1, 1, 1, 1) * color_mod
property vertex_offset = (0, 0)
```

They can be used to split the properties expressions so they don't grow long.

### Parameters
Having only hardcoded values in the material is pretty boring. Let's add some extern state.  
  
Properties are variables that are controlled from the engine / app. (If You had programed in glsl or hlsl before - those are uniforms/constants really).
In this little example we won't really explore the possiblities they give to us, but there are plenty of uses for them - imagine some sort of vignette, that gets darker the less the game's player health is 
- this is a perfect use case for parameters.

```cpp
using domain opaque_color
using parameter color_mod = 0.5

property color = (1, 1, 1, 1) * color_mod
property vertex_offset = (0, 0)
```

Parameters can be created using the ``using parameter`` contruction. The value we assign to it is a default value - in this case it is 0.5, so the ``color_mod`` is of type scalar.
You can also create vector parameters:
```python
# this one will be vector3
using parameter vec_param = (1, 1, 1)
```
Or even texture types
```python
# and this one will be texture
using parameter text_param = my_texture.png
```
Parameters' types cannot be changed. Their values of course can.

### Comments
As you saw in previous listenings matl does allow single-line comments with the ``#`` symbol.
Use it to explain the complex math of your materials!

### Functions
Matl functions are very similar to mathematical functions.




## Writing Materials
There is not many to cover in this topic, when the domains are unknown.  
See if your engine does provide an documentation about materials scripting.
Here is a list of requirements that material must met:
```yaml
1 : Domain must be included (using domain [...])
2 : All properties of the given domain must be specified with property keyword
3 : There must be no errors
```
Once the material met all the rules it can be translated into a proper shader and sent to the gpu.

## Writing Libraries
Just dump your functions into a file. See if your engine does have any special requirements














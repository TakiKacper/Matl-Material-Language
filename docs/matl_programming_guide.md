# Programming in Matl
## File types
In matl there are two types of files:  
1. Materials  
2. Libraries  
The first one does provides values for properties of some domain, and results in a fully functional shader code, while the later one can only be used to store functions.

## Scopes
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

## Keywords
Matl has only 5 keywords
```yaml
let [variable name] = [expression]                        : creates variable                                     (available in material and functions scopes)
property [property name] = [expression]                   : specify math equation for given property             (available in material scope)
func [function name] ( [arg1 name] , [arg2 name] , ... )  : creates function, opens the function scope           (available in material and library scope)
return [expression]                                       : returns value from function, end the function scope  (available in function scope)
using [case] [arguments]                                  : do magic                                             (available in material scope)
```

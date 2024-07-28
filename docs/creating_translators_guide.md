# Creating Translators
In order to create a translator, You need to create a global object of type ``translator``.  
Since to port a language like HLSL You only need to slighly change the provided glsl translator + create a way to handle parameters, I will not write a detailed guide on it.  
Look at ``matl_glsl.hpp`` for reference, I recommend also grabbing decent c++ ide, so You can search through matl classes members easily.  
  
After writing the translator remember to included it in the same file the matl is implemented.

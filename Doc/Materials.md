# Materials

Different objects having multiple vertex layouts do **not** result in seperate materials, instead producing seperate pipelines which take vertex buffers as inputs.

Different objects with different resources bound to shaders (using the same layout) do **not** result in seperate materials _OR_ in seperate pipelines.

### Proposed System Designs

Main focus is on _shaders_ and _binders_.  Pipelines can be created at load time based on shader metadata and the resources they access.

| Concept | Design |
|---------|--------|
| Shader  | Objects apply local material properties, `VkPipeline`s generated from object data + shader metadata.  Pipelines map objects to shaders / passes. |
| Binder  | Binds physical resources to shaders. |

Dependencies between pipelines and objects need to be known.

```
for each pass
	start pass
	for each view
		global uniforms
		for each pipeline
			bind pipeline
			update uniforms
			for each object
				update uniforms
				push constants
				record commands
	end pass
```

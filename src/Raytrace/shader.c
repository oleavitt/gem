/**
 *****************************************************************************
 * @file shader.c
 * Functions for managing shaders inthe renderer.
 *
 *****************************************************************************
 */

#include "ray.h"

/**
 * Initialize the shaders stuff.
 * Called by Ray_Initialize() in raytrace.c when the renderer is initialized.
 *
 * @return int - 1 to indicate success, or 0 for failure.
 */
int InitializeShader(void)
{
	return 1;
}


/**
 * Close down the shaders stuff.
 * Called by Ray_Close() in raytrace.c when the renderer is shut down.
 */
void CloseShader(void)
{
}


/**
 * Adds a shader to a shader list, starting a new list if necessary.
 *
 * @param existing_shader_list - Shader* - The list to add to.
 *   If NULL, the start of a new list will be returned.
 * @param vmshader - VMShader* - The VM shader to be added to the list.
 *
 * @return Shader* - Pointer to the head of the shader list, or NULL
 *   if initialization of list failed.
 */
// TODO shader: Argument list for parameterized declared shaders. VMArgument *arglist
Shader * Ray_AddShader(Shader *existing_shader_list, VMShader *vmshader)
{
	Shader *shader = (Shader *) Malloc(sizeof(Shader));

	if (shader != NULL)
	{
		shader->next = NULL;
		shader->vmshader = vmshader;


		if (existing_shader_list != NULL)
		{
			Shader *last = existing_shader_list;

			// Seek the tail of the list.
			while (last->next != NULL)
				last = last->next;

			// Append new shader as the tail.
			last->next = shader;

		}
		else // This is the head of a new list.
			existing_shader_list = shader;
	}

	return existing_shader_list;
}


/**
 * Deletes the Shader list elements.
 * The VMShaders pointed to are released.
 *
 * @param shader_list - Shader* - Pointer to Shader list to be deleted.
 */
void Ray_DeleteShaderList(Shader *shader_list)
{
	while (shader_list != NULL)
	{
		Shader *shader = shader_list;

		shader_list = shader_list->next;

		Ray_ReleaseVMShader(shader->vmshader);
		// TODO shader: vm_delete_arglist(shader->arglist);
		Free(shader, sizeof(Shader));
	}
}


/**
 * Registers a share of a VMShader.
 * Called whenever the same VMShader used in more than one place.
 *
 * @param vmshader - VMShader* - Pointer to VMShader to be shared.
 *
 * @return VMShader* - Pointer to the same VMShader instance.
 */
VMShader *Ray_ShareVMShader(VMShader *vmshader)
{
	if (vmshader != NULL)
		vmshader->nrefs++;

	return vmshader;
}


/**
 * Releases a share of a VMShader.
 * Called whenever we are deleting a reference to a VMShader.
 *
 * @param vmshader - VMShader* - Pointer to VMShader to be released.
 */
void Ray_ReleaseVMShader(VMShader *vmshader)
{
	if (vmshader != NULL)
	{
		if (--vmshader->nrefs == 0)
		{
			vm_delete( (VMStmt *) vmshader);
		}
	}
}


/**
 * Creates copy of a Shader list from an existing list.
 *
 * @param srcshaderlist - Shader* - Pointer to list to be copied.
 *
 * @return Surface* - Pointer to a new Shader list, or NULL if alloc failed.
 */
Shader *Ray_CloneShaderList(Shader *srcshaderlist)
{
	Shader *src, *dest, *last, *head;

	head = last = NULL;

	if (srcshaderlist != NULL)
	{
		for (src = srcshaderlist; src != NULL; src = src->next)
		{
			dest = (Shader *) Malloc(sizeof(Shader));
			if (dest != NULL)
			{
				dest->next = NULL;
				
				// The VMShaders pointed to in the list are shared.
				// Bump up the share count on them.
				//
				dest->vmshader = Ray_ShareVMShader(src->vmshader);
// TODO shader: Deep copy argument list for parameterized declared shaders.
				if (last != NULL)
				{
					last->next = dest;
					last = dest;
				}
				else
					head = last = dest;
			}
		}
	}

	return head;
}

/**
 * Runs the VMShader
 *
 * @param shader - Shader* - Shader list element containing current shader to run.
 * @param data - Generic pointer to renderer object that shader works on.
 */
void Ray_RunShader(Shader *shader, void *data)
{
	// TODO shader: shader->vmshader->tmp_arglist = shader->arglist;
	shader->vmshader->tmp_data = data;
	shader->vmshader->vmstmt.methods->fn( (VMStmt *) shader->vmshader);
}

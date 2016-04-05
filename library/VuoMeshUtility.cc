/**
 * @file
 * VuoMeshUtility implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include "VuoMeshUtility.h"
#include "module.h"
#include <vector>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "VuoMeshUtility",
					"dependencies" : [
						"VuoList_VuoPoint4d",
						"VuoList_VuoInteger"
					 ]
				 });
#endif
}

/// Constant providing the ratio of a circle's circumference to its diameter
#define PI 3.14159265359

/**
 * Returns component-wise min, ignoring W.
 */
static inline VuoPoint4d VuoPoint4d_min(const VuoPoint4d lhs, const VuoPoint4d rhs)
{
	return VuoPoint4d_make(
		fmin(lhs.x, rhs.x),
		fmin(lhs.y, rhs.y),
		fmin(lhs.z, rhs.z),
		1.
		);
}

/**
 * Returns component-wise max, ignoring W.
 */
static inline VuoPoint4d VuoPoint4d_max(const VuoPoint4d lhs, const VuoPoint4d rhs)
{
	return VuoPoint4d_make(
		fmax(lhs.x, rhs.x),
		fmax(lhs.y, rhs.y),
		fmax(lhs.z, rhs.z),
		1.
		);
}

/**
 * Calculates the normal for each vertex in this submesh.  Vertices pointed at by multiple 
 * element indices will be averaged.
 */
void VuoMeshUtility_calculateNormals(VuoSubmesh* submesh)
{
	if( submesh->elementAssemblyMethod != VuoMesh_IndividualTriangles )
	{
		VLog("VuoMeshUtility_calculateNormals() requires element assembly method VuoMesh_IndividualTriangles.");
		return;
	}

	unsigned int elementCount = submesh->elementCount;
	unsigned int vertexCount = submesh->vertexCount;
	const unsigned int* elements = submesh->elements;
	const VuoPoint4d* vertices = submesh->positions;

	VuoPoint4d* normals = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	unsigned int* normalsAverage = (unsigned int*)malloc(sizeof(unsigned int) * vertexCount);

	// initialize normals to {0,0,0,0}
	for(unsigned int i = 0; i < vertexCount; i++)
	{
		normals[i] = VuoPoint4d_make(0,0,0,0);
		normalsAverage[i] = 0;
	}

	// sum up all normals
	for(unsigned int i = 0; i < elementCount; i+=3)
	{
		unsigned int i0 = elements[i+0];
		unsigned int i1 = elements[i+1];
		unsigned int i2 = elements[i+2];

		VuoPoint4d a = vertices[i0];
		VuoPoint4d b = vertices[i1];
		VuoPoint4d c = vertices[i2];

		VuoPoint4d cross = VuoPoint4d_normalize3d(VuoPoint4d_crossProduct(VuoPoint4d_subtract(b,a), VuoPoint4d_subtract(c,a)));

		normals[i0] = VuoPoint4d_add(normals[i0], cross);
		normals[i1] = VuoPoint4d_add(normals[i1], cross);
		normals[i2] = VuoPoint4d_add(normals[i2], cross);

		normalsAverage[i0]++;
		normalsAverage[i1]++;
		normalsAverage[i2]++;
	}

	// now go through and average
	for(unsigned int i = 0; i < vertexCount; i++)
	{
		normals[i] = VuoPoint4d_multiply(normals[i], 1./(float)normalsAverage[i]);
	}

	free(normalsAverage);

	if(submesh->normals != NULL) free(submesh->normals);
	submesh->normals = normals;
}

/**
 * Calculates tangents and bitangents for a mesh given vertices, textures, normals, and triangles.
 * Assumes triangles are wound using VuoMesh_IndividualTriangles, and that the positions, textures, normals,
 * and triangles arrays are valid.
 * Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”. Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
 */
void VuoMeshUtility_calculateTangents(VuoSubmesh* submesh)
{
	unsigned int vertexCount = submesh->vertexCount;
	const VuoPoint4d *vertex = submesh->positions;
	const VuoPoint4d *normal = submesh->normals;
	const VuoPoint4d *texcoord = submesh->textureCoordinates;
	unsigned int triangleCount = submesh->elementCount;
	const unsigned int *triangle = submesh->elements;

	if( submesh->elementAssemblyMethod != VuoMesh_IndividualTriangles )
	{
		VLog("VuoMeshUtility_calculateNormals() requires element assembly method VuoMesh_IndividualTriangles.");
		return;
	}

	if(submesh->tangents != NULL) free(submesh->tangents);
	if(submesh->bitangents != NULL) free(submesh->bitangents);

	VuoPoint4d *tangent = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);
	VuoPoint4d *bitangent = (VuoPoint4d*)malloc(sizeof(VuoPoint4d) * vertexCount);

	int tan1count = vertexCount * 2;
	VuoPoint4d *tan1 = new VuoPoint4d[tan1count];
	VuoPoint4d *tan2 = tan1 + vertexCount;

	for( int i = 0; i < tan1count; i++)
		tan1[i] = (VuoPoint4d) {0,0,0,0};

	for (int a = 0; a < triangleCount; a+=3)
	{
		long i1 = triangle[a+0];
		long i2 = triangle[a+1];
		long i3 = triangle[a+2];

		if(i1 > vertexCount || i2 > vertexCount || i3 > vertexCount)
		{
			VLog("Skipping tangent generation for triangle: (%lu, %lu, %lu) because it references out of bounds vertices.", i1, i2, i3);
			continue;
		}

		const VuoPoint4d &v1 = vertex[i1];
		const VuoPoint4d &v2 = vertex[i2];
		const VuoPoint4d &v3 = vertex[i3];

		const VuoPoint4d &w1 = texcoord[i1];
		const VuoPoint4d &w2 = texcoord[i2];
		const VuoPoint4d &w3 = texcoord[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.0 / (s1 * t2 - s2 * t1);
		VuoPoint4d sdir = (VuoPoint4d)
		{
			(t2 * x1 - t1 * x2) * r,
			(t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r,
			1.
		};

		VuoPoint4d tdir = (VuoPoint4d)
		{
			(s1 * x2 - s2 * x1) * r,
			(s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r,
			1.
		};

		tan1[i1] = VuoPoint4d_add(tan1[i1], sdir);
		tan1[i2] = VuoPoint4d_add(tan1[i2], sdir);
		tan1[i3] = VuoPoint4d_add(tan1[i3], sdir);

		tan2[i1] = VuoPoint4d_add(tan2[i1], tdir);
		tan2[i2] = VuoPoint4d_add(tan2[i2], tdir);
		tan2[i3] = VuoPoint4d_add(tan2[i3], tdir);
	}

	for (long a = 0; a < vertexCount; a++)
	{
		VuoPoint3d n = (VuoPoint3d){ normal[a].x, normal[a].y, normal[a].z };
		VuoPoint3d t = (VuoPoint3d){ tan1[a].x, tan1[a].y, tan1[a].z };
		VuoPoint3d t2 = (VuoPoint3d){ tan2[a].x, tan2[a].y, tan2[a].z };

		// Gram-Schmidt orthogonalize
		VuoPoint3d tan = VuoPoint3d_normalize(VuoPoint3d_subtract(t, VuoPoint3d_multiply(n, VuoPoint3d_dotProduct(n, t))));
		VuoPoint3d bitan = VuoPoint3d_normalize(VuoPoint3d_subtract(t2, VuoPoint3d_multiply(n, VuoPoint3d_dotProduct(n, t2))));

		tangent[a] = (VuoPoint4d){tan.x, tan.y, tan.z, 0.};
		bitangent[a] = (VuoPoint4d){ bitan.x, bitan.y, bitan.z, 0. };

		// Calculate handedness
		tangent[a].w = (VuoPoint3d_dotProduct(VuoPoint3d_crossProduct(n, t), t2) < 0.0F) ? -1.0F : 1.0F;
		bitangent[a].w = (VuoPoint3d_dotProduct(VuoPoint3d_crossProduct(n, t2), t) < 0.0F) ? -1.0F : 1.0F;
	}

	submesh->tangents = tangent;
	submesh->bitangents = bitangent;

	delete[] tan1;
}

/**
 * Get the bounds of a submesh.
 */
bool VuoMeshUtility_bounds(const VuoSubmesh mesh, VuoPoint4d* min, VuoPoint4d* max)
{
	const VuoPoint4d* vertices = mesh.positions;

	if(mesh.vertexCount < 1)
		return false;

	// Calculate the center of the mesh.
	*min = mesh.positions[0];
	*max = mesh.positions[0];

	for(int i = 1; i < mesh.vertexCount; i++)
	{
		*min = VuoPoint4d_min(*min, vertices[i]);
		*max = VuoPoint4d_max(*max, vertices[i]);
	}

	return true;
}

/**
 * Generate UVs by calculating each vertex position as projected onto a bounding sphere.
 * http://en.wikipedia.org/wiki/UV_mapping
 */
void VuoMeshUtility_calculateSphericalUVs(VuoSubmesh* submesh)
{
	unsigned int dup_vertex_start = submesh->vertexCount;

	if( submesh->elementAssemblyMethod == VuoMesh_IndividualTriangles )
		VuoMeshUtility_insertSeam(submesh);

	const VuoPoint4d* vertices = submesh->positions;
	VuoPoint4d* textures = submesh->textureCoordinates;

	VuoPoint4d min, max;
	VuoMeshUtility_bounds(*submesh, &min, &max);
	VuoPoint4d center = VuoPoint4d_multiply(VuoPoint4d_add(max, min), .5);

	for(int i = 0; i < submesh->vertexCount; i++)
	{
		VuoPoint4d v = VuoPoint4d_normalize(VuoPoint4d_subtract(vertices[i], center));

		textures[i] = (VuoPoint4d)
		{
			.5 + (atan2(v.z, v.x) / (2 * PI)),
			.5 - (asin(v.y) / PI),
			0.,
			1.
		};

		if(i >= dup_vertex_start)
			textures[i].x += 1.;
	}

	// remove vertices *after* generating textures because
	// textures uses the overflowing vertices to determine
	// which coordinates need to be wrapped.
	if( submesh->elementAssemblyMethod == VuoMesh_IndividualTriangles && dup_vertex_start < submesh->vertexCount )
		VuoMeshUtility_removeUnusedVertices(submesh);
}

/**
 * Dot product of XYZ values in 4d points.
 */
static float dot(const VuoPoint4d lhs, const VuoPoint4d rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Inserts a seam on a submesh.  May (probably will) leave unused vertices.
 */
void VuoMeshUtility_insertSeam(VuoSubmesh* submesh)
{
	const VuoPoint4d* positions = submesh->positions;
	const VuoPoint4d* normals = submesh->normals;
	const VuoPoint4d* tangents = submesh->tangents;
	const VuoPoint4d* bitangents = submesh->bitangents;
	const VuoPoint4d* textures = submesh->textureCoordinates;

	const unsigned int* indices = submesh->elements;
	unsigned int* seam_indices = (unsigned int*)malloc(sizeof(unsigned int) * submesh->elementCount);
	memcpy(seam_indices, indices, sizeof(unsigned int) * submesh->elementCount);

	VuoList_VuoPoint4d seam_positions = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d seam_normals = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d seam_tangents = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d seam_bitangents = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d seam_textures = VuoListCreate_VuoPoint4d();

	for(unsigned int i = 0; i < submesh->vertexCount; i++)
	{
		if(positions) VuoListAppendValue_VuoPoint4d(seam_positions, positions[i]);
		if(normals) VuoListAppendValue_VuoPoint4d(seam_normals, normals[i]);
		if(tangents) VuoListAppendValue_VuoPoint4d(seam_tangents, tangents[i]);
		if(bitangents) VuoListAppendValue_VuoPoint4d(seam_bitangents, bitangents[i]);
		if(textures) VuoListAppendValue_VuoPoint4d(seam_textures, textures[i]);
	}

	const VuoPoint4d left = VuoPoint4d_make(-1., 0., 0., 1.);
	const VuoPoint4d forward = VuoPoint4d_make(0., 0., 1., 1.);

	// get mesh bounds and center point
	VuoPoint4d min, max;
	VuoMeshUtility_bounds(*submesh, &min, &max);
	VuoPoint4d center = VuoPoint4d_multiply(VuoPoint4d_add(max, min), .5);
	unsigned int vertexCount = submesh->vertexCount;

	// Insert vertical seam along western hemisphere
	for(unsigned int i = 0; i < submesh->elementCount; i+=3)
	{
		if(indices[i] > vertexCount || indices[i+1] > vertexCount || indices[i+2] > vertexCount)
		{
			VLog("Triangle indices are referencing out of bounds vertices. (%u, %u, %u)  Vertex Count: %u", indices[i], indices[i+1], indices[i+2], vertexCount);
			continue;
		}
		// {0,1} {1,2}, {2,0}
		for(int n = 0; n < 3; n++)
		{
			unsigned int x = i + n;
			unsigned int y = i + (n == 2 ? 0 : n+1);

			// check if this triangle edge crosses the new seam
			VuoPoint4d v0 = VuoPoint4d_normalize(VuoPoint4d_subtract(positions[indices[x]], center));
			VuoPoint4d v1 = VuoPoint4d_normalize(VuoPoint4d_subtract(positions[indices[y]], center));

			// if v0 is in left back quadrant, and v1 is in the forward half, v0 needs to be duplicated.  Ditto in opposite order
			if( dot(v0, left) > 0 && dot(v0, forward) < 0 && dot(v1, forward) > 0 )
			{
				if(positions) VuoListAppendValue_VuoPoint4d(seam_positions, positions[indices[x]]);
				if(normals) VuoListAppendValue_VuoPoint4d(seam_normals, normals[indices[x]]);
				if(tangents) VuoListAppendValue_VuoPoint4d(seam_tangents, tangents[indices[x]]);
				if(bitangents) VuoListAppendValue_VuoPoint4d(seam_bitangents, bitangents[indices[x]]);
				if(textures) VuoListAppendValue_VuoPoint4d(seam_textures, textures[indices[x]]);

				seam_indices[x] = VuoListGetCount_VuoPoint4d(seam_positions)-1;
			}

			// if v1 is in left back quadrant, but v0 isn't, duplicate v1
			if( dot(v1, left) > 0 && dot(v1, forward) < 0 && dot(v0, forward) > 0 )
			{
				if(positions) VuoListAppendValue_VuoPoint4d(seam_positions, positions[indices[y]]);
				if(normals) VuoListAppendValue_VuoPoint4d(seam_normals, normals[indices[y]]);
				if(tangents) VuoListAppendValue_VuoPoint4d(seam_tangents, tangents[indices[y]]);
				if(bitangents) VuoListAppendValue_VuoPoint4d(seam_bitangents, bitangents[indices[y]]);
				if(textures) VuoListAppendValue_VuoPoint4d(seam_textures, textures[indices[y]]);

				seam_indices[y] = VuoListGetCount_VuoPoint4d(seam_positions)-1;
			}
		}
	}

	unsigned int seam_vertexCount = VuoListGetCount_VuoPoint4d(seam_positions);

	VuoPoint4d* _positions = (VuoPoint4d*)malloc( seam_vertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _normals = (VuoPoint4d*)malloc( seam_vertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _tangents = (VuoPoint4d*)malloc( seam_vertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _bitangents = (VuoPoint4d*)malloc( seam_vertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _textures = (VuoPoint4d*)malloc( seam_vertexCount * sizeof(VuoPoint4d) );

	for(int i = 0; i < seam_vertexCount; i++)
	{
		if(positions)	_positions[i] = VuoListGetValue_VuoPoint4d(seam_positions, i+1);
		if(normals)	 	_normals[i] = VuoListGetValue_VuoPoint4d(seam_normals, i+1);
		if(tangents)	_tangents[i] = VuoListGetValue_VuoPoint4d(seam_tangents, i+1);
		if(bitangents)	_bitangents[i] = VuoListGetValue_VuoPoint4d(seam_bitangents, i+1);
		if(textures)	_textures[i] = VuoListGetValue_VuoPoint4d(seam_textures, i+1);
	}

	if(submesh->positions) free(submesh->positions);
	if(submesh->normals) free(submesh->normals);
	if(submesh->tangents) free(submesh->tangents);
	if(submesh->bitangents) free(submesh->bitangents);
	if(submesh->textureCoordinates) free(submesh->textureCoordinates);

	submesh->vertexCount = seam_vertexCount;

	submesh->positions = _positions;
	submesh->normals = _normals;
	submesh->tangents = _tangents;
	submesh->bitangents = _bitangents;
	submesh->textureCoordinates = _textures;

	free(submesh->elements);
	submesh->elements = seam_indices;
}

/**
 * Compare function for triangle ordering qsort in removeUnusedVertices.
 */
static int compare(const void* lhs, const void* rhs)
{
	return ( *(int*)lhs - *(int*)rhs );
}

/**
 * Removes unused vertices from a mesh.
 */
void VuoMeshUtility_removeUnusedVertices(VuoSubmesh* mesh)
{
	unsigned int elementCount = mesh->elementCount;
	unsigned int vertexCount = mesh->vertexCount;

	unsigned int* sortedTriangles = (unsigned int*)malloc(sizeof(unsigned int) * elementCount);
	memcpy(sortedTriangles, mesh->elements, sizeof(unsigned int) * elementCount);

	// sort indices from smallest to largest
	qsort(sortedTriangles, elementCount, sizeof(unsigned int), compare);

	// lop off any elements that are referencing out of bounds vertices
	unsigned int last = elementCount-1;

	while( sortedTriangles[last] > vertexCount )
		last--;

	// if there are invalid indices, reallocate the the elements array with only the good'ns.
	if(last < elementCount-1)
	{
		unsigned int* pruned_elements = (unsigned int*)malloc(sizeof(unsigned int) * (last+1));

		unsigned int n = 0;

		for(unsigned int i = 0; i < elementCount; i++)
		{
			if(mesh->elements[i] < vertexCount)
				pruned_elements[n++] = mesh->elements[i];
		}

		elementCount = last+1;
		mesh->elementCount = elementCount;
		free(mesh->elements);
		mesh->elements = pruned_elements;
	}

	// generate list of unused indices
	std::vector<unsigned int> unused;

	int lastVal = -1;
	for(int i = 0; i < elementCount && sortedTriangles[i] < elementCount; i++)
	{
		for(int n = lastVal+1; n < sortedTriangles[i]; n++)
		{
			unused.push_back(n);
		}

		lastVal = sortedTriangles[i];
	}

	// rebuild index array by substracting the amount of vertices removed in the range lower
	// than element[i]
	int unusedCount = unused.size();
	for(int i = 0; i < elementCount; i++)
	{
		unsigned int vertex = mesh->elements[i];

		if(vertex > vertexCount)
			continue;

		int n = 0;

		for(auto& val : unused)
		{
			if(val < vertex)
				n++;
			else
				break;
		}

		mesh->elements[i] -= n;
	}

	// rebuild vertex arrays without the unused values
	// not using vertexCount-unusedCount because unused may contain duplicates
	VuoList_VuoPoint4d new_positions = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d new_normals = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d new_tangents = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d new_bitangents = VuoListCreate_VuoPoint4d();
	VuoList_VuoPoint4d new_textures = VuoListCreate_VuoPoint4d();

	unsigned int curIndex = 0;
	for(int i = 0; i < vertexCount; i++)
	{
		if(curIndex >= unusedCount || i < unused[curIndex])
		{
			if(mesh->positions)				VuoListAppendValue_VuoPoint4d(new_positions, mesh->positions[i]);
			if(mesh->normals)				VuoListAppendValue_VuoPoint4d(new_normals, mesh->normals[i]);
			if(mesh->tangents)				VuoListAppendValue_VuoPoint4d(new_tangents, mesh->tangents[i]);
			if(mesh->bitangents)			VuoListAppendValue_VuoPoint4d(new_bitangents, mesh->bitangents[i]);
			if(mesh->textureCoordinates)	VuoListAppendValue_VuoPoint4d(new_textures, mesh->textureCoordinates[i]);
		}
		else
		{
			do {
				curIndex++;
			} while(curIndex < unusedCount && unused[curIndex] <= i);
		}
	}

	// Re-apply points to mesh
	unsigned int culledVertexCount = VuoListGetCount_VuoPoint4d(new_positions);

	VuoPoint4d* _positions = (VuoPoint4d*)malloc( culledVertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _normals = (VuoPoint4d*)malloc( culledVertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _tangents = (VuoPoint4d*)malloc( culledVertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _bitangents = (VuoPoint4d*)malloc( culledVertexCount * sizeof(VuoPoint4d) );
	VuoPoint4d* _textures = (VuoPoint4d*)malloc( culledVertexCount * sizeof(VuoPoint4d) );

	for(int i = 0; i < culledVertexCount; i++)
	{
		if(mesh->positions)				_positions[i] = VuoListGetValue_VuoPoint4d(new_positions, i+1);
		if(mesh->normals)				_normals[i] = VuoListGetValue_VuoPoint4d(new_normals, i+1);
		if(mesh->tangents)				_tangents[i] = VuoListGetValue_VuoPoint4d(new_tangents, i+1);
		if(mesh->bitangents)			_bitangents[i] = VuoListGetValue_VuoPoint4d(new_bitangents, i+1);
		if(mesh->textureCoordinates)	_textures[i] = VuoListGetValue_VuoPoint4d(new_textures, i+1);
	}

	if(mesh->positions) free(mesh->positions);
	if(mesh->normals) free(mesh->normals);
	if(mesh->tangents) free(mesh->tangents);
	if(mesh->bitangents) free(mesh->bitangents);
	if(mesh->textureCoordinates) free(mesh->textureCoordinates);

	mesh->vertexCount = culledVertexCount;

	mesh->positions = _positions;
	mesh->normals = _normals;
	mesh->tangents = _tangents;
	mesh->bitangents = _bitangents;
	mesh->textureCoordinates = _textures;
}

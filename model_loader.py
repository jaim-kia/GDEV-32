# 3d model loader for .obj files for OPENGL applications
import os
import sys
import math

# USAGE: python model_loader.py <file>.obj
#
# this script reads a .obj file and outputs a formatted text file
# tho before copy pasting to opengl, follow this checklist first:
# 
# 1. make sure obj file only has 'v', 'vt', 'vn', and 'f' lines
# 2. make sure obj is accompanied by texture image file (png, jpg, etc)
# 3. after running this script, check if vertices are exactly 11 values long (IMPORTANT!!!)
#       add/remove padding as necessary (see comments in output_final_model function)
# 4. copy output.txt contents into your opengl's vertex array oww yeah
# 
# marco notes :DD
# 
# obj notations
#
# storage buffers
#
# v  - vertex               example: v 0.123 0.234 0.345
# vt - texture coordinate   example: vt 0.0 0.0
# vn - vertex normal        example: vn 0.707 0.000 0.707
# f  - face                 example: f 5/1 6/2 2/3 1/4
#
# defining elements
#
# the 'f' lines define faces using INDICES of verticies, texture coordinates, and vertex normals.
# they use a 'vertex_index/texture_index/normal_index format'
# (ex. in "f 5/1/3 6/2/4 2/3/2 1/4", "5/1/3" means the face uses the 5th vertex, 1st texture coordinate, and 3rd normal,
# "6/2/4" means the 6th vertex, 2nd texture coordinate, and 4th normal, and so on.)
#
# parser logic
#
# basically, the idea is to
# 1. put the data into their own lists (verts, tex_coords, normals, faces)
# 2. using the face definitions, create the final list that will be used for rendering
#    by indexing into the original lists

vertices = []
vertex_textures = []
vertex_normals = []
faces = []

final_vertices = []  # position, texture coord, normal


def load_obj(file_path):
    if not os.path.isfile(file_path):
        raise FileNotFoundError(f"The file {file_path} does not exist.")

    with open(file_path, 'r') as file:
        for line in file:
            if line.startswith('v '):  # vertex line
                parts = line.strip().split()
                # start from index 1 to skip 'v'
                vertex = tuple(float(coord) for coord in parts[1:4])
                vertices.append(vertex)
            elif line.startswith('vt '):  # vertex texture line
                parts = line.strip().split()
                texture = tuple(float(coord) for coord in parts[1:3])
                vertex_textures.append(texture)
            elif line.startswith('vn '):  # vertex normal line
                parts = line.strip().split()
                normal = tuple(float(coord) for coord in parts[1:4])
                vertex_normals.append(normal)
            elif line.startswith('f '):
                parts = line.strip().split()[1:]  # skip the 'f'
                
                face = []
                for part in parts:
                    indices = part.split('/')
                    face.append(tuple(indices))
                faces.append(tuple(face))

def print_model_data():
    print("Vertices:")
    for v in vertices:
        print(v)
    print("\nVertex Textures:")
    for vt in vertex_textures:
        print(vt)
    print("\nVertex Normals:")
    for vn in vertex_normals:
        print(vn)
    print("\nFaces:")
    for f in faces:
        print(f)


def assemble_final_model():
    for face in faces:
        if len(face) == 3:
            for index in face:
                # print(index)
                vertex = []
                for i in vertices[int(index[0]) - 1]:
                    vertex.append(i)  # vertex position
                
                for j in vertex_textures[int(index[1]) - 1]:
                    vertex.append(j)  # texture coordinate
                
                if len(index) > 2:
                    for k in vertex_normals[int(index[2]) - 1]:
                        vertex.append(k)  # vertex normal
                # print("tuple(vertex): ", tuple(vertex))
                final_vertices.append(tuple(vertex))

        elif len(face) == 4:
            quad_indices = [0, 1, 2, 0, 2, 3]
            for idx in quad_indices:
                index = face[idx]
                vertex = []
                for i in vertices[int(index[0]) - 1]:
                    vertex.append(i)  # vertex position
                
                for j in vertex_textures[int(index[1]) - 1]:
                    vertex.append(j)  # texture coordinate
                
                if len(index) > 2:
                    for k in vertex_normals[int(index[2]) - 1]:
                        vertex.append(k)  # vertex normal
                final_vertices.append(tuple(vertex))

# tangent space calculation helper functions
def vec3_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])

def vec2_sub(a, b):
    return (a[0] - b[0], a[1] - b[1])

def vec3_dot(a, b):
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

def vec3_cross(a, b):
    return (
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0]
    )

def vec3_scale(v, s):
    return (v[0]*s, v[1]*s, v[2]*s)

def vec3_add(a, b):
    return (a[0]+b[0], a[1]+b[1], a[2]+b[2])

def vec3_normalize(v):
    length = math.sqrt(vec3_dot(v, v))
    if length == 0:
        return (0.0, 0.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)

def compute_tangent_space():
    global final_vertices

    new_vertices = []

    for i in range(0, len(final_vertices), 3):
        v0 = final_vertices[i]
        v1 = final_vertices[i + 1]
        v2 = final_vertices[i + 2]

        p0 = v0[0:3]
        uv0 = v0[3:5]
        n0 = v0[5:8]

        p1 = v1[0:3]
        uv1 = v1[3:5]

        p2 = v2[0:3]
        uv2 = v2[3:5]

        edge1 = vec3_sub(p1, p0)
        edge2 = vec3_sub(p2, p0)

        deltaUV1 = vec2_sub(uv1, uv0)
        deltaUV2 = vec2_sub(uv2, uv0)

        denom = deltaUV1[0] * deltaUV2[1] - deltaUV2[0] * deltaUV1[1]

        if abs(denom) < 1e-8:
            tangent = (0.0, 0.0, 0.0)
        else:
            f = 1.0 / denom

            tangent = (
                f * (deltaUV2[1] * edge1[0] - deltaUV1[1] * edge2[0]),
                f * (deltaUV2[1] * edge1[1] - deltaUV1[1] * edge2[1]),
                f * (deltaUV2[1] * edge1[2] - deltaUV1[1] * edge2[2])
            )

        n = vec3_normalize(n0)
        t = vec3_sub(tangent, vec3_scale(n, vec3_dot(n, tangent)))
        t = vec3_normalize(t)

        new_vertices.append(v0 + t)
        new_vertices.append(v1 + t)
        new_vertices.append(v2 + t)

    # format final vertices to be 11 values long (pos(3), tex(2), normal(3), tangent(3))
    final_vertices = new_vertices



def output_final_model(output_file):
    with open(output_file, "w") as file:
        for v in final_vertices:
            for f in v:
                file.write(f"{f}, ")
            file.write("\n")
    
    print(f"{len(final_vertices)} total vertices. Output written to {output_file}")


def main(file, output_file="output.txt"):
    file_path = os.path.join(os.path.dirname(__file__), file)
    load_obj(file_path)
    print(f"Loaded {len(vertices)} vertices, {len(vertex_textures)} texture coordinates, {len(vertex_normals)} normals, and {len(faces)} faces.\n")

    assemble_final_model()
    compute_tangent_space()

    output_final_model(output_file)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python model_loader.py <file>")
        sys.exit(1)
    elif len(sys.argv) == 2:
        main(sys.argv[1])
    elif len(sys.argv) == 3:
        main(sys.argv[1], sys.argv[2])



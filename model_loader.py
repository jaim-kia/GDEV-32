# 3d model loader for .obj files for OPENGL applications
import os
import sys

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
                vertex = []
                for i in vertices[int(index[0]) - 1]:
                    vertex.append(i)  # vertex position
                
                for j in vertex_textures[int(index[1]) - 1]:
                    vertex.append(j)  # texture coordinate
                
                if len(index) > 2:
                    for k in vertex_normals[int(index[2]) - 1]:
                        vertex.append(k)  # vertex normal
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


def output_final_model():
    with open("output.txt", "w") as file:
        # file.write(file_content)
        for v in final_vertices:
            for f in v:
                file.write(f"{f}, ")
            file.write("1.0, 1.0, 1.0, ") # normal
            file.write("1.0, 1.0, 1.0, ") # color
            file.write("\n")
    
    print(f"{len(final_vertices)} total vertices" )


def main(file):
    file_path = os.path.join(os.path.dirname(__file__), file)
    load_obj(file_path)
    print(f"Loaded {len(vertices)} vertices, {len(vertex_textures)} texture coordinates, {len(vertex_normals)} normals, and {len(faces)} faces.\n")

    # print_model_data()

    assemble_final_model()

    output_final_model()

    # print(final_vertices)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python model_loader.py <file>")
        sys.exit(1)
    main(sys.argv[1])



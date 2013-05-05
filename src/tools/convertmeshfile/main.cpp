
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Project headers.
#include "commandlinehandler.h"

// appleseed.shared headers.
#include "application/application.h"
#include "application/superlogger.h"

// appleseed.foundation headers.
#include "foundation/math/vector.h"
#include "foundation/mesh/genericmeshfilereader.h"
#include "foundation/mesh/genericmeshfilewriter.h"
#include "foundation/mesh/imeshbuilder.h"
#include "foundation/mesh/imeshwalker.h"
#include "foundation/platform/compiler.h"
#include "foundation/utility/foreach.h"

// Standard headers.
#include <cstddef>
#include <deque>
#include <list>
#include <string>
#include <vector>

using namespace appleseed::convertmeshfile;
using namespace appleseed::shared;
using namespace foundation;
using namespace std;

namespace
{
    struct Face
    {
        vector<size_t>      m_vertices;
        vector<size_t>      m_vertex_normals;
        vector<size_t>      m_vertex_tex_coords;
        size_t              m_material;
    };

    struct Mesh
    {
        string              m_name;
        vector<Vector3d>    m_vertices;
        vector<Vector3d>    m_vertex_normals;
        vector<Vector2d>    m_tex_coords;
        vector<string>      m_material_slots;
        deque<Face>         m_faces;
    };

    class MeshBuilder
      : public IMeshBuilder
    {
      public:
        const list<Mesh>& get_meshes() const
        {
            return m_meshes;
        }

        virtual void begin_mesh(const char* name) OVERRIDE
        {
            m_current_mesh = Mesh();
            m_current_mesh.m_name = name;
        }

        virtual size_t push_vertex(const Vector3d& v) OVERRIDE
        {
            m_current_mesh.m_vertices.push_back(v);
            return m_current_mesh.m_vertices.size() - 1;
        }

        virtual size_t push_vertex_normal(const Vector3d& v) OVERRIDE
        {
            m_current_mesh.m_vertex_normals.push_back(v);
            return m_current_mesh.m_vertex_normals.size() - 1;
        }

        virtual size_t push_tex_coords(const Vector2d& v) OVERRIDE
        {
            m_current_mesh.m_tex_coords.push_back(v);
            return m_current_mesh.m_tex_coords.size() - 1;
        }

        virtual size_t push_material_slot(const char* name) OVERRIDE
        {
            m_current_mesh.m_material_slots.push_back(name);
            return m_current_mesh.m_material_slots.size() - 1;
        }

        virtual void begin_face(const size_t vertex_count) OVERRIDE
        {
            m_current_face = Face();
            m_current_face.m_vertices.resize(vertex_count);
            m_current_face.m_vertex_normals.resize(vertex_count);
            m_current_face.m_vertex_tex_coords.resize(vertex_count);
            m_current_face.m_material = 0;
        }

        virtual void set_face_vertices(const size_t vertices[]) OVERRIDE
        {
            for (size_t i = 0; i < m_current_face.m_vertices.size(); ++i)
                m_current_face.m_vertices[i] = vertices[i];
        }

        virtual void set_face_vertex_normals(const size_t vertex_normals[]) OVERRIDE
        {
            for (size_t i = 0; i < m_current_face.m_vertex_normals.size(); ++i)
                m_current_face.m_vertex_normals[i] = vertex_normals[i];
        }

        virtual void set_face_vertex_tex_coords(const size_t tex_coords[]) OVERRIDE
        {
            for (size_t i = 0; i < m_current_face.m_vertex_tex_coords.size(); ++i)
                m_current_face.m_vertex_tex_coords[i] = tex_coords[i];
        }

        virtual void set_face_material(const size_t material) OVERRIDE
        {
            m_current_face.m_material = material;
        }

        virtual void end_face() OVERRIDE
        {
            m_current_mesh.m_faces.push_back(m_current_face);
        }

        virtual void end_mesh() OVERRIDE
        {
            m_meshes.push_back(m_current_mesh);
        }

      private:
        list<Mesh>  m_meshes;
        Mesh        m_current_mesh;
        Face        m_current_face;
    };

    class MeshWalker
      : public IMeshWalker
    {
      public:
        explicit MeshWalker(const Mesh& mesh)
          : m_mesh(mesh)
        {
        }

        virtual const char* get_name() const OVERRIDE
        {
            return m_mesh.m_name.c_str();
        }

        virtual size_t get_vertex_count() const OVERRIDE
        {
            return m_mesh.m_vertices.size();
        }

        virtual Vector3d get_vertex(const size_t i) const OVERRIDE
        {
            return m_mesh.m_vertices[i];
        }

        virtual size_t get_vertex_normal_count() const OVERRIDE
        {
            return m_mesh.m_vertex_normals.size();
        }

        virtual Vector3d get_vertex_normal(const size_t i) const OVERRIDE
        {
            return m_mesh.m_vertex_normals[i];
        }

        virtual size_t get_tex_coords_count() const OVERRIDE
        {
            return m_mesh.m_tex_coords.size();
        }

        virtual Vector2d get_tex_coords(const size_t i) const OVERRIDE
        {
            return m_mesh.m_tex_coords[i];
        }

        virtual size_t get_material_slot_count() const OVERRIDE
        {
            return m_mesh.m_material_slots.size();
        }

        virtual const char* get_material_slot(const size_t i) const OVERRIDE
        {
            return m_mesh.m_material_slots[i].c_str();
        }

        virtual size_t get_face_count() const
        {
            return m_mesh.m_faces.size();
        }

        virtual size_t get_face_vertex_count(const size_t face_index) const
        {
            return m_mesh.m_faces[face_index].m_vertices.size();
        }

        virtual size_t get_face_vertex(const size_t face_index, const size_t vertex_index) const
        {
            return m_mesh.m_faces[face_index].m_vertices[vertex_index];
        }

        virtual size_t get_face_vertex_normal(const size_t face_index, const size_t vertex_index) const
        {
            return m_mesh.m_faces[face_index].m_vertex_normals[vertex_index];
        }

        virtual size_t get_face_tex_coords(const size_t face_index, const size_t vertex_index) const
        {
            return m_mesh.m_faces[face_index].m_vertex_tex_coords[vertex_index];
        }

        virtual size_t get_face_material(const size_t face_index) const
        {
            return m_mesh.m_faces[face_index].m_material;
        }

      private:
        const Mesh& m_mesh;
    };
}


//
// Entry point of convertmeshfile.
//

int main(int argc, const char* argv[])
{
    SuperLogger logger;

    Application::check_installation(logger);

    CommandLineHandler cl;
    cl.parse(argc, argv, logger);

    // Retrieve the input and output file paths.
    const string& input_filepath = cl.m_filename.values()[0];
    const string& output_filepath = cl.m_filename.values()[1];

    // Read the input mesh file.
    MeshBuilder builder;
    GenericMeshFileReader reader(input_filepath.c_str());
    reader.read(builder);

    // Write the output mesh file.
    GenericMeshFileWriter writer(output_filepath.c_str());
    for (const_each<list<Mesh> > i = builder.get_meshes(); i; ++i)
    {
        const MeshWalker walker(*i);
        writer.write(walker);
    }

    return 0;
}
//
//  debug_renderer.h
//  tau3d
//
//  Created by Carl Johan Gribel on 2016-12-12.
//
//

#ifndef Renderer_h
#define Renderer_h

#include <vector>
#include <list>
#include <unordered_map>
#include <stack>

#include "config.h"
#include "glcommon.h"

#include "ShaderLoader.h"
#include "vec.h"
#include "mat.h"
#include "hash_combine.h"

#define BUFOFS(offset) ( (GLvoid*)(offset) )

using namespace linalg;
class Ray;

//
// Debug Renderer
// Dynamic Batching / Immediate Mode / Draw-and-Dispose type of renderer
//
// Supports polygon, line and point geometry
//
// Spec: https://docs.google.com/document/d/12gMZiiiGAL2spWb5T30PNCTAmi-0PFzTNL0XMvbPtxQ/edit#heading=h.odm9z4h5t34

namespace Renderer {

    // #define IM_COL32, imgui.h

    using uint = uint32_t;
    using uchar = unsigned char;

    template<typename... Types>
    class StateStack {

        std::tuple<std::stack<Types>...> stacks;

        template<typename T>
        std::stack<T>& getStack()
        {
            return std::get<std::stack<T>>(stacks);
        }

        template<typename T>
        const std::stack<T>& getStack() const
        {
            return std::get<std::stack<T>>(stacks);
        }

        template<typename T>
        void push_impl(const T& value)
        {
            getStack<T>().push(value);
        }

        template<typename T>
        void pop_impl()
        {
            assert(!empty<T>());
            getStack<T>().pop();

            // The default state should not be popped
            assert(!empty<T>());
        }

        template<typename T>
        T& top_impl()
        {
            assert(!empty<T>());
            return getStack<T>().top();
        }

    public:

        template<typename... Args>
        void push(Args&&... args)
        {
            (push_impl(std::forward<Args>(args)), ...);
        }

        template<typename... Args>
        void pop()
        {
            (pop_impl<Args>(), ...);
        }

        template<typename... Args>
            requires (sizeof...(Args) > 1)
        auto top()
        {
            return std::make_tuple(top_impl<Args>()...);
        }

        template<typename T>
        T& top()
        {
            return top_impl<T>();
        }

        //    template<typename T>
        //    T& top() {
        //        assert(!empty<T>());
        //        return getStack<T>().top();
        //    }

        template<typename T>
        bool empty() const
        {
            return getStack<T>().empty();
        }
    };

    struct Color4u
    {
        using uint = uint32_t;
        uint color;

        Color4u(uint color)
            : color(color)
        {

        }

        Color4u(uchar r, uchar g, uchar b, uchar a)
        {
            const uint r_shift = 0;
            const uint g_shift = 8;
            const uint b_shift = 16;
            const uint a_shift = 24;

            color = ((uint)a << a_shift) | ((uint)b << b_shift) | ((uint)g << g_shift) | ((uint)r << r_shift);
        }

        Color4u(const v4f& cf)
            : Color4u((uchar)(255 * cf.x), (uchar)(255 * cf.y), (uchar)(255 * cf.z), (uchar)(255 * cf.w))
        {
        }

        Color4u(const v3f& cf)
            : Color4u(xyz1(cf))
        {
        }

        operator uint() const { return color; }

        static const Color4u Black;
        static const Color4u White;
        static const Color4u Red;
        static const Color4u Lime;
        static const Color4u Blue;
        static const Color4u Yellow;
        static const Color4u Cyan;
        static const Color4u Magenta;
        static const Color4u Gray;
        static const Color4u Maroon;
        static const Color4u Olive;
        static const Color4u Green;
        static const Color4u Purple;
        static const Color4u Teal;
        static const Color4u Navy;
        static const Color4u Orange;
        static const Color4u Pink;
        static const Color4u Brown;
        static const Color4u Silver;
        static const Color4u Gold;
        static const Color4u Turquoise;
    };

    struct PolyVertex
    {
        vec3f p;
        vec3f normal;
        uint color;
    };

    struct LineVertex
    {
        vec3f p;
        uint color;
    };

    struct PointVertex
    {
        vec3f p;
        uint color;
    };

    struct ArrowDescriptor
    {
        //ArrowDescriptor(const ArrowDescriptor&) = default;

        float cone_fraction;
        float cone_radius;
        float cylinder_radius;
    };

    enum class DepthTest : bool
    {
        True = true,
        False = false
    };

    enum class BackfaceCull : bool
    {
        True = true,
        False = false
    };

    class ImPrimitiveRenderer
    {
        long framenbr = 0;

        // Shaders
        GLuint lambert_shader;
        GLuint line_shader;
        GLuint point_shader;

        // Pre-initialized primitives
        struct
        {
            std::vector<PolyVertex> vertices;
            std::vector<unsigned> indices;
        }
        unitcone_vbo,
            unitcylinder_vbo,
            unitsphere_vbo,
            unitspherewireframe_vbo;

        //
        // Line hash
        //
        // Topology:    GL_LINES
        // Key:         color (vec3f) + [Todo] line width. Consider which hash function to use (avoid changing line width often).
        // Value:       indices
        //

        struct LineDrawcall
        {
            GLenum topology = GL_LINES;
            DepthTest depth_test = DepthTest::True;

            bool operator == (const LineDrawcall& dc) const
            {
                return
                    topology == dc.topology &&
                    depth_test == dc.depth_test;
            }
        };

        struct LineDrawcallHashFunction
        {
            std::size_t operator () (const LineDrawcall& ldc) const
            {
                return hash_combine(ldc.topology, ldc.depth_test);
            }
        };

        std::vector<LineVertex> line_vertices;
        std::unordered_map<LineDrawcall, std::vector<unsigned>, LineDrawcallHashFunction> line_hash;

        GLuint lines_VBO = 0;
        GLuint lines_IBO = 0;
        GLuint lines_VAO = 0;

        //
        // Polygon/general hash
        //
        // Topology:    custom
        // Key:         { topology, color (int) }
        // Value:       index-range
        //

        struct PolygonDrawcall
        {
            GLenum topology = GL_TRIANGLES;
            DepthTest depth_test = DepthTest::True;
            BackfaceCull cull_face = BackfaceCull::True;

            bool operator == (const PolygonDrawcall& dc) const
            {
                return
                    topology == dc.topology &&
                    depth_test == dc.depth_test &&
                    cull_face == dc.cull_face;
            }
        };

        struct PolygonDrawcallHashfunction
        {
            std::size_t operator () (const PolygonDrawcall& pdc) const
            {
                return hash_combine(pdc.topology, pdc.depth_test);
            }
        };

        struct IndexRange
        {
            GLsizei start, size;
            GLint ofs;
        };

        std::vector<PolyVertex> polygon_vertices;
        std::vector<unsigned> polygon_indices;
        // Motivation for unordered_multimap over multimap:
        // insertion: avg constant (N worst case) vs avg logN
        // count (if used in iteration) avg N in nbr of matches (N in size worst case) vs logN, plus N in the number of matches.
        std::unordered_multimap<PolygonDrawcall, IndexRange, PolygonDrawcallHashfunction> polygon_hash;

        GLuint polygon_vbo = 0;
        GLuint polygon_ibo = 0;
        GLuint polygon_vao = 0;

        //
        // Point batch
        //
        // Topology:    GL_POINTS
        // Key:         point-size
        // Value:       { position, color }
        //

        struct PointDrawcall
        {
            unsigned size;
            DepthTest depth_test = DepthTest::True;

            bool operator == (const PointDrawcall& pdc) const
            {
                return depth_test == pdc.depth_test && size == pdc.size;
            }
        };

        struct PointDrawcallHashFunction
        {
            std::size_t operator () (const PointDrawcall& pdc) const
            {
                return  std::hash<GLenum>{}(pdc.size);
                //            return  ((std::hash<GLenum>()(pdc.topology) ^ (std::hash<int>()(pdc.color) << 1)) >> 1);
            }
        };

        std::unordered_map<PointDrawcall, std::vector<PointVertex>, PointDrawcallHashFunction > point_hash;

        StateStack<DepthTest, BackfaceCull, m4f, Color4u> state_stack;

        std::stack<DepthTest> depthtest_stack;
        std::stack<BackfaceCull> cullface_stack;

        GLuint point_vbo = 0;
        GLuint point_vao = 0;

        bool initialized = false;

    public:
        void init();

        template<typename... Args>
        void push_states(Args&&... args) {
            state_stack.push(std::forward<Args>(args)...);
        }

        template<typename... Args>
        void pop_states() {
            state_stack.pop<Args...>();
        }

        template<typename... Args>
        auto get_states()
        {
            return state_stack.top<Args...>();
        }

        template<class T>
        auto get_state_integral()
        {
            return to_integral(state_stack.top<T>());
        }

        void push_quad(const v3f points[4],
            const v3f& n);

        void push_quad();

        void push_quad_wireframe();

        void push_cube();

        void push_cube_wireframe();

        template<unsigned N>
        void push_circle_ring()
        {
            const auto [transform, color, depth_test] = get_states<m4f, Color4u, DepthTest>();

            auto vertex_generator =
                []()
                {
                    std::vector<v4f> vertices;
                    for (int i = 0; i < N; i++)
                    {
                        const float theta = i * 2.0f * fPI / (N - 1);
                        vertices.push_back(v4f{
                            std::cos(theta), std::sin(theta), 0.0f, 1.0f });
                    }
                    return vertices;
                };
            static const auto vertices = vertex_generator();

            auto& index_batch = line_hash[LineDrawcall{ GL_LINES, depth_test }];
            unsigned vertex_ofs = (unsigned)line_vertices.size();

            for (int i = 0; i < N; i++)
            {
                line_vertices.push_back(LineVertex{
                    xyz(transform * vertices[i]),
                    color
                    });
                if (i < N - 1) {
                    index_batch.push_back(vertex_ofs + i);
                    index_batch.push_back(vertex_ofs + i + 1);
                }
            }
        }

        void push_line(const vec3f& pos0, const vec3f& pos1);

        void push_lines_from_cyclic_source(const LineVertex* vertices,
            int start_index,
            int nbr_vertices,
            int max_vertices);

        void push_lines(const std::vector<v3f>& vertices,
            const std::vector<unsigned>& indices);

        void push_lines(const v3f* vertices,
            size_t nbr_vertices,
            const unsigned* indices,
            size_t nbr_indices);

        void push_lines(const v3f* vertices, size_t nbr_vertices);

        void push_grid(const vec3f& pos, unsigned size, unsigned resolution);

        void push_cone(const vec3f& from, const vec3f to, float r);

        void push_cone(float h, float r, bool flip_normals = false);

        void push_cylinder(float height, float radius, Ray* ray = nullptr);

        void push_arrow(const vec3f& from,
            const vec3f& to,
            ArrowDescriptor arrow_desc, // = {0.25f,0.15f,0.025f},
            Ray* ray = nullptr);

        void push_sphere(float h, float r);

        void push_sphere_wireframe(float h, float r);

        void push_helix(const vec3f& from,
            const vec3f& to,
            float r_outer,
            float r_inner,
            float revs);

        void push_helix(float length,
            float r_outer,
            float r_inner,
            float revs);

        void push_frustum(const mat4f& invProjView);

        void push_basis_basic(const m4f& basis, float arrlen);

        void push_basis_basic2d(const m4f& basis, float arrlen);

        void push_basis(const m4f& basis,
            float arrlen,
            const ArrowDescriptor& arrdesc,
            Ray* ray = nullptr);

        void push_point(const vec3f& p, unsigned size);

        void push_points(const PointVertex* p,
            unsigned nbr_points,
            unsigned size);

        int render(const mat4f& PROJ_VIEW);

        void post_render();
    };

} // Renderer

#endif /* Renderer */

/*!
 * \file obj.cpp
 * \author Igor Bereznyak <igor.bereznyak@gmail.com>
 *
 * \todo check format specs: http://www.martinreddy.net/gfx/3d/OBJ.spec
 *                           http://en.wikipedia.org/wiki/Wavefront_.obj_file
 *
 * \todo Each of these types of vertices is numbered separately, starting with
 * 1. This means that the first geometric vertex in the file is 1, the
 * second is 2, and so on. The first texture vertex in the file is 1, the
 * second is 2, and so on. The numbering continues sequentially throughout
 * the entire file. Frequently, files have multiple lists of vertex data.
 * This numbering sequence continues even when vertex data is separated by
 * other data.
 *
 * */

#include <minire/formats/obj.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <cctype>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <map>
#include <cassert>
#include <array>

// https://people.sc.fsu.edu/~jburkardt/data/obj/obj.html

namespace minire::formats
{
    bool Obj::validate() const
    {
        if (_faceVertices.size() != _faceNormals.size() && 
            _faceNormals.size() != 0)
        {
            return false;
        }
     
        if (_faceVertices.size() != _faceUvs.size() && 
            _faceUvs.size() != 0)
        {
            return false;
        }

        return true;
    }

    namespace
    {
        template<size_t kMaxBufferSize = 512>
        class ObjLineParser
        {
        public:
            explicit ObjLineParser(Obj & output) : _output(output) {}

            void readLine(std::istream & is)
            {
                static const std::string kV = "v";
                static const std::string kVt = "vt";
                static const std::string kVn = "vn";
                static const std::string kF = "f";
                static const std::string kSharp = "#";

                _p = 0;
                _buffer[0] = '\0';
                is.getline(_buffer.data(), _buffer.size());

                skipWhitespace();
                if (isEnd()) return;

                if (skipToken(kV))
                {
                    _output._vertices.emplace_back(parseVec3());
                }
                else if (skipToken(kVt))
                {
                    _output._uvs.emplace_back(parseVec2());
                }
                else if (skipToken(kVn))
                {
                    _output._normals.emplace_back(parseVec3());
                }
                else if (skipToken(kF))
                {
                    parseFacePoint();
                    parseFacePoint();
                    parseFacePoint();
                }
                else if (skipToken(kSharp))
                {
                    _p = _buffer.size() - 1;
                }
                else
                {
                    MINIRE_WARNING("unknown item skipped: {}", ptr());
                    _p = _buffer.size() - 1;
                }

                skipWhitespace();
                if (!isEnd())
                {
                    MINIRE_THROW("resudial data left in line: /{}/: {}", _buffer.data(), ptr());
                }
            }

        private:
            // "v" or "v/t" or "v/t/n" or "v//n"
            void parseFacePoint()
            {
                _output._faceVertices.emplace_back(parseUint() - 1);
                if (!skipChar('/')) return;

                if (skipChar('/'))
                {
                    _output._faceNormals.emplace_back(parseUint() - 1);
                }
                else
                {
                    _output._faceUvs.emplace_back(parseUint() - 1);
                    if (skipChar('/'))
                    {
                        _output._faceNormals.emplace_back(parseUint() - 1);
                    }
                }
            }

        private:
            bool skipToken(std::string const & token)
            {
                skipWhitespace();

                size_t bufResidue = _buffer.size() - _p;
                if (bufResidue >= token.size() &&
                    0 == strncmp(ptr(), token.c_str(), token.size()) &&
                    isWhitespace(token.size()))
                {
                    _p += token.size();
                    return true;
                }
                return false;
            }

            bool skipChar(char c)
            {
                skipWhitespace();
                bool const skip = !isEnd() && chr() == c;
                _p += (skip ? 1 : 0);
                return skip;
            }

            void skipWhitespace() { while(!isEnd() && ::isspace(chr())) ++_p; }

            bool isWhitespace(size_t offset) const
            {
                size_t np = _p + offset;
                return np < _buffer.size() &&
                       ('\0' == _buffer[np] || ::isspace(_buffer[np]));
            }

            float parseFloat()
            {
                skipWhitespace();

                char * endptr;
                char const * begin = ptr();
                float result = std::strtof(begin, &endptr);
                if (HUGE_VAL == result) MINIRE_THROW("bad float token");
                _p += (endptr - begin);
                return result;
            }

            uint32_t parseUint()
            {
                skipWhitespace();

                char * endptr;
                char const * begin = ptr();
                int64_t result = std::strtoll(begin, &endptr, 10);
                if (LLONG_MAX == result || LLONG_MIN == result) MINIRE_THROW("bad int token");
                if (result <= 0) MINIRE_THROW("negative indeces not supported");
                if (result > std::numeric_limits<uint32_t>::max()) MINIRE_THROW("too large index");
                _p += (endptr - begin);
                return static_cast<uint32_t>(result);
            }

            glm::vec2 parseVec2()
            {
                float x = parseFloat();
                float y = parseFloat();
                return glm::vec2(x, y);
            }

            glm::vec3 parseVec3()
            {
                float x = parseFloat();
                float y = parseFloat();
                float z = parseFloat();
                return glm::vec3(x, y, z);
            }

            bool isEnd() const { return _p >= _buffer.size() || '\0' == *ptr(); }
            char const * ptr() const { return &_buffer[_p]; }
            char chr() const { return _buffer[_p]; }

        private:
            using Buffer = std::array<char, kMaxBufferSize>;

            size_t    _p = 0;
            Buffer    _buffer;
            Obj     & _output;
        };
    }

    /*!
     * Load from file
     * */
    Obj loadObj(std::string const & filename)
    {
        std::ifstream ifs(filename, std::ifstream::in);
        if (!ifs) MINIRE_THROW("failed to open: \"{}\"", filename);
        return loadObj(ifs);
    }

    /*!
     * Load from input stream
     * */
    Obj loadObj(std::istream & is)
    {
        Obj result;
        ObjLineParser<512> parser(result);
        while(is.good()) parser.readLine(is);
        assert(result.validate());
        return result;
    }

    namespace
    {
        void save(glm::vec2 const & i, std::ostream & os)
        {
            os << i.x << ' ' << i.y;
        }

        void save(glm::vec3 const & i, std::ostream & os)
        {
            os << i.x << ' ' << i.y << ' ' << i.z;
        }

        template<typename T>
        void save(char const * pfx,
                  std::vector<T> const & buf,
                  std::ostream & os)
        {
            for(T const & i : buf)
            {
                os << pfx << ' '; save(i, os); os << '\n';
            }
            os << '\n';
        }

        template<typename Serializer>
        void save_f(size_t const count, std::ostream & os,
                    Serializer serializer)
        {
            assert((count % 3) == 0);
            for(size_t i(0); i < count; i += 3)
            {
                os << "f";
                os << " "; serializer(i + 0, os);
                os << " "; serializer(i + 1, os);
                os << " "; serializer(i + 2, os);
                os << '\n';
            }
        }

        void save_v(Obj::Faces const & fv, std::ostream & os)
        {
            save_f(fv.size(), os, [&fv](size_t const i, std::ostream & os)
                   { os << fv[i]+1; });
        }

        void save_vt(Obj::Faces const & fv, Obj::Faces const & ft,
                     std::ostream & os)
        {
            assert(fv.size() == ft.size());
            save_f(fv.size(), os, [&fv, &ft](size_t const i, std::ostream & os)
                   { os << (fv[i]+1) << '/' << (ft[i]+1); });
        }

        void save_vtn(Obj::Faces const & fv, Obj::Faces const & ft,
                      Obj::Faces const & fn, std::ostream & os)
        {
            assert(fv.size() == ft.size());
            assert(ft.size() == fn.size());
            save_f(fv.size(), os, [&fv, &ft, &fn](size_t const i, std::ostream & os)
                   { os << (fv[i]+1) << '/' << (ft[i]+1) << '/' << (fn[i]+1); });

        }

        void save_vn(Obj::Faces const & fv, Obj::Faces const & fn,
                     std::ostream & os)
        {
            assert(fv.size() == fn.size());
            save_f(fv.size(), os, [&fv, &fn](size_t const i, std::ostream & os)
                   { os << (fv[i]+1) << "//" << (fn[i]+1); });
        }
    }

    void saveObj(Obj const & mesh, std::string const & filename)
    {
        std::ofstream ofs(filename, std::ofstream::out
                                  | std::ofstream::trunc);
        saveObj(mesh, ofs);
    }

    void saveObj(Obj const & mesh, std::ostream & os)
    {
        os << "# minire (aka bznk-lib) .obj generator\n";
        os << std::setprecision(6) << std::fixed;

        save("v", mesh._vertices, os);
        save("vn", mesh._normals, os);
        save("vt", mesh._uvs, os);

        unsigned code = (mesh._faceNormals.empty() ? 0 : 2)
                      | (mesh._faceUvs.empty() ? 0 : 1);

        switch(code)
        {
            case 0: save_v(mesh._faceVertices, os); break;
            case 1: save_vt(mesh._faceVertices, mesh._faceUvs, os); break;
            case 2: save_vn(mesh._faceVertices, mesh._faceNormals, os); break;
            case 3: save_vtn(mesh._faceVertices, mesh._faceUvs, mesh._faceNormals, os); break;
            default: MINIRE_THROW("wrong mode code: {}", code);
        }
    }
}

#pragma once

#include <any>
#include <initializer_list>
#include <utility> // for std::forward

namespace minire::utils
{
    class UserData
    {
    public:
        UserData() = default;
        virtual ~UserData() = default;

        void setUserData(std::any userData) { _userData = userData; }

        template<typename T>
        void setUserData(T && userData)
        {
            _userData = std::forward<T>(userData);
        }

        template<typename T, typename ... Args>
        void emplaceUserData(Args && ... args)
        {
            _userData.emplace<T>(std::forward<Args>(args)...);
        }
 
        template<typename T, typename U, typename ... Args>
        void emplaceUserData(std::initializer_list<U> il,
                             Args && ... args)
        {
            _userData.emplace<T>(il, std::forward<Args>(args)...);
        }
 
    public:
        std::any const & userData() const { return _userData; }
        std::any & userData() { return _userData; }

        template<typename T>
        T const & userDataAs() const { return std::any_cast<T>(_userData); }

    private:
        std::any _userData;
    };
}
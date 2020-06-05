#ifndef HANDMADE_MATH_H

struct v2
{
    union
    {
        struct 
        {
            real32 X;
            real32 Y;
        };
        real32 E[2];
    };
    inline v2 &operator+=(v2 A);
    inline v2 &operator-=(v2 A);
    inline v2 &operator*=(real32 A);
};

inline v2 V2(real32 X, real32 Y)
{
    v2 Result = {};
    Result.X = X;
    Result.Y = Y;
    return Result;
}

inline v2 operator*(real32 A, v2 B)
{
    v2 Result;

    Result.X = A * B.X;
    Result.Y = A * B.Y;

    return Result;
}

inline v2 operator*(v2 B, real32 A)
{
    v2 Result;

    Result.X = A * B.X;
    Result.Y = A * B.Y;

    return Result;
}

inline v2 &v2::operator*=(real32 A)
{
    *this = A * *this;
    return *this;
}

inline v2 operator-(v2 A)
{
    v2 Result;
    Result.X = -A.X;
    Result.Y = -A.Y;

    return Result;
}

inline v2 operator+(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

inline v2 &v2::operator+=(v2 A)
{
    *this = A + *this;
    return *this;
}

inline v2 operator-(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

inline v2 &v2::operator-=(v2 A)
{
    *this = A - *this;
    return *this;
}


#define HANDMADE_MATH_H
#endif
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
    *this = *this - A;
    return *this;
}

inline real32 
Inner(v2 A, v2 B)
{
    real32 Result = A.X * B.X + A.Y * B.Y;
    return Result;
}

inline real32 
LengthSq(v2 A)
{
    real32 Result = Inner(A, A);
    return Result;
}

struct rectangle2
{
    v2 TopLeft;
    v2 BottomRight;
};

inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim)
{
    rectangle2 Rect;
    Rect.TopLeft = Center - HalfDim;
    Rect.BottomRight = Center + HalfDim;
    return Rect;
}

inline rectangle2
RectCenterDim(v2 Center, v2 Dim)
{
    rectangle2 Rect = RectCenterHalfDim(Center, 0.5f*Dim);
    return Rect;
}

inline bool 
IsInRectangle(rectangle2 Rect, v2 Test)
{
    bool Result = (
        (Test.X >= Rect.TopLeft.X) &&
        (Test.X < Rect.BottomRight.X) &&
        (Test.Y >= Rect.TopLeft.Y) &&
        (Test.Y < Rect.BottomRight.Y)
    );
    return Result;
}

inline v2
GetMinCorner(rectangle2 Rect)
{
    return Rect.TopLeft;
}

inline v2
GetMaxCorner(rectangle2 Rect)
{
    return Rect.BottomRight;
}

inline v2
GetCenter(rectangle2 Rect)
{
    v2 Result = 0.5f * (GetMinCorner(Rect) + GetMaxCorner(Rect));
    return Result;
}


#define HANDMADE_MATH_H
#endif
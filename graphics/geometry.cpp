union Point2 {
    struct {float32 x, y;};
    struct {float32 width, height;};
    float32 e[2];
};
typedef Point2 Dimensions2;

inline Point2 Lerp2(float32 t, Point2 p0, Point2 p1) {
    return {(1-t)*p0.x + t*p1.x, (1-t)*p0.y + t*p1.y};
}

inline Point2 Qerp2(float32 t, Point2 p0, Point2 p1, Point2 p2) {
    return {(1-t)*(1-t)*p0.x + 2*(1-t)*t*p1.x + t*t*p2.x,
            (1-t)*(1-t)*p0.y + 2*(1-t)*t*p1.y + t*t*p2.y};
}

inline Point2 Cerp2(float32 t, Point2 p0, Point2 p1, Point2 p2, Point2 p3) {
    return {(1-t)*(1-t)*(1-t)*p0.x + 3*(1-t)*(1-t)*t*p1.x + 3*(1-t)*t*t*p2.x + t*t*t*p3.x,
            (1-t)*(1-t)*(1-t)*p0.y + 3*(1-t)*(1-t)*t*p1.y + 3*(1-t)*t*t*p2.y + t*t*t*p3.y};
}

void PushPoint2(StringBuilder* builder, Point2 p, int32 precision) {
    builder->ptr += StringCopy("(", builder->ptr);
    builder->ptr += Float32ToDecimal(p.x, precision, builder->ptr);
    builder->ptr += StringCopy(", ", builder->ptr);
    builder->ptr += Float32ToDecimal(p.y, precision, builder->ptr);
    builder->ptr += StringCopy(")", builder->ptr);
}

union Point2i {
    struct {int32 x, y;};
    struct {int32 width, height;};
    int32 e[2];
};
typedef Point2i Dimensions2i;

void PushPoint2i(StringBuilder* builder, Point2i p) {
    builder->ptr += StringCopy("(", builder->ptr);
    builder->ptr += Int32ToDecimal(p.x, builder->ptr);
    builder->ptr += StringCopy(", ", builder->ptr);
    builder->ptr += Int32ToDecimal(p.y, builder->ptr);
    builder->ptr += StringCopy(")", builder->ptr);
}

#define MOVE2(p0, p1)           {(p0).x+(p1).x, (p0).y+(p1).y}
#define SCALE2(p, a)            {a*(p).x, a*(p).y}

union Box2 {
    struct {Point2 p0, p1;};
    struct {float32 x0, y0, x1, y1;};
};

union Box2i {
    struct {Point2i p0, p1;};
    struct {int32 x0, y0, x1, y1;};
};

#define INSIDE2(b, p)           ((b).x0<=(p).x && (p).x<=(b).x1 && (b).y0<=(p).y && (p).y<=(b).y1)

struct Line2 {
    Point2* points;
    int32 count;
};

struct Sphere2 {
    Point2 center;
    float32 radius;
};
typedef Sphere2 Ball2;

Point2 PointOnSphere(Sphere2 sphere, float32 rad) {
    return {sphere.radius*cosf(rad) + sphere.center.x, 
            sphere.radius*sinf(rad) + sphere.center.y};
}

union Point3 {
    struct {float32 x, y, z;};
    struct {float32 width, height, depth;};
    float32 e[3];
};
typedef Point3 Dimensions3;

union Point4 {
    struct {float32 x, y, z, w;};
    struct {float32 r, g, b, a;};
    float32 e[4];
};

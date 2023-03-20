union Point2 {
	struct {float32 x, y;};
	struct {float32 width, height;};
	float32 e[2];
};
typedef Point2 Dimensions2;

inline Point2 operator+(Point2 p0, Point2 p1) {
	return {p0.x + p1.x, p0.y + p1.y};
}

inline Point2 operator-(Point2 p0, Point2 p1) {
	return {p0.x - p1.x, p0.y - p1.y};
}

inline Point2 operator*(float32 a, Point2 p) {
	return {a*p.x, a*p.y};
}

inline Point2 Lerp(float32 t, Point2 p0, Point2 p1) {
	return (1-t)*p0 + t*p1;
}

inline Point2 Qerp(float32 t, Point2 p0, Point2 p1, Point2 p2) {
	return (1-t)*(1-t)*p0 + 2*(1-t)*t*p1 + t*t*p2;
}

inline Point2 Cerp(float32 t, Point2 p0, Point2 p1, Point2 p2, Point2 p3) {
	return (1-t)*(1-t)*(1-t)*p0 + 3*(1-t)*(1-t)*t*p1 + 3*(1-t)*t*t*p2 + t*t*t*p3;
}

inline float32 Dot(Point2 p0, Point2 p1) {
	return (p0.x*p1.x) + (p0.y*p1.y);
}

inline float32 DistanceSquared(Point2 p0, Point2 p1) {
	Point2 p2 = p0 - p1;
	return Dot(p2, p2);
}

union Point2i {
	struct {int32 x, y;};
	struct {int32 width, height;};
	int32 e[2];
};
typedef Point2i Dimensions2i;

inline Point2i operator+(Point2i p0, Point2i p1) {
	return {p0.x + p1.x, p0.y + p1.y};
}

inline Point2i operator-(Point2i p0, Point2i p1) {
	return {p0.x - p1.x, p0.y - p1.y};
}

inline Point2i operator*(int32 a, Point2i p) {
	return {a*p.x, a*p.y};
}

inline int32 Dot(Point2i p0, Point2i p1) {
	return (p0.x*p1.x) + (p0.y*p1.y);
}

inline int32 DistanceSquared(Point2i p0, Point2i p1) {
	Point2i p2 = p0 - p1;
	return Dot(p2, p2);
}

union Box2 {
	struct {Point2 p0, p1;};
	struct {float32 x0, y0, x1, y1;};
};

union Box2i {
	struct {Point2i p0, p1;};
	struct {int32 x0, y0, x1, y1;};
};

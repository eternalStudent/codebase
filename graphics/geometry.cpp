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

inline Point2 ceil(Point2 p) {
	return {ceil(p.x), ceil(p.y)};
}

inline Point2 floor(Point2 p) {
	return {floor(p.x), floor(p.y)};
}

inline Point2 round(Point2 p) {
	return {round(p.x), round(p.y)};
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

bool InBounds(Point2 pos, Dimensions2 dim, Point2 p) {
	return pos.x <= p.x && p.x < pos.x + dim.width && pos.y <= p.y && p.y < pos.y + dim.height;
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

inline Point2 operator+(Point2i p0, float32 a) {
	return {p0.x + a, p0.y + a};
}

inline int32 DistanceSquared(Point2i p0, Point2i p1) {
	Point2i p2 = p0 - p1;
	return Dot(p2, p2);
}

union Box2 {
	struct {Point2 p0, p1;};
	struct {float32 x0, y0, x1, y1;};
};

inline Box2 operator*(float32 a, Box2 b) {
	return {a*b.x0, a*b.y0, a*b.x1, a*b.y1};
}

inline Box2 operator+(Box2 a, Point2 p) {
	return {a.x0 + p.x, a.y0 + p.y, a.x1 + p.x, a.y1 + p.y};
}

inline Dimensions2 GetDim(Box2 box) {
	return {box.x1 - box.x0, box.y1 - box.y0};
}

inline bool InBounds(Box2 bounds, Point2 p) {
	return bounds.x0 <= p.x && p.x < bounds.x1 && bounds.y0 <= p.y && p.y < bounds.y1;
}

inline Box2 GetBounds(Point2 pos, Dimensions2 dim) {
	return {pos.x, pos.y, pos.x + dim.width, pos.y + dim.height};
}

inline Box2 GetIntersection(Box2 a, Box2 b) {
	float32 x0 = MAX(a.x0, b.x0);
	float32 y0 = MAX(a.y0, b.y0);
	float32 x1 = MAX(MIN(a.x1, b.x1), x0);
	float32 y1 = MAX(MIN(a.y1, b.y1), y0);
	return {x0, y0, x1, y1};
}

inline Box2 GetIntersection(Box2 box, Point2 pos, Dimensions2 dim) {
	return GetIntersection(box, GetBounds(pos, dim));
}

union Box2i {
	struct {Point2i p0, p1;};
	struct {int32 x0, y0, x1, y1;};
};

inline bool InBounds(Box2i bounds, Point2i p) {
	return bounds.x0 <= p.x && p.x < bounds.x1 && bounds.y0 <= p.y && p.y < bounds.y1;
}

union Point3 {
	struct {
		union {
			struct {float32 x, y;};
			Point2 xy;
		};
		float32 z;
	};
	float32 e[3];
};

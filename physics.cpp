typedef float32 pixels;
typedef float32 milliseconds;
typedef float32 pixels_per_millisec;
typedef float32 pixels_per_millisec_2;

typedef Point2  Vector2;
typedef Vector2 Position2;
typedef Vector2 Velocity2;
typedef Vector2 Acceleration2;

struct HitBox {
	pixels height; 
	pixels radius; 
	// pixels altitude;
};

struct PhysicalBody {
	union {
		HitBox hitBox;
		struct {pixels height, radius;};
	};
	Velocity2 velocity;
    bool onGround;
};
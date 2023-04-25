
static struct Rectangle2D *rect;
static struct Rectangle2D *ground;

static struct Circle2D *c1, *c2;

static V2 gravity;

struct PRect {
    V2 pos;
    V2 vel;
    V2 size;
    float mass;
    V2 force;
};

struct PShape2D {
    uint32_t shape; // to determine the support function
    V2 origin;
    // TODO: rotation?
    union {
        struct { // CONVEX polygon
            uint32_t vertCount;
            V2 *verts;
        } polygon;
        struct {
            V2 size;
        } rectangle;
        struct {
            float radius;
        } circle;
    };
};

enum PhysShape {
    Phys_Shape_None,
    Phys_Shape_Polygon,
    Phys_Shape_Rectangle,
    Phys_Shape_Circle,
};

typedef V2 (*SupportFunc)(struct PShape2D*, V2);

V2 support_polygon(struct PShape2D *s, V2 dir) {
    // TODO: implement
    return (V2){0.0f, 0.0f};
}

V2 support_circle(struct PShape2D *s, V2 dir) {
    V2 ret;
    v2_normalize(&dir);
    v2_add(&ret, s->origin, v2_scale(dir, s->circle.radius));
    return ret;
}

V2 support_rectangle(struct PShape2D *s, V2 dir) {
    // TODO: rotated rect
    V2 ret;
    V2 vertices[] = {
        -s->rectangle.size.x/2.0f, -s->rectangle.size.y/2.0f,
         s->rectangle.size.x/2.0f, -s->rectangle.size.y/2.0f,
        -s->rectangle.size.x/2.0f,  s->rectangle.size.y/2.0f,
         s->rectangle.size.x/2.0f,  s->rectangle.size.y/2.0f,
    };
    float max = -FLT_MAX;
    int idx = -1;
    for(int i = 0; i < 4; i++) {
        float dot = v2_dot(dir, vertices[i]);
        if(dot > max) {
            max = dot;
            idx = i;
        }
    }
    assert(idx != -1);
    v2_add(&ret, s->origin, vertices[idx]);
    return ret;
}

static SupportFunc suppArr[] = {NULL, support_polygon, support_rectangle, support_circle};

bool nearest_simplex(V2 simplex[], int *vertCount, V2 *dir) {
    // TODO; implement
    assert(*vertCount == 2 || *vertCount == 3);
    if(*vertCount == 2) {
        V2 AB;
        V2 A0 = v2_scale(simplex[1], -1.0f);
        v2_sub(&AB, simplex[0], simplex[1]);
        if(v2_dot(AB, A0) > 0.0f) { // between AB
            *dir = (V2){-AB.y, AB.x};
            if(v2_dot(*dir, A0) < 0.0) {
                (*dir).x *= -1.0f;
                (*dir).y *= -1.0f;
            }
        } else { // outside on A side
            *dir = A0;
            simplex[0] = simplex[1];
            *vertCount = 1;
        }
    } else {
        V2 AB, AC;
        V2 A0 = v2_scale(simplex[2], -1.0f);
        v2_sub(&AB, simplex[1], simplex[2]);
        v2_sub(&AC, simplex[0], simplex[2]);
        // (2->1 opposite of 2->0 perp) dot A0 pass
        V2 line1 = (V2){-AB.y, AB.x};
        // TODO: remove branch (with determinant)
        if(v2_dot(line1, AC) > 0.0f) {
            line1.x *= -1.0f;
            line1.y *= -1.0f;
        } 
        // (2->0 opposite of 2->1 perp) dot A0 pass
        V2 line2 = (V2){-AC.y, AC.x};
        // TODO: remove branch (with determinant)
        if(v2_dot(line2, AB) > 0.0f) {
            line2.x *= -1.0f;
            line2.y *= -1.0f;
        }    
        if(v2_dot(line1, A0) > 0.0f) {
            if(v2_dot(AB, A0) > 0.0f) {
                simplex[0] = simplex[2];
                *vertCount = 2;
                *dir = line1;
            } else {
                simplex[0] = simplex[2];
                *vertCount = 1;
                *dir = A0;
            }
        } else {
            if(v2_dot(line2, A0) > 0.0f) {
                if(v2_dot(AC, A0) > 0.0f) {
                    simplex[1] = simplex[0];
                    simplex[0] = simplex[2];
                    *vertCount = 2;
                    *dir = line2;
                } else {
                    simplex[0] = simplex[2];
                    *vertCount = 1;
                    *dir = A0;
                }
            } else {
                return true;
            }
        }
    }
    return false;
}

bool gjk_intersection(struct PShape2D *s1, struct PShape2D *s2, V2 axis, V2 rsimplex[]) {
    V2 A;
    v2_sub(&A, suppArr[s1->shape](s1, axis), suppArr[s2->shape](s2, v2_scale(axis, -1.0f)));
    V2 simplex[3];
    int vertCount = 1;
    simplex[0] = A;
    axis = v2_scale(A, -1.0f);
    bool ret = false;
    int its;
    for(its = 0; its < 32; its++) {
        v2_sub(&A, suppArr[s1->shape](s1, axis), suppArr[s2->shape](s2, v2_scale(axis, -1.0f)));
        if(v2_dot(axis, axis) <= FLT_EPSILON || v2_dot(A, axis) < 0.0f) {
            break;
        }
        simplex[vertCount++] = A;
        if(nearest_simplex(simplex, &vertCount, &axis)) {
            if(rsimplex != NULL) {
                rsimplex[0] = simplex[0];
                rsimplex[1] = simplex[1];
                rsimplex[2] = simplex[2];
            }
            /*V2 v1, v2, v3;
            v2_sub(&v1, s1->origin, simplex[0]);
            v2_sub(&v2, s1->origin, simplex[1]);
            v2_sub(&v3, s1->origin, simplex[2]);
            draw_line(v1, v2, 0x00000000);
            draw_line(v2, v3, 0x00000000);
            draw_line(v3, v1, 0x00000000);*/
            ret = true;
            break;
        } else if(vertCount == 3) {
            break;
        }
    }
    return ret;
}

V2 epa_penetration(struct PShape2D *s1, struct PShape2D *s2, V2 axis, float eps) {
    V2 simplex[32];
    uint32_t vertCount = 3;
    bool col = gjk_intersection(s1, s2, axis, simplex);
    while(col) {
        float min = FLT_MAX;
        int edge = -1;
        V2 cp;
        for(int i = 0; i < vertCount; i++) {
            V2 start = simplex[i];
            V2 end = simplex[(i+1)%vertCount];
            V2 cpc = closest_point_on_line_seg((V2){0.0f, 0.0f}, start, end);
            float d2 = cpc.x*cpc.x + cpc.y*cpc.y;
            if(d2 < min) {
                min = d2;
                edge = i;
                cp = cpc;
            }
        }
        {
            V2 start, end;
            v2_add(&end, cp, (V2){400.0f, 400.0f});
            draw_line((V2){400.0f, 400.0f}, end, 0xFF00FF00FF);
        }
        assert(edge != -1);
        V2 A;
        v2_sub(&A, suppArr[s1->shape](s1, cp), suppArr[s2->shape](s2, v2_scale(cp, -1.0f)));
        float md2 = A.x*A.x + A.y*A.y;
        if(abs(md2-min) <= eps) {
            printf("ok %f %f\r\n", cp.x, cp.y);
            /*for(int i = 0; i < vertCount; i++) {
                draw_line(simplex[i], simplex[(i+1)%vertCount], 0x00000000);
            }*/
            return cp;
        } else {
            // buffer too small to do any better
            if(vertCount == 32) {
                printf("not ok but out of buffer %f %f\r\n", cp.x, cp.y);
                for(int i = 0; i < vertCount; i++) {
                    V2 start, end;
                    v2_add(&start, simplex[i], (V2){400.0f, 400.0f});
                    v2_add(&end, simplex[(i+1)%vertCount], (V2){400.0f, 400.0f});
                    draw_line(start, end, 0x00000000);
                }
                return cp;
            }
            // TODO: avoid concave mesh
            // TODO: avoid branch somehow
            if(edge != vertCount) {
                for(int i = 0; i < vertCount - edge - 1; i++) {
                    int widx = vertCount-i;
                    simplex[widx] = simplex[widx-1];
                }
                simplex[edge+1] = A;
                vertCount++;
            } else {
                for(int i = 0; i < vertCount; i++) {
                    simplex[vertCount - i] = simplex[vertCount - i - 1];
                }
                simplex[0] = A;
                vertCount++;
            }
        }
    }
    return (V2){0.0f, 0.0f};
}

struct PShape2D c1s;
struct PShape2D c2s;

struct PRect prect;
struct PShape2D rect1s;

float dt;

bool dragging;
struct Rectangle2D *dragRect;
V2 dragOffset;

bool is_in_rectangle(Rectangle2D *r, V2 pos);
bool is_in_circle(Circle2D *c, V2 pos);

void mouse_move(V2 newPos, V2 deltaPos) {
    if(dragging) {
        dragRect->pos.x = newPos.x+dragOffset.x;
        dragRect->pos.y = newPos.y+dragOffset.y;
    }
}

void button_down(int button, V2 pos) {
    if(button == 0 && is_in_rectangle(rect, pos)) {
        printf("down %d %f %f \r\n", button, pos.x, pos.y);
        dragRect = rect;
        dragging = true;
        v2_sub(&dragOffset, (V2){rect->pos.x, rect->pos.y}, pos);
    }
}

void button_up(int button, V2 pos) {
    printf("up %d %f %f\r\n", button, pos.x, pos.y);
    if(button == 0) {
        dragging = false;
    }
}

bool is_in_rectangle(Rectangle2D *r, V2 pos) {
    if(pos.x >= r->pos.x-r->scale.x/2.0f 
            && pos.x <= r->pos.x+r->scale.x/2.0f
            && pos.y >= r->pos.y-r->scale.y/2.0f
            && pos.y <= r->pos.y+r->scale.y/2.0f) {
        return true;
    }
    return false;
}

bool is_in_circle(Circle2D *c, V2 pos) {
    float xdif = pos.x - c->pos.x;
    float ydif = pos.y - c->pos.y;
    float d = sqrtf(xdif*xdif + ydif*ydif);
    return d <= c->radius;
}

void init() {
    rect = create_rectangle((V3){400.0f, 100.0f, 0.0f}, 0.0f, (V2){50.0f, 50.0f});
    ground = create_rectangle((V3){400.0f, 600.5f, 0.0f}, 0.0f, (V2){2000.0f, 10.0f});
    prect.pos = (V2){rect->pos.x, rect->pos.y};
    prect.size = (V2){rect->scale.x, rect->scale.y};
    prect.mass = 1.0f;
    prect.force = (V2){0.0f, 0.0f};
    prect.vel = (V2){0.0f, 0.0f};
    rect1s.shape = Phys_Shape_Rectangle;
    rect1s.rectangle.size = prect.size;

    c1 = create_circle((V3){100.0f, 100.0f, 0.0f}, 50.0f);
    c1->color = 0x000000FF;
    c2 = create_circle((V3){100.0f, 250.0f, 0.0f}, 50.0f);

    c1s.shape = Phys_Shape_Circle;
    c1s.circle.radius = c1->radius;
    c2s.shape = Phys_Shape_Circle;
    c2s.circle.radius = c2->radius;

    dt = 1.0f / 60.0f;

    gravity = (V2){0.0f, 9.1f};
}

void update() {
    V2 acc;
    acc.x = gravity.x + prect.force.x/prect.mass;
    acc.y = gravity.y + prect.force.y/prect.mass;

    prect.vel = prect.vel + v2_scale(acc, dt);
    prect.pos = prect.pos + v2_scale(prect.vel, dt);

    c1->pos.y += 0.001f;

    c1s.origin = (V2){c1->pos.x, c1->pos.y};
    c2s.origin = (V2){c2->pos.x, c2->pos.y};

    rect1s.origin = (V2){rect->pos.x, rect->pos.y};

    bool col1 = gjk_intersection(&c1s, &rect1s, (V2){1.0f, 0.0f}, NULL);
    bool col2 = gjk_intersection(&c2s, &rect1s, (V2){1.0f, 0.0f}, NULL);
    printf("blue %d white %d \r\n", col1, col2);

    V2 pen = epa_penetration(&c1s, &rect1s, (V2){1.0f, 0.0f}, 0.001f);
    V2 end;
    v2_sub(&end, c1s.origin, pen);
    //draw_line(c1s.origin, end, 0);

    c2->pos.x = end.x;
    c2->pos.y = end.y;

    //rect->pos.x = prect.pos.x; 
    //rect->pos.y = prect.pos.y; 
}

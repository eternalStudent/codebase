struct hheap_chunk {
   hheap_chunk *next;
};

struct hheap {
   hheap_chunk *head;
   void* first_free;
   int32 num_remaining_in_head_chunk;
};

void *hheap_alloc(Arena* arena, hheap *hh, int64 size) {
   if (hh->first_free) {
      void *p = hh->first_free;
      hh->first_free = * (void **) p;
      return p;
   } else {
      if (hh->num_remaining_in_head_chunk == 0) {
         int32 count = (size < 32 ? 2000 : size < 128 ? 800 : 100);
         hheap_chunk *c = (hheap_chunk *) ArenaAlloc(arena, sizeof(hheap_chunk) + size * count);
         if (c == NULL)
            return NULL;
         c->next = hh->head;
         hh->head = c;
         hh->num_remaining_in_head_chunk = count;
      }
      --hh->num_remaining_in_head_chunk;
      return (char *) (hh->head) + sizeof(hheap_chunk) + size * hh->num_remaining_in_head_chunk;
   }
}

void hheap_free(hheap *hh, void *p) {
   *(void **) p = hh->first_free;
   hh->first_free = p;
}

void hheap_cleanup(hheap *hh) {
   hheap_chunk *c = hh->head;
   while (c) {
      hheap_chunk *n = c->next;
      c = n;
   }
}

struct edge {
   float32 x0, y0, x1, y1;
   int32 invert;
};


struct active_edge {
   struct active_edge *next;
   float32 fx, fdx, fdy;
   float32 direction;
   float32 sy;
   float32 ey;
};

active_edge *new_active(Arena* arena, hheap *hh, edge *e, int32 off_x, float32 start_point) {
   active_edge *z = (active_edge *) hheap_alloc(arena, hh, sizeof(*z));
   float32 dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
   ASSERT(z != NULL);
   if (!z) return z;
   z->fdx = dxdy;
   z->fdy = dxdy != 0.0f ? (1.0f/dxdy) : 0.0f;
   z->fx = e->x0 + dxdy * (start_point - e->y0);
   z->fx -= off_x;
   z->direction = e->invert ? 1.0f : -1.0f;
   z->sy = e->y0;
   z->ey = e->y1;
   z->next = 0;
   return z;
}

void handle_clipped_edge(float32 *scanline, int32 x, active_edge *e, 
                         float32 x0, float32 y0, float32 x1, float32 y1) {
   if (y0 == y1) return;
   ASSERT(y0 < y1);
   ASSERT(e->sy <= e->ey);
   if (y0 > e->ey) return;
   if (y1 < e->sy) return;
   if (y0 < e->sy) {
      x0 += (x1-x0) * (e->sy - y0) / (y1-y0);
      y0 = e->sy;
   }
   if (y1 > e->ey) {
      x1 += (x1-x0) * (e->ey - y1) / (y1-y0);
      y1 = e->ey;
   }

   if (x0 == x)
      {ASSERT(x1 <= x+1);}
   else if (x0 == x+1)
      {ASSERT(x1 >= x);}
   else if (x0 <= x)
      {ASSERT(x1 <= x);}
   else if (x0 >= x+1)
      {ASSERT(x1 >= x+1);}
   else
      {ASSERT(x1 >= x && x1 <= x+1);}

   if (x0 <= x && x1 <= x)
      scanline[x] += e->direction * (y1-y0);
   else if (x0 >= x+1 && x1 >= x+1)
      ;
   else {
      ASSERT(x0 >= x && x0 <= x+1 && x1 >= x && x1 <= x+1);
      scanline[x] += e->direction * (y1-y0) * (1-((x0-x)+(x1-x))/2); // coverage = 1 - average x position
   }
}

void fill_active_edges_new(float32 *scanline, float32 *scanline_fill, 
                           int32 len, active_edge *e, float32 y_top) {
   float32 y_bottom = y_top+1;

   while (e) {
      // brute force every pixel

      // compute intersection points with top & bottom
      ASSERT(e->ey >= y_top);

      if (e->fdx == 0) {
         float32 x0 = e->fx;
         if (x0 < len) {
            if (x0 >= 0) {
               handle_clipped_edge(scanline,(int) x0,e, x0,y_top, x0,y_bottom);
               handle_clipped_edge(scanline_fill-1,(int) x0+1,e, x0,y_top, x0,y_bottom);
            } else {
               handle_clipped_edge(scanline_fill-1,0,e, x0,y_top, x0,y_bottom);
            }
         }
      } else {
         float32 x0 = e->fx;
         float32 dx = e->fdx;
         float32 xb = x0 + dx;
         float32 x_top, x_bottom;
         float32 sy0,sy1;
         float32 dy = e->fdy;
         ASSERT(e->sy <= y_bottom && e->ey >= y_top);

         // compute endpoints of line segment clipped to this scanline (if the
         // line segment starts on this scanline. x0 is the intersection of the
         // line with y_top, but that may be off the line segment.
         if (e->sy > y_top) {
            x_top = x0 + dx * (e->sy - y_top);
            sy0 = e->sy;
         } else {
            x_top = x0;
            sy0 = y_top;
         }
         if (e->ey < y_bottom) {
            x_bottom = x0 + dx * (e->ey - y_top);
            sy1 = e->ey;
         } else {
            x_bottom = xb;
            sy1 = y_bottom;
         }

         if (x_top >= 0 && x_bottom >= 0 && x_top < len && x_bottom < len) {
            // from here on, we don't have to range check x values

            if ((int) x_top == (int) x_bottom) {
               float32 height;
               // simple case, only spans one pixel
               int32 x = (int) x_top;
               height = sy1 - sy0;
               ASSERT(x >= 0 && x < len);
               scanline[x] += e->direction * (1-((x_top - x) + (x_bottom-x))/2)  * height;
               scanline_fill[x] += e->direction * height; // everything right of this pixel is filled
            } else {
               int32 x,x1,x2;
               float32 y_crossing, step, sign, area;
               // covers 2+ pixels
               if (x_top > x_bottom) {
                  // flip scanline vertically; signed area is the same
                  float32 t;
                  sy0 = y_bottom - (sy0 - y_top);
                  sy1 = y_bottom - (sy1 - y_top);
                  t = sy0, sy0 = sy1, sy1 = t;
                  t = x_bottom, x_bottom = x_top, x_top = t;
                  dx = -dx;
                  dy = -dy;
                  t = x0, x0 = xb, xb = t;
               }

               x1 = (int) x_top;
               x2 = (int) x_bottom;
               // compute intersection with y axis at x1+1
               y_crossing = (x1+1 - x0) * dy + y_top;

               sign = e->direction;
               // area of the rectangle covered from y0..y_crossing
               area = sign * (y_crossing-sy0);
               // area of the triangle (x_top,y0), (x+1,y0), (x+1,y_crossing)
               scanline[x1] += area * (1-((x_top - x1)+(x1+1-x1))/2);

               step = sign * dy;
               for (x = x1+1; x < x2; ++x) {
                  scanline[x] += area + step/2;
                  area += step;
               }
               y_crossing += dy * (x2 - (x1+1));

               ASSERT(fabs(area) <= 1.01f);

               scanline[x2] += area + sign * (1-((x2-x2)+(x_bottom-x2))/2) * (sy1-y_crossing);

               scanline_fill[x2] += sign * (sy1-sy0);
            }
         } else {
            // if edge goes outside of box we're drawing, we require
            // clipping logic. since this does not match the intended use
            // of this library, we use a different, very slow brute
            // force implementation
            int32 x;
            for (x=0; x < len; ++x) {
               // cases:
               //
               // there can be up to two intersections with the pixel. any intersection
               // with left or right edges can be handled by splitting into two (or three)
               // regions. intersections with top & bottom do not necessitate case-wise logic.
               //
               // the old way of doing this found the intersections with the left & right edges,
               // then used some simple logic to produce up to three segments in sorted order
               // from top-to-bottom. however, this had a problem: if an x edge was epsilon
               // across the x border, then the corresponding y position might not be distinct
               // from the other y segment, and it might ignored as an empty segment. to avoid
               // that, we need to explicitly produce segments based on x positions.

               // rename variables to clearly-defined pairs
               float32 y0 = y_top;
               float32 x1 = (float32) (x);
               float32 x2 = (float32) (x+1);
               float32 x3 = xb;
               float32 y3 = y_bottom;

               // x = e->x + e->dx * (y-y_top)
               // (y-y_top) = (x - e->x) / e->dx
               // y = (x - e->x) / e->dx + y_top
               float32 y1 = (x - x0) / dx + y_top;
               float32 y2 = (x+1 - x0) / dx + y_top;

               if (x0 < x1 && x3 > x2) {         // three segments descending down-right
                  handle_clipped_edge(scanline,x,e, x0,y0, x1,y1);
                  handle_clipped_edge(scanline,x,e, x1,y1, x2,y2);
                  handle_clipped_edge(scanline,x,e, x2,y2, x3,y3);
               } else if (x3 < x1 && x0 > x2) {  // three segments descending down-left
                  handle_clipped_edge(scanline,x,e, x0,y0, x2,y2);
                  handle_clipped_edge(scanline,x,e, x2,y2, x1,y1);
                  handle_clipped_edge(scanline,x,e, x1,y1, x3,y3);
               } else if (x0 < x1 && x3 > x1) {  // two segments across x, down-right
                  handle_clipped_edge(scanline,x,e, x0,y0, x1,y1);
                  handle_clipped_edge(scanline,x,e, x1,y1, x3,y3);
               } else if (x3 < x1 && x0 > x1) {  // two segments across x, down-left
                  handle_clipped_edge(scanline,x,e, x0,y0, x1,y1);
                  handle_clipped_edge(scanline,x,e, x1,y1, x3,y3);
               } else if (x0 < x2 && x3 > x2) {  // two segments across x+1, down-right
                  handle_clipped_edge(scanline,x,e, x0,y0, x2,y2);
                  handle_clipped_edge(scanline,x,e, x2,y2, x3,y3);
               } else if (x3 < x2 && x0 > x2) {  // two segments across x+1, down-left
                  handle_clipped_edge(scanline,x,e, x0,y0, x2,y2);
                  handle_clipped_edge(scanline,x,e, x2,y2, x3,y3);
               } else {  // one segment
                  handle_clipped_edge(scanline,x,e, x0,y0, x3,y3);
               }
            }
         }
      }
      e = e->next;
   }
}

// directly AA rasterize edges w/o supersampling
void rasterize_sorted_edges(Arena* arena, Image* result, edge *e, int32 n, int32 vsubsample, 
                            int32 off_x, int32 off_y)
{
   hheap hh = { 0, 0, 0 };
   active_edge *active = NULL;
   int32 y,j=0, i;
   float32 scanline_data[129], *scanline, *scanline2;

   (void)(vsubsample);

   if (result->width > 64)
      scanline = (float32 *) ArenaAlloc(arena, (result->width*2+1) * sizeof(float32));
   else
      scanline = scanline_data;

   scanline2 = scanline + result->width;

   y = off_y;
   e[n].y0 = (float32) (off_y + result->height) + 1;

   while (j < result->height) {
      // find center of pixel for this scanline
      float32 scan_y_top    = y + 0.0f;
      float32 scan_y_bottom = y + 1.0f;
      active_edge **step = &active;

      memset(scanline , 0, result->width*sizeof(scanline[0]));
      memset(scanline2, 0, (result->width+1)*sizeof(scanline[0]));

      // update all active edges;
      // remove all active edges that terminate before the top of this scanline
      while (*step) {
         active_edge * z = *step;
         if (z->ey <= scan_y_top) {
            *step = z->next; // delete from list
            ASSERT(z->direction);
            z->direction = 0;
            hheap_free(&hh, z);
         } else {
            step = &((*step)->next); // advance through list
         }
      }

      // insert all edges that start before the bottom of this scanline
      while (e->y0 <= scan_y_bottom) {
         if (e->y0 != e->y1) {
            active_edge *z = new_active(arena, &hh, e, off_x, scan_y_top);
            if (z != NULL) {
               if (j == 0 && off_y != 0) {
                  if (z->ey < scan_y_top) {
                     // this can happen due to subpixel positioning and some kind of fp rounding error i think
                     z->ey = scan_y_top;
                  }
               }
               ASSERT(z->ey >= scan_y_top); // if we get really unlucky a tiny bit of an edge can be out of bounds
               // insert at front
               z->next = active;
               active = z;
            }
         }
         ++e;
      }

      // now process all active edges
      if (active)
         fill_active_edges_new(scanline, scanline2+1, result->width, active, scan_y_top);

      {
         float32 sum = 0;
         for (i=0; i < result->width; ++i) {
            float32 k;
            int32 m;
            sum += scanline2[i];
            k = scanline[i] + sum;
            k = (float32) fabs(k)*255 + 0.5f;
            m = (int32) k;
            if (m > 255) m = 255;
            result->data[j*result->channels + i] = (byte) m;
         }
      }
      // advance all the edges
      step = &active;
      while (*step) {
         active_edge *z = *step;
         z->fx += z->fdx; // advance to position for current scanline
         step = &((*step)->next); // advance through list
      }

      ++y;
      ++j;
   }

   hheap_cleanup(&hh);
}

#define COMPARE(a,b)  ((a)->y0 < (b)->y0)

void sort_edges_ins_sort(edge *p, int32 n)
{
   int32 i,j;
   for (i=1; i < n; ++i) {
      edge t = p[i], *a = &t;
      j = i;
      while (j > 0) {
         edge *b = &p[j-1];
         int32 c = COMPARE(a,b);
         if (!c) break;
         p[j] = p[j-1];
         --j;
      }
      if (i != j)
         p[j] = t;
   }
}

void sort_edges_quicksort(edge *p, int32 n)
{
   /* threshold for transitioning to insertion sort */
   while (n > 12) {
      edge t;
      int32 c01,c12,c,m,i,j;

      /* compute median of three */
      m = n >> 1;
      c01 = COMPARE(&p[0],&p[m]);
      c12 = COMPARE(&p[m],&p[n-1]);
      /* if 0 >= mid >= end, or 0 < mid < end, then use mid */
      if (c01 != c12) {
         /* otherwise, we'll need to swap something else to middle */
         int32 z;
         c = COMPARE(&p[0],&p[n-1]);
         /* 0>mid && mid<n:  0>n => n; 0<n => 0 */
         /* 0<mid && mid>n:  0>n => 0; 0<n => n */
         z = (c == c12) ? 0 : n-1;
         t = p[z];
         p[z] = p[m];
         p[m] = t;
      }
      /* now p[m] is the median-of-three */
      /* swap it to the beginning so it won't move around */
      t = p[0];
      p[0] = p[m];
      p[m] = t;

      /* partition loop */
      i=1;
      j=n-1;
      for(;;) {
         /* handling of equality is crucial here */
         /* for sentinels & efficiency with duplicates */
         for (;;++i) {
            if (!COMPARE(&p[i], &p[0])) break;
         }
         for (;;--j) {
            if (!COMPARE(&p[0], &p[j])) break;
         }
         /* make sure we haven't crossed */
         if (i >= j) break;
         t = p[i];
         p[i] = p[j];
         p[j] = t;

         ++i;
         --j;
      }
      /* recurse on smaller side, iterate on larger */
      if (j < (n-i)) {
         sort_edges_quicksort(p,j);
         p = p+i;
         n = n-i;
      } else {
         sort_edges_quicksort(p+i, n-i);
         n = j;
      }
   }
}

void sort_edges(edge *p, int32 n)
{
   sort_edges_quicksort(p, n);
   sort_edges_ins_sort(p, n);
}

void rasterize(Arena* arena, Image* result, Point2 *pts, int32 *wcount, 
               int32 windings, float32 scale_x, float32 scale_y, 
               float32 shift_x, float32 shift_y, 
               int32 off_x, int32 off_y, int32 invert) {
   float32 y_scale_inv = invert ? -scale_y : scale_y;
   edge *e;
   int32 n,i,j,k,m;
   int32 vsubsample = 1;
   // vsubsample should divide 255 evenly; otherwise we won't reach full opacity

   // now we have to blow out the windings into explicit edge lists
   n = 0;
   for (i=0; i < windings; ++i)
      n += wcount[i];

   e = (edge *) ArenaAlloc(arena, sizeof(*e) * (n+1)); // add an extra one as a sentinel
   if (e == 0) return;
   n = 0;

   m=0;
   for (i=0; i < windings; ++i) {
      Point2 *p = pts + m;
      m += wcount[i];
      j = wcount[i]-1;
      for (k=0; k < wcount[i]; j=k++) {
         int32 a=k,b=j;
         // skip the edge if horizontal
         if (p[j].y == p[k].y)
            continue;
         // add edge from j to k to the list
         e[n].invert = 0;
         if (invert ? p[j].y > p[k].y : p[j].y < p[k].y) {
            e[n].invert = 1;
            a=j,b=k;
         }
         e[n].x0 = p[a].x * scale_x + shift_x;
         e[n].y0 = (p[a].y * y_scale_inv + shift_y) * vsubsample;
         e[n].x1 = p[b].x * scale_x + shift_x;
         e[n].y1 = (p[b].y * y_scale_inv + shift_y) * vsubsample;
         ++n;
      }
   }

   // now sort the edges by their highest point (should snap to integer, and then by x)
   sort_edges(e, n);

   // now, traverse the scanlines and find the intersections on each scanline, use xor winding rule
   rasterize_sorted_edges(arena, result, e, n, vsubsample, off_x, off_y);
}

void add_point(Point2 *points, int32 n, float32 x, float32 y)
{
   if (!points) return; // during first pass, it's unallocated
   points[n].x = x;
   points[n].y = y;
}

// tessellate until threshold p is happy... @TODO warped to compensate for non-linear stretching
int32 tesselate_curve(Point2 *points, int32 *num_points, float32 x0, float32 y0, float32 x1, float32 y1, float32 x2, float32 y2, float32 objspace_flatness_squared, int32 n)
{
   // midpoint
   float32 mx = (x0 + 2*x1 + x2)/4;
   float32 my = (y0 + 2*y1 + y2)/4;
   // versus directly drawn line
   float32 dx = (x0+x2)/2 - mx;
   float32 dy = (y0+y2)/2 - my;
   if (n > 16) // 65536 segments on one curve better be enough!
      return 1;
   if (dx*dx+dy*dy > objspace_flatness_squared) { // half-pixel error allowed... need to be smaller if AA
      tesselate_curve(points, num_points, x0,y0, (x0+x1)/2.0f,(y0+y1)/2.0f, mx,my, objspace_flatness_squared,n+1);
      tesselate_curve(points, num_points, mx,my, (x1+x2)/2.0f,(y1+y2)/2.0f, x2,y2, objspace_flatness_squared,n+1);
   } else {
      add_point(points, *num_points,x2,y2);
      *num_points = *num_points+1;
   }
   return 1;
}

void tesselate_cubic(Point2 *points, int32 *num_points, float32 x0, float32 y0, float32 x1, float32 y1, float32 x2, float32 y2, float32 x3, float32 y3, float32 objspace_flatness_squared, int32 n)
{
   // @TODO this "flatness" calculation is just made-up nonsense that seems to work well enough
   float32 dx0 = x1-x0;
   float32 dy0 = y1-y0;
   float32 dx1 = x2-x1;
   float32 dy1 = y2-y1;
   float32 dx2 = x3-x2;
   float32 dy2 = y3-y2;
   float32 dx = x3-x0;
   float32 dy = y3-y0;
   float32 longlen = (float32) (sqrt(dx0*dx0+dy0*dy0)+sqrt(dx1*dx1+dy1*dy1)+sqrt(dx2*dx2+dy2*dy2));
   float32 shortlen = (float32) sqrt(dx*dx+dy*dy);
   float32 flatness_squared = longlen*longlen-shortlen*shortlen;

   if (n > 16) // 65536 segments on one curve better be enough!
      return;

   if (flatness_squared > objspace_flatness_squared) {
      float32 x01 = (x0+x1)/2;
      float32 y01 = (y0+y1)/2;
      float32 x12 = (x1+x2)/2;
      float32 y12 = (y1+y2)/2;
      float32 x23 = (x2+x3)/2;
      float32 y23 = (y2+y3)/2;

      float32 xa = (x01+x12)/2;
      float32 ya = (y01+y12)/2;
      float32 xb = (x12+x23)/2;
      float32 yb = (y12+y23)/2;

      float32 mx = (xa+xb)/2;
      float32 my = (ya+yb)/2;

      tesselate_cubic(points, num_points, x0,y0, x01,y01, xa,ya, mx,my, objspace_flatness_squared,n+1);
      tesselate_cubic(points, num_points, mx,my, xb,yb, x23,y23, x3,y3, objspace_flatness_squared,n+1);
   } else {
      add_point(points, *num_points,x3,y3);
      *num_points = *num_points+1;
   }
}

// returns number of contours
Point2 *FlattenCurves(Arena* arena, vertex *vertices, int32 num_verts, float32 objspace_flatness, int32 **contour_lengths, int32 *num_contours)
{
   Point2 *points=0;
   int32 num_points=0;

   float32 objspace_flatness_squared = objspace_flatness * objspace_flatness;
   int32 i,n=0,start=0, pass;

   // count how many "moves" there are to get the contour count
   for (i=0; i < num_verts; ++i)
      if (vertices[i].type == vmove)
         ++n;

   *num_contours = n;
   if (n == 0) return 0;

   *contour_lengths = (int32 *) ArenaAlloc(arena, sizeof(**contour_lengths) * n);

   if (*contour_lengths == 0) {
      *num_contours = 0;
      return 0;
   }

   // make two passes through the points so we don't need to realloc
   for (pass=0; pass < 2; ++pass) {
      float32 x=0,y=0;
      if (pass == 1) {
         points = (Point2 *) ArenaAlloc(arena, num_points * sizeof(points[0]));
         if (points == NULL) goto error;
      }
      num_points = 0;
      n= -1;
      for (i=0; i < num_verts; ++i) {
         switch (vertices[i].type) {
            case vmove:
               // start the next contour
               if (n >= 0)
                  (*contour_lengths)[n] = num_points - start;
               ++n;
               start = num_points;

               x = vertices[i].x, y = vertices[i].y;
               add_point(points, num_points++, x,y);
               break;
            case vline:
               x = vertices[i].x, y = vertices[i].y;
               add_point(points, num_points++, x, y);
               break;
            case vcurve:
               tesselate_curve(points, &num_points, x,y,
                                        vertices[i].cx, vertices[i].cy,
                                        vertices[i].x,  vertices[i].y,
                                        objspace_flatness_squared, 0);
               x = vertices[i].x, y = vertices[i].y;
               break;
            case vcubic:
               tesselate_cubic(points, &num_points, x,y,
                                        vertices[i].cx, vertices[i].cy,
                                        vertices[i].cx1, vertices[i].cy1,
                                        vertices[i].x,  vertices[i].y,
                                        objspace_flatness_squared, 0);
               x = vertices[i].x, y = vertices[i].y;
               break;
         }
      }
      (*contour_lengths)[n] = num_points - start;
   }

   return points;
error:
   *contour_lengths = 0;
   *num_contours = 0;
   return NULL;
}

void Rasterize(Arena* arena, Image* result, float32 flatness_in_pixels, vertex *vertices, int32 num_verts, 
               float32 scale_x, float32 scale_y, 
               float32 shift_x, float32 shift_y, 
               int32 x_off, int32 y_off, int32 invert)
{
   float32 scale            = scale_x > scale_y ? scale_y : scale_x;
   int32 winding_count      = 0;
   int32 *winding_lengths   = NULL;
   Point2 *windings = FlattenCurves(arena, vertices, num_verts, flatness_in_pixels / scale, &winding_lengths, &winding_count);
   if (windings) {
      rasterize(arena, result, windings, winding_lengths, winding_count, scale_x, scale_y, shift_x, shift_y, x_off, y_off, invert);
   }
}
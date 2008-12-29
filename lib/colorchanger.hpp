/* This file is part of MyPaint.
 * Copyright (C) 2008 by Martin Renold <martinxyz@gmx.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

class ColorChanger {
public:
  static const int size = 256;

  float brush_h, brush_s, brush_v;
  void set_brush_color(float h, float s, float v)
  {
    brush_h = h;
    brush_s = s;
    brush_v = v;
  }

#ifndef SWIG

  struct PrecalcData {
    int h;
    int s;
    int v;
    //signed char s;
    //signed char v;
  };

  PrecalcData * precalcData[4];
  int precalcDataIndex;

  ColorChanger()
  {
    precalcDataIndex = -1;
    for (int i=0; i<4; i++) {
      precalcData[i] = NULL;
    }
  }

  PrecalcData * precalc_data(float phase0)
  {
    // Hint to the casual reader: some of the calculation here do not
    // what I originally intended. Not everything here will make sense.
    // It does not matter in the end, as long as the result looks good.

    int width, height;
    float width_inv, height_inv;
    int x, y, i;
    PrecalcData * result;

    width = size;
    height = size;
    result = (PrecalcData*)malloc(sizeof(PrecalcData)*width*height);

    //phase0 = rand_double (rng) * 2*M_PI;

    width_inv = 1.0/width;
    height_inv = 1.0/height;

    i = 0;
    for (y=0; y<height; y++) {
      for (x=0; x<width; x++) {
        float h, s, v, s_original, v_original;
        int dx, dy;
        float v_factor = 0.8;
        float s_factor = 0.8;
        float h_factor = 0.05;

#define factor2_func(x) ((x)*(x)*SIGN(x))
        float v_factor2 = 0.01;
        float s_factor2 = 0.01;


        h = 0;
        s = 0;
        v = 0;

        dx = x-width/2;
        dy = y-height/2;

        // basically, its x-axis = value, y-axis = saturation
        v = dx*v_factor + factor2_func(dx)*v_factor2;
        s = dy*s_factor + factor2_func(dy)*s_factor2;

        v_original = v; s_original = s;

        // overlay sine waves to color hue, not visible at center, ampilfying near the border
        if (1) {
          float amplitude, phase;
          float dist, dist2, borderdist;
          float dx_norm, dy_norm;
          float angle;
          dx_norm = dx*width_inv;
          dy_norm = dy*height_inv;

          dist2 = dx_norm*dx_norm + dy_norm*dy_norm;
          dist = sqrtf(dist2);
          borderdist = 0.5 - MAX(ABS(dx_norm), ABS(dy_norm));
          angle = atan2f(dy_norm, dx_norm);
          amplitude = 50 + dist2*dist2*dist2*100;
          phase = phase0 + 2*M_PI* (dist*0 + dx_norm*dx_norm*dy_norm*dy_norm*50) + angle*7;
          //h = sinf(phase) * amplitude;
          h = sinf(phase);
          h = (h>0)?h*h:-h*h;
          h *= amplitude;

          // calcualte angle to next 45-degree-line
          angle = ABS(angle)/M_PI;
          if (angle > 0.5) angle -= 0.5;
          angle -= 0.25;
          angle = ABS(angle) * 4;
          // angle is now in range 0..1
          // 0 = on a 45 degree line, 1 = on a horizontal or vertical line

          v = 0.6*v*angle + 0.4*v;
          h = h * angle * 1.5;
          s = s * angle * 1.0;

          // this part is for strong color variations at the borders
          if (borderdist < 0.3) {
            float fac;
            float h_new;
            fac = (1 - borderdist/0.3);
            // fac is 1 at the outermost pixels
            v = (1-fac)*v + fac*0;
            s = (1-fac)*s + fac*0;
            fac = fac*fac*0.6;
            h_new = (angle+phase0+M_PI/4)*360/(2*M_PI) * 8;
            while (h_new > h + 360/2) h_new -= 360;
            while (h_new < h - 360/2) h_new += 360;
            h = (1-fac)*h + fac*h_new;
            //h = (angle+M_PI/4)*360/(2*M_PI) * 4;
          }
        }

        {
          // undo that funky stuff on horizontal and vertical lines
          int min = ABS(dx);
          if (ABS(dy) < min) min = ABS(dy);
          if (min < 30) {
            float mul;
            min -= 6;
            if (min < 0) min = 0;
            mul = min / (30.0-1.0-6.0);
            h = mul*h; //+ (1-mul)*0;

            v = mul*v + (1-mul)*v_original;
            s = mul*s + (1-mul)*s_original;
          }
        }

        h -= h*h_factor;

        result[i].h = (int)h;
        result[i].v = (int)v;
        result[i].s = (int)s;
        i++;
      }
    }
    return result;
  }

  void get_hsv(float &h, float &s, float &v, PrecalcData * pre)
  {
    h = brush_h + pre->h/360.0;
    s = brush_s + pre->s/255.0;
    v = brush_v + pre->v/255.0;

    if (s < 0) { if (s < -0.2) { s = - (s + 0.2); } else { s = 0; } } 
    if (s > 1) { if (s > 1.0 + 0.2) { s = 1.0 - ((s-0.2)-1.0); } else { s = 1.0; } }
    if (v < 0) { if (v < -0.2) { v = - (v + 0.2); } else { v = 0; } }
    if (v > 1) { if (v > 1.0 + 0.2) { v = 1.0 - ((v-0.2)-1.0); } else { v = 1.0; } }

  }

#endif /* #ifndef SWIG */

  void render(PyObject * arr)
  {
    uint8_t * pixels;
    int x, y;
    float h, s, v;

    assert(PyArray_ISCARRAY(arr));
    assert(PyArray_NDIM(arr) == 3);
    assert(PyArray_DIM(arr, 0) == size);
    assert(PyArray_DIM(arr, 1) == size);
    assert(PyArray_DIM(arr, 2) == 4);
    pixels = (uint8_t*)((PyArrayObject*)arr)->data;
    
    precalcDataIndex++;
    precalcDataIndex %= 4;

    PrecalcData * pre = precalcData[precalcDataIndex];
    if (!pre) {
      pre = precalcData[precalcDataIndex] = precalc_data(2*M_PI*(precalcDataIndex/4.0));
    }

    for (y=0; y<size; y++) {
      for (x=0; x<size; x++) {

        get_hsv(h, s, v, pre);
        pre++;

        hsv_to_rgb_range_one (&h, &s, &v);
        uint8_t * p = pixels + 4*(y*size + x);
        p[0] = h; p[1] = s; p[2] = v; p[3] = 255;
      }
    }
  }

  PyObject* pick_color_at(float x_, float y_)
  {
    float h,s,v;
    PrecalcData * pre = precalcData[precalcDataIndex];
    assert(precalcDataIndex >= 0);
    assert(pre != NULL);
    int x = CLAMP(x_, 0, size);
    int y = CLAMP(y_, 0, size);
    pre += y*size + x;
    get_hsv(h, s, v, pre);
    return Py_BuildValue("fff",h,s,v);
  }
};

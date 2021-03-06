/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.opengl.cts;

public class Shaders {
    public static String successfulcompile_frag =
        "#ifdef GL_ES \n"
        + "precision mediump float; \n"
        + "#endif \n"
        + "uniform float   mortarThickness; \n"
        + "uniform vec3    brickColor; \n"
        + "uniform vec3    mortarColor; \n"

        + "uniform float   brickMortarWidth; \n"
        + "uniform float   brickMortarHeight; \n"
        + "uniform float   mwf; \n"
        + "uniform float   mhf; \n"

        + "varying vec3  Position; \n"
        + "varying float lightIntensity; \n"
        + "\n"
        + "void main (void){ \n"
        + "    vec3    ct; \n"
        + "    float   ss, tt, w, h; \n"
        + ""
        + "    vec3 pos = Position; \n"
        + ""
        + "    ss = pos.x / brickMortarWidth; \n"
        + "    tt = pos.z / brickMortarHeight; \n"
        + "    if (fract (tt * 0.5) > 0.5) \n"
        + "        ss += 0.5; \n"

        + "    ss = fract (ss); \n"
        + "    tt = fract (tt); \n"

        + "    w = step (mwf, ss) - step (1.0 - mwf, ss); \n"
        + "    h = step (mhf, tt) - step (1.0 - mhf, tt); \n"

        + "    ct = clamp(mix (mortarColor, brickColor, w * h) * lightIntensity, 0.0, 1.0); \n"

        + "    gl_FragColor = vec4 (ct, 1.0); \n"
        + "} \n";

    public static String errorcompile_frag =
        "#ifdef GL_ES \n"
        + "precision mediump float; \n"
        + "#endif \n"
        + "uniform float   mortarThickness; \n"
        + "uniform vec3    brickColor; \n"
        + "uniform vec3    mortarColor; \n"

        + "uniform float   brickMortarWidth; \n"
        + "uniform float   brickMortarHeight; \n"
        + "uniform float   mhf; \n"

        + "varying vec3  Position; \n"
        + "varying float lightIntensity; \n"
        + "\n"
        + "void main (void){ \n"
        + "    vec3    ct; \n"
        + "    float   ss, tt, w, h; \n"
        + ""
        + "    vec3 pos = Position; \n"
        + ""
        + "    ss = pos.x / brickMortarWidth; \n"
        + "    tt = pos.z / brickMortarHeight; \n"
        + "    if (fract (tt * 0.5) > 0.5) \n"
        + "        ss += 0.5; \n"

        + "    ss = fract (ss); \n"
        + "    tt = fract (tt); \n"

        + "    w = step (mwf, ss) - step (1.0 - mwf, ss); \n"
        + "    h = step (mhf, tt) - step (1.0 - mhf, tt); \n"

        + "    ct = clamp(mix (mortarColor, brickColor, w * h) * lightIntensity, 0.0, 1.0); \n"

        + "    gl_FragColor = vec4 (ct, 1); \n"
        + "} \n";
}
